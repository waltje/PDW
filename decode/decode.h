#ifndef PDW_DECODE_H
# define PDW_DECODE_H


#ifdef __cplusplus
extern "C" {
#endif

extern double ct1600, ct3200, ct_bit;

#ifdef __cplusplus
}
#endif


extern FILE *pd_raw_fp;
extern FILTER filter;

// Statistics - globals
extern unsigned long	hourly_stat[NUM_STAT][2],
			hourly_char[NUM_STAT][2];
extern unsigned long	daily_stat[NUM_STAT][2],
			daily_char[NUM_STAT][2];

extern SYSTEMTIME	statTime,
			prev_statTime;
extern int		stat_timer;

extern char ob[32];
extern double rcver[65];         // rcv clock error ring buf
extern double exc;
extern int ircver;
extern int g_vt;

// receive symbol clock tightness - lower numbers mean slower loop response
extern double rcv_clkt;         // rcv loop clock tightness
extern double rcv_clkt_hi;      // coarse rcv clock (sync ups only)
extern double rcv_clkt_fl;      // fine rcv clock (data) - FLEX
extern double rcv_clkt_po;      // fine rcv clock (data) - POCSAG
extern double rcv_clkt_mb;      // fine rcv clock (data) - Mobitex


// this table translates received modem status line combinations into
// the received symbol; it gives the number of modem status lines that
// are high (other than RI) for a given line status nybble
extern int rcv[16]; // { 0, 1, 1, 2, 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3 };


// These hold or track VxD input data from comport
extern unsigned long  bufsize;
extern unsigned long  *cpstn;
extern unsigned short int *freqdata;
extern unsigned char  *linedata;

extern char  time_str[41], date_str[41], temp_name[41], out_str[256];
extern int    out_len, prev_timeout;
extern time_t curr_time;

// RAH; record and playback globals
extern int bPlayback, bRecording, bAutoRecording;
extern FILE *pd_rec;

//********* Prototypes. ****************************
void pd_reset_all(void);
void pdw_decode_acars(void);
void pdw_decode_mobitex(void);
void pdw_decode_ermes(void);
void pdw_decode(void);
void check_save_data(void);

// RAH: record and playback
int Open_Recording(OPENFILENAME *pofn, LPTSTR lpstrFile, int bOpennotsave);
int Start_Playback(LPTSTR lpstrFile);
void Stop_Playback(void);
void Start_Recording(LPTSTR lpstrFile);
void Stop_Recording(void);
void pdw_playback(void);
void pdw_record(void);


#endif	/*PDW_DECODE_H*/
