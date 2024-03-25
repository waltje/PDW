#ifndef PDW_H
# define PDW_H


#define FILTER_CAPCODE_LEN 9	// longest is FLEX long (9 chars)
#define FILTER_LABEL_LEN 70	// PH: was 40
#define FILTER_TEXT_LEN	40	// PH: was 25
#define FILTER_FILE_LEN	128	// PH: was 256

#define MAX_STR_LEN	5120

#define MAX_FILE_LEN	256

#define MAIL_TEXT_LEN	100

#define LINE_SIZE	180	// PH Test : was 160 (problem Colin)

/* Used in SetWindowPaneSize(int panes) */
#define WINDOW_SIZE	0
#define PANE_SIZES	1
#define PANE_SWITCH	2

/* Used in DrawPaneLabels(HWND hwnd, int pane) */
#define PANE1		0x01
#define PANE2		0x02
#define PANERXQUAL	0x04

#define MOBITEX_SHOW_MPAK	0x01
#define MOBITEX_SHOW_TEXT	0x02
#define MOBITEX_SHOW_HPDATA	0x04
#define MOBITEX_SHOW_DATA	0x08
#define MOBITEX_VERBOSE		0x10
#define MOBITEX_SHOW_HPID	0x20
#define MOBITEX_SHOW_SWEEP	0x40

// COLORS
#define COLOR_UNUSED		0
#define COLOR_BACKGROUND	1
#define COLOR_ADDRESS		2
#define COLOR_TIMESTAMP		3
#define COLOR_MODETYPEBIT	4
#define COLOR_MESSAGE		5
#define COLOR_NUMERIC		6
#define COLOR_MISC		7
#define COLOR_BITERRORS		8
#define COLOR_FILTERMATCH	9
#define COLOR_INSTRUCTIONS	10
#define COLOR_AC_MESSAGE_NR	11
#define COLOR_AC_DBI		12
#define COLOR_MB_SENDER		13

/* LABEL COLORS (21-36 are reserved for the filter label colors) */
#define COLOR_FILTERLABEL	20


/* ID for PDW sytem tray icon. */
#define SYSTEMTRAY_ICON_MESSAGE (WM_USER+1)


typedef enum {
    UNUSED_FILTER = 0,
    FLEX_FILTER,
    POCSAG_FILTER,
    TEXT_FILTER,
    ERMES_FILTER,
    ACARS_FILTER,
    MOBITEX_FILTER
} FILTER_TYPE;

typedef struct {
    FILTER_TYPE	type;
    char	capcode[FILTER_CAPCODE_LEN+1];
    char	label[FILTER_LABEL_LEN+1];
    char	text[FILTER_TEXT_LEN+1];
    int		match_exact_msg;
    int		cmd_enabled;
    int		reject;
    int		monitor_only;
    int		wave_number;
    int		label_enabled;
    int		label_color;
    int		smtp;
    int		sep_filterfile_en;
    char	sep_filterfile[3][FILTER_FILE_LEN+1];	// PH: max 3 sepfiles
    int		sep_filterfiles;			// PH: #sepfiles
    unsigned	hitcounter;
    char	lasthit_date[10];
    char	lasthit_time[10];
} FILTER;

#ifdef __cplusplus
# include <cstring>
# pragma warning(disable : 4786)
# include <vector>
# pragma warning(default : 4786)
# include <algorithm>

using namespace std;

typedef vector<FILTER> FILTERLIST;
#endif

