#ifndef PDW_MENU_H
# define PDW_MENU_H


#ifdef __cplusplus
extern "C" {
#endif

void check_menu_item(int submenu_no, UINT item_id, int item_checked);
void set_menu_items(void);
void set_lang_menu(void);

#ifdef __cplusplus
}
#endif


#endif	/*PDW_MENU_H*/
