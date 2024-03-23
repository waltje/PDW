#ifndef PDW_SIGIND_H
# define PDW_SIGIND_H

#ifdef __cplusplus
extern "C" {
#endif

void FreeSigInd(void);
int LoadSigInd(HINSTANCE hThisInstance);
void DrawSigInd(HWND hwnd);
void UpdateSigInd(int direction_flg);

#ifdef __cplusplus
}
#endif


#endif	/*PDW_SIGIND_H*/
