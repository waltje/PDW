#include <windows.h>
#include <winbase.h>
#include <stdio.h>
#include "../pdw.h"
#include "debug.h"
#include "ostype.h"
#include "rs232.h"


#define SLICER_BUFSIZE 10000

#define assert(a)	if (!(a))  { OUTPUTDEBUGMSG(("SIMULATE ASSERT in file %s at %d\n", __FILE__, __LINE__ )); }


static volatile HANDLE hRxThread = INVALID_HANDLE_VALUE;
static HANDLE	hComPort = INVALID_HANDLE_VALUE;
static DWORD	dwThreadId = 0;
static int	bConnectedToComport = FALSE,
		bKeepThreadAlive,
		bOrgcomPortRS232,
		bSlicerDriver = FALSE;
static double	nTiming;
static DWORD	rs232_cpstn;
static BYTE	byRS232Data[SLICER_BUFSIZE * (sizeof(WORD) + sizeof(BYTE))];
static WORD	rs232_freqdata[SLICER_BUFSIZE];
static BYTE	rs232_linedata[SLICER_BUFSIZE];


static int
rs232_read(void) 
{
    BYTE buff[256];
    DWORD dwRead;
    int bit, i, j;

    // OUTPUTDEBUGMSG((("rs232_read() \n")));
    if (hComPort == INVALID_HANDLE_VALUE) {
	OUTPUTDEBUGMSG((("rs232_read : COMport not open!\n")));
	return 0;
    }

    if (! ReadFile(hComPort, buff, sizeof(buff), &dwRead, 0)) {
	OUTPUTDEBUGMSG((("rs232_read : Error in reading 0x%0x!\n"), GetLastError()));
	PurgeComm(hComPort, PURGE_RXCLEAR);
    } else {
	for (i = 0; i < dwRead; i++) {
		for (j = 7; j >= 0; j--) {
			if (Profile.fourlevel) {
				j-- ;
				bit = (buff[i] >> j) & 0x03;
			} else {
				bit = (buff[i] >> j) & 0x01;
			}	

			rs232_linedata[rs232_cpstn] = bit << 4;
			rs232_freqdata[rs232_cpstn++] = nTiming;	
			if (rs232_cpstn >= SLICER_BUFSIZE)
				rs232_cpstn = 0;
		}
	}
    }

    return 0;
}


static int
slicer_read(void) 
{
    DWORD dwRead, i, num;
    WORD *freq;
    BYTE *line;

    if (hComPort == INVALID_HANDLE_VALUE) {
	OUTPUTDEBUGMSG((("slicer_read : COMport not open!\n")));
	return 0;
    }

    if (! ReadFile(hComPort, byRS232Data, sizeof(byRS232Data), &dwRead, 0)) {
	OUTPUTDEBUGMSG((("slicer_read : Error in reading 0x%0x!\n"), GetLastError()));
	PurgeComm(hComPort, PURGE_RXCLEAR) ;
    }

    num = dwRead / (sizeof(WORD) + sizeof(BYTE));
//  OUTPUTDEBUGMSG((("slicer_read : Reading %d bytes --> %d slices\n"), dwRead, num));
    line = byRS232Data;
    freq = (WORD *) (byRS232Data + num * sizeof(BYTE));
    for (i = 0; i < num; i++) {
	rs232_linedata[rs232_cpstn] = *line++;
	rs232_freqdata[rs232_cpstn++] = *freq++;
	if (rs232_cpstn >= SLICER_BUFSIZE)
		rs232_cpstn = 0;
    }

    return i;
}


/***********************************************************
 * This worker thread will place received data in a mailbox *
 ***********************************************************/
static DWORD WINAPI
RxThread(LPVOID pCl)
{
    do {
	if (bOrgcomPortRS232) {
		rs232_read();
	} else {
		slicer_read();
	}

	Sleep(50);
    } while (bKeepThreadAlive);
	
    OUTPUTDEBUGMSG(("RxThread : terminating...\n"));
    hRxThread = INVALID_HANDLE_VALUE;
    ExitThread(0L);

    return 0;
}