typedef struct {
    int xPos;			// application window's horizontal position
    int yPos;			// application window's vertical position
    int xSize;			// application window's horizontal size
    int ySize;			// application window's vertical size
    unsigned confirmExit;	// confirm exit flag
    unsigned showtone;		// show tone-only messages flag
    unsigned shownumeric;	// show numeric messages flag
    unsigned showmisc;		// show miscellaneous messages flag
    unsigned filterbeep;	// beep when filter is triggered
    unsigned filterwindowonly;	// show filtered messages only in window
    unsigned decodepocsag;	// flag to enable POCSAG decoding
    unsigned decodeflex;	// flag to enable FLEX/ReFLEX decoding
    unsigned showinstr;		// flag to show Short Instructions
    unsigned convert_si;	// flag to convert Short Instructions to textmessage
    unsigned pocsag_512;	// show pocsag-512  flag
    unsigned pocsag_1200;	// show pocsag-1200 flag
    unsigned pocsag_2400;	// show pocsag-2400 flag
    unsigned pocsag_fnu;	// display pocsag function numbers as default (NTTA)
    unsigned pocsag_showboth;	// show "difficult" messages as numeric AND alpha
    unsigned flex_1600;		// show flex-1600 flag
    unsigned flex_3200;		// show flex-3200 flag
    unsigned flex_6400;		// show flex-6400 flag
    unsigned acars_parity_check;
    unsigned show_cfs;		// flag to show FLEX cycle/frame information in titlebar
    unsigned show_rejectblocked;// flag to show rejected/blocked messages in titlebar
    
    int Hide_Column;
    int LabelLog;		// labels in normal logfile
    int LabelNewline;		// labels on new line
    char ColLogfile[10];	// flag for columns to be logged in logfile
    char ColFilterfile[10];	// flag for columns to be logged in filterfile
    int Linefeed;		// flag for converting ¯ to linefeed
    int Separator;		// flag for separating messages (empty line)
    int MonthNumber;		// flag for using monthnumber in logfilenames
    int DateFormat;		// flag for date format
    int Date_USA;		// flag for using MM-DD-YY in stead of DD-MM-YY
    char LogfilePath[MAX_PATH];	// flag for using monthnumber in logfilenames
    int BlockDuplicate;		// flag for blocking duplicate messages
    int FilterWindowColors;	// flag for showing label colors in filterwindow
    int FilterWindowExtra;	// flag for showing CMD/DESC/SEP/etc in filterwindow
    
    int SystemTray;		// flag for enabeling the system tray
    int SystemTrayRestore;	// flag for enabeling auto restore from tray
    
    int FlexTIME;		// flag for FlexTIME as systemtime
    int FlexGroupMode;		// flag for FlexGroupMode
    
    int SMTP;			// SMTP-email
    
#if 0	// FIXME: NICO
    int MESSAGE_QUEUE = 1;
    int mqPort = 5672;
    char mqHostName[100] = "localhost";
    char mqUsername[100] = "internal";
    char mqPassword[100] = "iseeu5448!";
    char mqVirtualHost[100] = "private";
    char mqRoutingKey[100] = "pdw-pager";
    char mqTopic[100] = "webnotify";
#endif	// END NICO

    int Trayed;			// TRUE if trayed

    int comPortEnabled;
    int comPort;		// COM port (1-4 or 5 for custom)
    int comPortRS232;		// Special serial converter from Rene
    int comRS232bitrate;
    int comPortAddr;		// COM port I/O address
    int comPortIRQ;		// COM port IRQ vector
    int fourlevel;		// use 4 Level FSK interface flag
    int invert;			// invert COM port/Sound card data
    int invert_option;		// 0=auto,1=Yes,2=No

    int percent;		// percent of total main window area

    double dRX_Quality;

    LOGFONT fontInfo;

    int logfile_enabled;
    char logfile[MAX_FILE_LEN+1];
    int logfile_use_date;

    int filterfile_enabled;
    char filterfile[MAX_FILE_LEN+1];
    int filterfile_use_date;
    int filter_cmd_file_enabled;
    char filter_cmd[MAX_FILE_LEN+1];
    char filter_cmd_args[MAX_FILE_LEN+1];
    int filter_default_type;
    int filter_searchwhiletyping;

    char szMailHost[MAIL_TEXT_LEN];
    char szMailHeloDomain[MAIL_TEXT_LEN];
    char szMailTo[MAIL_TEXT_LEN * 5];
    char szMailFrom[MAIL_TEXT_LEN];
    char szMailUser[MAIL_TEXT_LEN];
    char szMailPassword[MAIL_TEXT_LEN];
    int iMailPort;
    int nMailOptions;

    COLORREF color_background;
    COLORREF color_address;
    COLORREF color_timestamp;
    COLORREF color_modetypebit;
    COLORREF color_numeric;
    COLORREF color_message;
    COLORREF color_misc;
    COLORREF color_biterrors;
    COLORREF color_filtermatch;
    COLORREF color_instructions;
    COLORREF color_ac_message_nr;
    COLORREF color_ac_dbi;
    COLORREF color_mb_sender;
    COLORREF color_filterlabel[17];	// array for all label colors

    int pane1_size;
    int pane2_size;
    int ScrollSpeed;
    int ScreenColumns[7];

    int stat_file_enabled;
    int stat_file_use_date;
    char stat_file[MAX_FILE_LEN+1];
    char edit_save_file[MAX_FILE_LEN+1];
    int maximize_flg;
    int maximize_tmp;
    int minimize_flg;
    int audioEnabled;
    int audioDevice;
    int audioChannels;
    int audioSampleRate;
    int audioConfig;
    int audioThreshold[4];
    int audioResync[4];
    int audioCentering[4];

    int monitor_paging;
    int monitor_acars;
    int monitor_mobitex;
    int monitor_ermes;
    int reverse_msg;
    int lang_mi_index;		// decides language menu item.
    int lang_tbl_index;		// decides language character map.
    int ssl;

#ifdef __cplusplus
    FILTERLIST filters;		// only if compiling a C++ module
#endif
} PROFILE, *PPROFILE;

