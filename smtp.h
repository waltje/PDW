#ifndef PDW_SMTP_H
# define PDW_SMTP_H


#define MAIL_OPTION_MODE_ALL		0x000000
#define MAIL_OPTION_MODE_FILTER		0x000001
#define MAIL_OPTION_MODE_MONITOR	0x000002
#define MAIL_OPTION_MODE_SELECTABLE	0x000003
#define MAIL_OPTION_MODES		0x00000F

#define MAIL_OPTION_ADDRESS		0x000010
#define MAIL_OPTION_TIME		0x000020
#define MAIL_OPTION_DATE		0x000040
#define MAIL_OPTION_MODE		0x000080

#define MAIL_OPTION_TYPE		0x000100
#define MAIL_OPTION_BITRATE		0x000200
#define MAIL_OPTION_MESSAGE		0x000400
#define MAIL_OPTION_LABEL		0x000800

#define MAIL_OPTION_SUBJECT		0x001000
#define MAIL_OPTION_MSG 		0x002000
#define MAIL_OPTION_AUTH		0x004000
#define MAIL_OPTION_ENABLE		0x008000

#define MAIL_OPTION_US_ASCII		0x010000
#define MAIL_OPTION_ISO_8859_1		0x020000
#define MAIL_OPTION_ISO_8859_2 		0x030000
#define MAIL_OPTION_ISO_8859_3 		0x040000
#define MAIL_OPTION_ISO_8859_4 		0x050000
#define MAIL_OPTION_ISO_8859_5 		0x060000
#define MAIL_OPTION_ISO_8859_6 		0x070000
#define MAIL_OPTION_ISO_8859_7 		0x080000
#define MAIL_OPTION_ISO_8859_8 		0x090000
#define MAIL_OPTION_ISO_8859_9 		0x0A0000
#define MAIL_OPTION_ISO_8859_10		0x0B0000
#define MAIL_OPTION_ISO_2022_KR		0x0C0000
#define MAIL_OPTION_KOI8_R     		0x0D0000
#define MAIL_OPTION_EUC_KR     		0x0E0000
#define MAIL_OPTION_SHIFT_JIS  		0x0F0000
#define MAIL_OPTION_ISO_2022_JP		0x100000
#define MAIL_OPTION_EUC_JP     		0x110000
#define MAIL_OPTION_GB2312     		0x120000
#define MAIL_OPTION_BIG5       		0x130000
#define MAIL_OPTION_WINDOWS_1250	0x140000
#define MAIL_OPTION_WINDOWS_1251	0x150000
#define MAIL_OPTION_WINDOWS_1252	0x160000
#define MAIL_OPTION_WINDOWS_1253	0x170000
#define MAIL_OPTION_WINDOWS_1254	0x180000
#define MAIL_OPTION_WINDOWS_1255	0x190000
#define MAIL_OPTION_WINDOWS_1256	0x1A0000
#define MAIL_OPTION_WINDOWS_1257	0x1B0000
#define MAIL_OPTION_WINDOWS_1258	0x1C0000

#ifdef USE_SSL
# define MAIL_OPTION_SSL		0x200000
#endif

#define MAX_SMTP_CHARSETS   28


#ifdef __cplusplus
extern "C" {
#endif

extern int nSMTPsessions;
extern int nSMTPemails;
extern int nSMTPerrors;
extern int iSMTPlastError;

extern const char *szSmtpCharSets[];


int MailInit(char *szMailHost, char *szMailHeloDomain, char *szMailFrom, char *szMailTo, char *szMailUser, char *szMailPassword, int iMailPort, int nOptions) ;
int SendMail(HWND hResponse, int bMatch, int bMonitor_only, int iSeparateSMTP, char *sz1, char *sz2, char *sz3, char *sz4, char *sz5, char *sz6, char *sz7, char *szLabel) ;

#ifdef __cplusplus
}
#endif


#endif	/*PDW_SMTP_H*/
