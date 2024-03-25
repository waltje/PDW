#ifndef PDW_RS232_H
# define PDW_RS232_H


#define CBR_SLICER_2K		CBR_110  // 100
#define CBR_SLICER_XP		CBR_300  // 200

#define RS232_SUCCESS		0
#define RS232_NO_DUT		1
#define RS232_NO_CONNECTION	2
#define RS232_UNKNOWN		3

#define DRIVER_TYPE_NOT_LOADED	0
#define DRIVER_TYPE_SLICER	1
#define DRIVER_TYPE_RS232	2


typedef struct {
   unsigned int  irq;        // IRQ number for COM port
   unsigned int  com_port;   // I/O address for COM port
} SLICER_IN_STR;

typedef struct {
   unsigned short *freqdata; // each freqdata entry is 16 bits
   unsigned char *linedata;  // each linedata entry is 8 bits
   unsigned long *cpstn;     // current offset into freqdata and linedata
   long bufsize;             // size of freqdata and linedata buffers
} SLICER_OUT_STR;


#ifdef __cplusplus
extern "C" {
#endif

int *rs232_find_ports(void);
int rs232_connect(SLICER_IN_STR *pInSlicer, SLICER_OUT_STR *pOutSlicer);
int rs232_disconnect(void);
int rs232_driver_type(void);

#ifdef __cplusplus
}
#endif


#endif	/*PDW_RS232_H*/
