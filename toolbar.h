#ifndef PDW_TOOLBAR_H
# define PDW_TOOLBAR_H


#ifdef __cplusplus
extern "C" {
#endif

HWND ShowMakeToolBar(HWND parent_hwnd, HINSTANCE hThisInstance);
BOOL GetToolBarImages(HINSTANCE hThisInstance);
void FreeToolBarImages(HINSTANCE hThisInstance);
void SetToolBarButtons(void);
void Add_TB_ButtonsBitmaps(HWND tbar_hwnd, HINSTANCE hThisInstance);
void TB_AutoSize(HWND hTbar);
void SetToolTXT(HINSTANCE hThisInstance, LPARAM lParam);

#ifdef __cplusplus
}
#endif


#endif	/*PDW_TOOLBAR_H*/