typedef struct {
    HWND	hWnd;
    unsigned	cyLines, cxClient, cyClient;
    int		iVscrollPos, iVscrollMax, iHscrollPos, iHscrollMax;
    unsigned	Bottom;
    int		currentPos;
    BYTE	currentColor;
    unsigned	buff_lines;
    char	*buff_char;
    BYTE	*buff_color;
} PaneStruct;

#define MODE_IDLE    0x00

#define MODE_REFLEX  0x10
#define MODE_POCSAG  0x20
#define MODE_ERMES   0x30
#define MODE_MOBITEX 0x40
#define MODE_ACARS   0x80
#define MODE_PAGING  0xF0

#define MODE_P512    0x01
#define MODE_P1200   0x02
#define MODE_P2400   0x04

#define MODE_FLEX_A  0x08
#define MODE_FLEX_B  0x04
#define MODE_FLEX_C  0x02
#define MODE_FLEX_D  0x01

#define STAT_FLEX6400    0
#define STAT_FLEX3200    1
#define STAT_FLEX1600    2
#define STAT_POCSAG2400  3
#define STAT_POCSAG1200  4
#define STAT_POCSAG512   5
#define STAT_ACARS2400   6
#define STAT_MOBITEX     7
#define STAT_ERMES       8
#define NUM_STAT         9

#define STAT_NUMERIC     0
#define STAT_ALPHA       1


