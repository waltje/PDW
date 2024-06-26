//
// initapp.c
//
// Declares main app globals and the following functions:
//
//            InitApplication()
//            InitInstance()
//            InitializePane1()
//            InitializePane2()
//
#include <windows.h>
#include <io.h>
#include <stdio.h>
#include <string.h>
#include "resource.h"
#include "pdw.h"
#include "gfx.h"
#include "utils/utils.h"


HINSTANCE ghInstance;			// current instance
HWND    ghWnd;				// main window handle
HWND    hToolbar = NULL;		// toolbar handle
HACCEL  ghAccel;
HMENU   ghMenu = NULL;			// main application menu
TCHAR   szAppName[128];			// name of the application (PH: was 64)
TCHAR   szShortAppName[32];		// short name of the application
TCHAR   szApiFailedMsg[64];		// "A Windows API Failed" message

PaneStruct Pane1;
PaneStruct Pane2;

FILE *pLogFile = NULL;			// Logged messages
FILE *pFilterFile = NULL;		// Filtered messages
FILE *pSepFilterFile = NULL;		// Separate filtered messages
FILE *pStatFile = NULL;			// Statistics
FILE *pFiltersFile = NULL;		// PH: Filters.ini

unsigned cxChar, cyChar, cxCaps;	// all windows use the same font size
int iMaxWidth;
int sizeSet = 0;
int pane1Height, pane2Height, pane1Pos, pane2Pos, pane1Top;

char  gszPDWClass[15] = "WinPDWWndClass";
char  gszPane1Class[17] = "WinPDWPane1Class";
char  gszPane2Class[17] = "WinPDWPane2Class";
char  gszPane2LabelClass[22] = "WinPDWPane2LabelClass";
char  gszColorClass[17] = "WinPDWColorClass";
char  gszACARSColorClass[19] = "WinACARSColorClass";
char  gszMOBITEXColorClass[21] = "WinMOBITEXColorClass";
char  gszERMESColorClass[19] = "WinERMESColorClass";

LPCTSTR lpszSourceFileName = TEXT(__FILE__);

TCHAR szDialogErrorMsg[80];
TCHAR szCenterOpenDlgMsg[80];

TCHAR szPath[MAX_PATH];          // path where the running application resides
TCHAR szLogPathName[MAX_PATH];   // Logfile-path
TCHAR szWavePathName[MAX_PATH];  // Wavefile-path
TCHAR szExePathName[MAX_PATH];   // full pathname of the application
TCHAR szHelpPathName[MAX_PATH];  // full pathname of the application's help file
TCHAR szIniPathName[MAX_PATH];   // full pathname of the application's ini file
TCHAR szFilterPathName[MAX_PATH];// full pathname of the application's filtersfile
TCHAR szFilterBackup[MAX_PATH];  // full pathname of the application's filters backupfile


static UINT
GetPathFromFullPathName(LPCTSTR lpFullPathName, LPTSTR lpPathBuffer,
                        UINT nPathBufferLength)
{
    UINT nLength;
    int i;

    if ((nLength = (UINT) lstrlen(lpFullPathName)) > nPathBufferLength)
	return nLength;

   lstrcpy(lpPathBuffer, lpFullPathName);

   for (i = lstrlen(lpPathBuffer);(lpPathBuffer[i] != '\\') && (lpPathBuffer[i] != ':'); i--);

   if (lpPathBuffer[i] == ':')
	lpPathBuffer[i+1] = '\0';
   else
	lpPathBuffer[i] = '\0';

   return (UINT)i;
}