int
rs232_connect(SLICER_IN_STR *pInSlicer, SLICER_OUT_STR *pOutSlicer)
{
    COMMTIMEOUTS cto = { 0 };
    COMMPROP cp = { 0 };
    DCB dcb = { 0 };
    HANDLE h;
    int rc = RS232_NO_DUT;
    char pcComPort[] = "COM1:";
extern double ct1600;

    // This as user can switch to slicer/rs232 without ports are close/opened (yet)
    bOrgcomPortRS232 = Profile.comPortRS232;
    if (pInSlicer->com_port > 9)
	sprintf(pcComPort, "\\\\.\\COM%d", pInSlicer->com_port);
    else
	sprintf(pcComPort, "COM%d", pInSlicer->com_port);

    switch (Profile.comPortRS232) {
	case 1:
		nTiming = 500;
		break;

	case 2:
	default:
		nTiming = ct1600;
		break;

	case 3:
		nTiming = 1.0/((float) 8000 * 839.22e-9);
		break;
    }
    OUTPUTDEBUGMSG((("calling: rs232_connect(%s)\n"), pcComPort));

    pOutSlicer->freqdata = rs232_freqdata;
    pOutSlicer->linedata = rs232_linedata;
    pOutSlicer->cpstn = &rs232_cpstn;
    pOutSlicer->bufsize = SLICER_BUFSIZE;

    if (bConnectedToComport) {
	rc = rs232_disconnect();
	assert (rc >= 0);
	if (rc < 0)
		return rc;
    }

    /********************************************************************
     * Seek contact with the serial.sys driver.				*
     * Configure it for overlapped operation, this is done so the	*
     * receiving thread (later on in this code) can be terminated by	*
     * the main thread							*
     ********************************************************************/	
    h = CreateFile(pcComPort,
		   GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
    if (h == INVALID_HANDLE_VALUE) {
	OUTPUTDEBUGMSG((("ERROR: CreateFile() %08lX!\n"), GetLastError()));
	return RS232_NO_DUT;
    }

    // We must check if this is a Slicer Driver
    if (! GetCommProperties(h, &cp)) {
	OUTPUTDEBUGMSG((("ERROR: GetCommProperties() %08lX!\n"), GetLastError()));
	CloseHandle(h);
	return RS232_NO_DUT;
    }

    if (cp.dwProvSpec1 == 0x48576877 && cp.dwProvSpec2 == 0x68774857)
	bSlicerDriver = TRUE;
    else 
	bSlicerDriver = FALSE;

    if (! bOrgcomPortRS232) {
	if (! bSlicerDriver) {
		OUTPUTDEBUGMSG((("ERROR:ComProp.dwProvSpec1 != 0x48576877 || ComProp.dwProvSpec2 != 0x68774857!\n")));
		CloseHandle(h);
		MessageBox(NULL,
			   "Please install the Slicer driver from the install package!",
			   "Slicer Driver Not Installed",
			   MB_OK | MB_ICONEXCLAMATION);

		return RS232_NO_DUT;
	}
    }

    // We got a connection with the serial.sys driver.
    if (! GetCommState(h, &dcb)) {
	OUTPUTDEBUGMSG((("ERROR: GetCommState() %08lX!\n"), GetLastError()));
	CloseHandle(h);
	return RS232_NO_DUT;
    }

    // Our specific part of the connection 
    // All the = Zero and by that default
    dcb.BaudRate = bOrgcomPortRS232 ? CBR_19200 : (nOSType == OS_WIN2000) ? CBR_SLICER_2K : CBR_SLICER_XP;	// This means SlicerMode for the COM driver
    dcb.ByteSize = 8 ;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fBinary = TRUE;
    dcb.fParity = FALSE;
    dcb.fDtrControl = DTR_CONTROL_DISABLE;
    dcb.fRtsControl = bOrgcomPortRS232 ? RTS_CONTROL_DISABLE : RTS_CONTROL_ENABLE ;
    if (! SetCommState(h, &dcb)) {
	OUTPUTDEBUGMSG((("ERROR: GetCommState() %08lX!\n"), GetLastError()));
	CloseHandle(hComPort);
	return RS232_NO_DUT;
    }

    if (! SetCommMask(h, bOrgcomPortRS232 ? 0 : EV_CTS | EV_DSR | EV_RLSD)) {
	OUTPUTDEBUGMSG((("ERROR: SetCommMask() %08lX!\n"), GetLastError()));
	CloseHandle(h);
	return RS232_NO_DUT;
    }

    /* Purge buffers:*/
    if (! PurgeComm(h, PURGE_TXABORT|PURGE_RXABORT|PURGE_TXCLEAR|PURGE_RXCLEAR)) {
	OUTPUTDEBUGMSG((("ERROR: PurgeComm() %08lX!\n"), GetLastError()));
	CloseHandle(h);
	return RS232_NO_DUT;
    }

    if (bOrgcomPortRS232) {
	cto.ReadIntervalTimeout = cto.ReadTotalTimeoutMultiplier = MAXDWORD;
	cto.ReadTotalTimeoutConstant = 100; // 100 ms timeout
	if (! SetCommTimeouts(h, &cto)) {
		OUTPUTDEBUGMSG((("ERROR: SetCommTimeouts() %08lX!\n"), GetLastError()));
		CloseHandle(h);
		return RS232_NO_DUT;
	}
    }

    /* We are connected! */
    hComPort = h;
    bConnectedToComport = TRUE;

    /*
     * Next step: fire up a thread that takes care of placing received
     * data in a buffer
     */
    bKeepThreadAlive = TRUE;

    hRxThread = CreateThread(NULL,
			     0,
			     RxThread, NULL,
			     CREATE_SUSPENDED, &dwThreadId);
    ResumeThread(hRxThread);

    return RS232_SUCCESS;
}


int
rs232_disconnect(void)
{
    COMMTIMEOUTS cto = { 0 };
    int rc;

    OUTPUTDEBUGMSG(("calling: rs232_disconnect()\n"));
    if (! bConnectedToComport) {
	// return when already connected
	return RS232_NO_CONNECTION;		
    }

    /***************************
     * Terminate the Rx thread *
     ***************************/
    OUTPUTDEBUGMSG(("main thread : set Rx Thread terminate event\n"));
    bKeepThreadAlive = FALSE;
    Sleep(250);
    TerminateThread(hRxThread, -1);
    Sleep(100);
    OUTPUTDEBUGMSG(("main thread : closing handles\n"));

    assert(hComPort != INVALID_HANDLE_VALUE);

    if (! SetCommTimeouts(hComPort, &cto)) {
	OUTPUTDEBUGMSG((("ERROR: SetCommTimeouts() %08lX!\n"), GetLastError()));
    }

    if (! SetCommMask(hComPort, 0)) {
	OUTPUTDEBUGMSG((("ERROR: SetCommMask() %08lX!\n"), GetLastError()));
    }

    rc = CloseHandle(hComPort);
    if (! rc) {
	OUTPUTDEBUGMSG(("main thread : error closing handle!\n"));
	rc = RS232_UNKNOWN;
    } else {
	OUTPUTDEBUGMSG(("main thread : handle closed.\n"));
    }

    /* Either way... */
    hComPort = INVALID_HANDLE_VALUE;
    bConnectedToComport = FALSE;

    return RS232_SUCCESS;
}


/* Create a list of usable serial ports. */
int *
rs232_find_ports(void)
{
    static int array[32];
    char port[32];
    DWORD error;
    int nNumFound = 0;
    HANDLE h;
    int i;

    memset(&array, 0x00, sizeof(array));

    for (i = 1; i < 50; i++) {
	if (i > 9)
		wsprintf(port, "\\\\.\\COM%d", i);
	else
		wsprintf(port, "COM%d", i);

	error = ERROR_SUCCESS;
	h = CreateFile(port, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
	if (h == INVALID_HANDLE_VALUE)
		error = GetLastError();
	if (error == ERROR_FILE_NOT_FOUND) {
		OUTPUTDEBUGMSG((("COM%d: Not Found\n"), i));
		CloseHandle(h);
	} else {
		OUTPUTDEBUGMSG((("COM%d: Found\n"), i));
		CloseHandle(h);
		array[nNumFound++] = i;
	}
    }

    array[nNumFound] = 0;

    return array;
}


int
rs232_driver_type(void)
{
    return bConnectedToComport ? (bSlicerDriver ?  DRIVER_TYPE_SLICER : DRIVER_TYPE_RS232) : DRIVER_TYPE_NOT_LOADED;
}