#ifdef __cplusplus
extern "C" {
#endif

// ***** Globals ******
extern HINSTANCE ghInstance;		// current instance
extern HWND ghWnd;			// main window handle
extern HWND hToolbar;			// Toolbar handle
extern HACCEL ghAccel;
extern HMENU ghMenu; 
extern TCHAR szAppName[128];		// name of the application (PH: was 64)
extern TCHAR szShortAppName[32];	// short name of the application
extern TCHAR szApiFailedMsg[64];	// "A Windows API Failed" message

extern char gszPDWClass[15];
extern char gszPane1Class[17];
extern char gszPane2Class[17];
//extern char gszPane2LabelClass[22];
extern char gszColorClass[17];
extern char gszACARSColorClass[19];
extern char gszMOBITEXColorClass[21];
extern char gszERMESColorClass[19];

extern LPCTSTR lpszSourceFileName;

extern TCHAR szDialogErrorMsg[80];
extern TCHAR szCenterOpenDlgMsg[80];

extern TCHAR szPath[MAX_PATH];          // path where the running application resides
extern TCHAR szLogPathName[MAX_PATH];   // Logfile-path
extern TCHAR szWavePathName[MAX_PATH];  // Wavefile-path
extern TCHAR szExePathName[MAX_PATH];   // full pathname of the application
extern TCHAR szHelpPathName[MAX_PATH];  // full pathname of the application's help file
extern TCHAR szIniPathName[MAX_PATH];   // full pathname of the application's ini file
extern TCHAR szFilterPathName[MAX_PATH];// full pathname of the application's filter file
extern TCHAR szFilterBackup[MAX_PATH];  // full pathname of the application's filter backupfile
extern TCHAR szVolPathName[MAX_PATH];   // full pathname to the PDW volume control.

extern unsigned int cxChar, cyChar, cxCaps;
extern int iMaxWidth;
extern int sizeSet;
extern int pane1Height, pane2Height, pane1Pos, pane2Pos, pane1Top;
extern PaneStruct Pane1;
extern PaneStruct Pane2;

extern PROFILE Profile;			// profile information

// Text editing globals
extern unsigned int start_col, start_row, end_col, end_row;
extern int select_on, selected, selecting;
extern PaneStruct *select_pane;

extern char *label_colors[9];		// PH: Colors for filter labels
extern char *wave_files[11];
extern char szWindowText[6][1000];

extern char aNumeric[17];

extern int g_sps;
extern int g_sps2;

extern int bUpdateFilters;		// PH: Does FILTERS.INI need to be updated?

extern FILE *pLogFile;			// Logged messages
extern FILE *pFilterFile;		// Filtered messages
extern FILE *pSepFilterFile;		// Separate filtered messages
extern FILE *pStatFile;			// Statistics
extern FILE *pClipboardFile;
extern FILE *pFiltersFile;		// PH: Filter.ini

extern double dRX_Quality;
extern int bTrayed, bDoubleDisplay;


// ************* Function Prototypes **************
int InitApplication(HINSTANCE hInstance);
HWND InitInstance(HINSTANCE hInstance, int nCmdShow);
void InitializePane(PaneStruct *pane);

int Need_Ext(char *s);
void Free_Common_Objects(void);
void PrintClip(void);
void display_color(PaneStruct *pane, BYTE ct);
void display_show_char(PaneStruct *pane, char cin);
void display_show_str(PaneStruct *pane, char strin[]);
void ShowMessage();
void display_show_timestamp(PaneStruct *pane);
void display_showmo(int mode);
void display_show_hex21(PaneStruct *pane, long int l);
void display_show_hex16(PaneStruct *pane, int l);
void setupecc(void);
int  nOnes(int k);
int  bit10(int gin);
int  ecd(void);
int  filter_addr(char addr_str[], char filter_str[]);


LRESULT CALLBACK PDWWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK Pane1WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK Pane2WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int GetEditSaveName(HWND hWnd);
void ClearPanes(int bPane1, int bPane2);
void InvertSelection(void);
void SaveClipToDisk(char *fname);
void CopyToClipboard(PaneStruct *pane, UINT min_col, UINT max_col, UINT min_row, UINT max_row);
void PanePaint(PaneStruct *pane);
void PaneHScroll(PaneStruct *pane, WPARAM wParam);
void PaneVScroll(PaneStruct *pane, WPARAM wParam, LPARAM lParam);
void BuildFilterString(char *temp_str, FILTER filter);
void ChangeDataMode(HWND hWnd, int mode);

void GoModalDialogBoxParam(HINSTANCE hInstance, LPCSTR lpszTemplate, HWND hWnd, DLGPROC lpDlgProc, LPARAM lParam);
UINT CALLBACK CenterOpenDlgBox(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
int CenterWindow(HWND hWnd);

INT_PTR AboutDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR ClearScreenDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR DebugDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR LogFileDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CustomAudioDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR SetupDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR SystemTrayDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int SelectFont(HWND hDlg);

INT_PTR ColorsDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ColorWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR ACARSColorsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);                                
LRESULT CALLBACK ACARSColorWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR MOBITEXColorsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK MOBITEXColorWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR ERMESColorsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ERMESColorWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR OptionsDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR GeneralOptionsDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR ScreenOptionsDlgProc(HWND hWnd, UINT uMsg,	WPARAM wParam, LPARAM lParam);
INT_PTR ScrollDlgProc(HWND hWnd, UINT uMsg,	WPARAM wParam, LPARAM lParam);
INT_PTR FilterDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR FilterEditDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR FilterOptionsDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR FilterFindDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR FilterCheckDuplicateDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR MonStatDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR MailDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
int SetTitle(HWND hWnd, TCHAR *cTitle);

int ErrorMessageBox(LPCTSTR lpszText, LPCTSTR lpszTitle, LPCTSTR lpszFile, INT Line);
int GetPrivateProfileSettings(LPCTSTR lpszAppTitle, LPCTSTR lpszIniPathName, PPROFILE pProfile);
void WriteSettings(void);
void WriteFilters(PPROFILE pProfile, int backup);
int ReadFilters(char *szFilters, PPROFILE pProfile, int bNew);

int LoadDriver(void);
void UnloadDriver(void);

void WriteStatFileHourly(FILE *fp);
void WriteStatFileDaily(FILE *fp);
void CreateDateFilename(char *ext, SYSTEMTIME *yesterday);
void SetWindowPaneSize(int param);
void SetNewWindowText(char* text);
void SystemTrayWindow(int bHideWindow);
void SystemTrayIcon(int bRemoveIcon);

#ifdef __cplusplus
}
#endif


#endif	/*PDW_H*/