BOOL
InitApplication(HINSTANCE hInstance)
{
    WNDCLASS wc;
    TCHAR szApiFailedMsg[] = TEXT("A Windows API Failed");
    LPCTSTR lpszIniFileExt = TEXT("INI");
    LPCTSTR lpszHelpFileExt = TEXT("HLP");
    TCHAR temp[1024], try[1024], *p;
    int c;

    // Load the "A Windows API Failed" resource string.
    if (! LoadString(ghInstance, IDS_API_FAILED_MSG, szApiFailedMsg, sizeof(szApiFailedMsg)))
	 ErrorMessageBox(TEXT("1 LoadString()"), szApiFailedMsg, lpszSourceFileName, __LINE__);

    // Load resource strings.
    if (! LoadString(ghInstance, IDS_APPNAME, szAppName, sizeof(szAppName)))
	 ErrorMessageBox(TEXT("2 LoadString()"), szApiFailedMsg, lpszSourceFileName, __LINE__);
    if (! LoadString(ghInstance, IDS_SHORT_APPNAME, szShortAppName, sizeof(szShortAppName)))
	 ErrorMessageBox(TEXT("3 LoadString()"), szApiFailedMsg, lpszSourceFileName, __LINE__);

    // Get application pathname and store the ini and help file pathname.
    GetModuleFileName(NULL, szExePathName, sizeof(szExePathName)/sizeof(TCHAR));
    GetPathFromFullPathName(szExePathName, szPath, sizeof(szPath)/sizeof(TCHAR));

    /*
     * See if we are perhaps in a "bin/" subfolder of the
     * installation path, in which case the real root of
     * the installation is one level up. We can test this
     * by looking for the 'bin' folder.
     */
    strncpy(temp, szPath, sizeof(temp)-9);
    strcat(temp, "\\PDW.hlp");

    if (_access(temp, 0) != 0) {
        /* No 'bin' folder found, so go up one level. */
        strncpy(temp, szPath, sizeof(temp)-1);

        /* Try this for several "levels" up. */
        for (c = 0; c < 4; c++) {
		p = strrchr(temp, '\\');
                if (p != NULL)
                    *p = '\0';

                strncpy(try, temp, sizeof(try)-1);
                strcat(try, "\\PDW.hlp");

                if (_access(try, 0) == 0) {
                        if (p != NULL)
                                *p = '\0';
                        strcpy(szPath, temp);
                        break;
                }
        }
    }

fprintf(stderr, "==> '%s'\n", szPath);
    wsprintf(szLogPathName, TEXT("%s\\%s"), szPath, "Logfiles");
    wsprintf(szWavePathName, TEXT("%s\\%s"), szPath, "Wavfiles");
    wsprintf(szIniPathName, TEXT("%s\\%s.%s"), szPath, szShortAppName, lpszIniFileExt);
    wsprintf(szHelpPathName, TEXT("%s\\%s.%s"),	szPath, szShortAppName, lpszHelpFileExt);
    wsprintf(szFilterPathName, TEXT("%s\\%s"), szPath, "filters.ini");
    wsprintf(szFilterBackup, TEXT("%s\\%s"), szPath, "filters.bak");

    if (! FileExists(szWavePathName))
	CreateDirectory(szWavePathName, NULL);

    GetPrivateProfileSettings(szShortAppName, szIniPathName, &Profile);

    /* After getting user profile settings get all drawing objects. */
    if (! Get_Drawing_Objects())
	return FALSE;

    // register window class
    wc.style		= 0;
    wc.lpfnWndProc	= PDWWndProc;
    wc.cbClsExtra	= 0;
    wc.cbWndExtra	= sizeof(LONG);
    wc.hInstance	= hInstance;
    wc.hIcon		= LoadIcon(hInstance, MAKEINTRESOURCE(PDWICON));
    wc.hCursor		= LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground	= hbr;
    wc.lpszMenuName	= MAKEINTRESOURCE(PDWMENU);
    wc.lpszClassName	= gszPDWClass;
    if (! RegisterClass(&wc))
	return FALSE;

    wc.style		= 0;
    wc.lpfnWndProc	= Pane1WndProc;
    wc.cbClsExtra	= 0;
    wc.cbWndExtra	= sizeof(LONG);
    wc.hInstance	= hInstance;
    wc.hIcon		= NULL;
    wc.hCursor		= LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground	= hbr;
    wc.lpszMenuName	= NULL;
    wc.lpszClassName	= gszPane1Class;
    if (! RegisterClass(&wc))
	return FALSE;

    wc.style		= 0;
    wc.lpfnWndProc	= Pane2WndProc;
    wc.cbClsExtra	= 0;
    wc.cbWndExtra	= sizeof(LONG);
    wc.hInstance	= hInstance;
    wc.hIcon		= NULL;
    wc.hCursor		= LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground	= hbr;
    wc.lpszMenuName	= NULL;
    wc.lpszClassName	= gszPane2Class;
    if (! RegisterClass(&wc))
	return FALSE;

    wc.style		= 0;
    wc.lpfnWndProc	= ColorWndProc;
    wc.cbClsExtra	= 0;
    wc.cbWndExtra	= sizeof(LONG);
    wc.hInstance	= hInstance;
    wc.hIcon		= NULL;
    wc.hCursor		= LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground	= hboxbr;
    wc.lpszMenuName	= NULL;
    wc.lpszClassName	= gszColorClass;
    if (! RegisterClass(&wc))
	return FALSE;

    wc.style		= 0;
    wc.lpfnWndProc	= ACARSColorWndProc;
    wc.cbClsExtra	= 0;
    wc.cbWndExtra	= sizeof(LONG);
    wc.hInstance	= hInstance;
    wc.hIcon		= NULL;
    wc.hCursor		= LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground	= hboxbr;
    wc.lpszMenuName	= NULL;
    wc.lpszClassName	= gszACARSColorClass;
    if (! RegisterClass(&wc))
	return FALSE;

    wc.style		= 0;
    wc.lpfnWndProc	= MOBITEXColorWndProc;
    wc.cbClsExtra	= 0;
    wc.cbWndExtra	= sizeof(LONG);
    wc.hInstance	= hInstance;
    wc.hIcon		= NULL;
    wc.hCursor		= LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground	= hboxbr;
    wc.lpszMenuName	= NULL;
    wc.lpszClassName	= gszMOBITEXColorClass;
    if (! RegisterClass(&wc))
	return FALSE;

    wc.style		= 0;
    wc.lpfnWndProc	= ERMESColorWndProc;
    wc.cbClsExtra	= 0;
    wc.cbWndExtra	= sizeof(LONG);
    wc.hInstance	= hInstance;
    wc.hIcon		= NULL;
    wc.hCursor		= LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground	= hboxbr;
    wc.lpszMenuName	= NULL;
    wc.lpszClassName	= gszERMESColorClass;
    if (! RegisterClass(&wc))
	return FALSE;

    return TRUE;
}


