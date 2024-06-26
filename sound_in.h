#ifndef PDW_SOUNDIN_H
# define PDW_SOUNDIN_H


#define DFLT_HI_AUDIO	1
#define DFLT_LO_AUDIO	0

#define INDEX512	0
#define INDEX1200	1
#define INDEX2400	2
#define INDEX1600	3
#define INDEX3200	4


#ifdef __cplusplus
extern "C" {
#endif

extern int bCapturing;
extern char high_audio;
extern char low_audio;
extern long BaudRate;
extern int  cross_over;

void CALLBACK Callback_Function(HWAVEIN hwi, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2);
void Process_ReadyBuffers(HWND hwnd);
void free_audio_buffers(void);
int Stop_Capturing(void);
int Start_Capturing(void);

void Audio_To_Bits  (char *lpAudioBuffer, long LenAudioBuffer);
void MOBITEX_To_Bits(char *lpAudioBuffer, long LenAudioBuffer);
void ACARS_To_Bits  (char *lpAudioBuffer, long LenAudioBuffer);
void ERMES_To_Bits  (char *lpAudioBuffer, long LenAudioBuffer); // PH: test

void Reset_ATB(void);
void SetAudioConfig(int sac_type);
int Get_Percent(int x, int percent);

#ifdef __cplusplus
}
#endif


#endif	/*PDW_SOUNDIN_H*/
