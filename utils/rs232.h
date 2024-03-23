#ifndef PDW_RS232_H
# define PDW_RS232_H


#define CBR_SLICER_2K			CBR_110  // 100
#define CBR_SLICER_XP			CBR_300  // 200

#define RS232_SUCCESS			0
#define RS232_NO_DUT			1
#define RS232_NO_CONNECTION		2
#define RS232_UNKNOWN			3

#define DRIVER_TYPE_NOT_LOADED	0
#define DRIVER_TYPE_SLICER		1
#define DRIVER_TYPE_RS232		2


#ifdef __cplusplus
extern "C" {
#endif

int rs232_connect(SLICER_IN_STR *pInSlicer, SLICER_OUT_STR *pOutSlicer);
int rs232_transmit_data(unsigned char buffer[], int nBytes);
int rs232_get_rx_data(unsigned char buffer[], int nBytes);
int rs232_disconnect(void);
int rs232_read(void);
int slicer_read(void);

int OpenComPort(void);
int WriteComPort(char *szLine);
int CloseComPort(void);

int *FindComPorts(void);

int GetRs232DriverType(void);

#ifdef __cplusplus
}
#endif


#endif	/*PDW_RS232_H*/