// Instance stuff
HWND
InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    WINDOWPLACEMENT wpl;
    POINT pt1, pt2;
    RECT rc;

    // load accelerators
    ghAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(PDWACCEL));

    ghWnd = CreateWindow(gszPDWClass, szAppName,
			 WS_OVERLAPPEDWINDOW,
			 Profile.xPos, Profile.yPos,
			 Profile.xSize, Profile.ySize,
			 NULL, NULL, hInstance, NULL);
    if (ghWnd == NULL)
	return NULL;

    ShowWindow(ghWnd, nCmdShow);

    // Maximize window?
    if (Profile.maximize_tmp)
	ShowWindow(ghWnd, SW_SHOWMAXIMIZED);

    UpdateWindow(ghWnd);

    // In case first Maximize attempt failed, set again,
    // also set minimize info!
    if (Profile.maximize_tmp) {
	// Set upper left corner for when minimized.
	pt1.x = 0; pt1.y = 0;
	pt2.x = 0; pt2.y = 0;

	// Set restore info (use profile defaults)
	rc.left = 0;		// Profile.xPos;
	rc.right = 593;		// Profile.xSize;
	rc.top = 0;		// Profile.yPos;
	rc.bottom = 442;	// Profile.ySize;

	wpl.length = sizeof(WINDOWPLACEMENT);
	wpl.flags = WPF_RESTORETOMAXIMIZED;
	wpl.showCmd = SW_SHOWMAXIMIZED;
	wpl.ptMinPosition = pt1;
	wpl.ptMaxPosition = pt2;
	wpl.rcNormalPosition = rc;
	SetWindowPlacement(ghWnd, &wpl);
	Profile.maximize_flg = 1;
    }

    return ghWnd;
}


void
InitializePane(PaneStruct *pane)
{
    char *pchar;
    BYTE *pcolor;
    unsigned x;

    pane->Bottom = 0;

    pchar = pane->buff_char;
    pcolor = pane->buff_color;

    for (x = 0; x < pane->buff_lines; x++) {
	pchar[x * (LINE_SIZE+1) + 0] = 0;
	pcolor[x * (LINE_SIZE+1) + 0] = COLOR_UNUSED;
    }

    pane->currentPos   = 0;
    pane->currentColor = COLOR_UNUSED;

    pane->iVscrollPos = 0;
    pane->iVscrollMax = 0;
    pane->iHscrollPos = 0;
    pane->iHscrollMax = 0;

    SetScrollRange(pane->hWnd, SB_VERT, 0, 0, TRUE);
    SetScrollRange(pane->hWnd, SB_HORZ, 0, 0, TRUE);
}
