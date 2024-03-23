// menu.c
//
// Menu routines.
//
//            - Routines for adjusting the Monitor menu.
//            - Routines for adjusting the language menu.
//            - Routines which set the current language table.
//
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "resource.h"
#include "pdw.h"
#include "language.h"
#include "menu.h"


// Check/uncheck the specified menu item.
// submenu_no = menu (position)containing the item to check/uncheck.
// item_id = identifier of the menu item.
// item_checked = whether to check or uncheck the item.
void
check_menu_item(int submenu_no, UINT item_id, int item_checked)
{
    MENUITEMINFO mii;
    HMENU hmenu, hsubmenu;   

    hmenu = GetMenu(ghWnd);
    hsubmenu = GetSubMenu(hmenu, submenu_no);         

    mii.cbSize = sizeof(MENUITEMINFO);  
    mii.fMask = MIIM_CHECKMARKS | MIIM_STATE;
    mii.fType = 0; 
    mii.fState = item_checked ? MFS_CHECKED : MFS_UNCHECKED; 
    mii.wID = 0; 
    mii.hSubMenu = NULL; 
    mii.hbmpChecked = NULL; 
    mii.hbmpUnchecked = NULL; 
    mii.dwItemData = 0; 
    mii.dwTypeData = 0; 
    mii.cch = 0; 

    SetMenuItemInfo(hsubmenu, item_id, FALSE, &mii);
}


// Checks/Unchecks menu items based on profile settings.
void
set_menu_items(void)
{
    static int prev_lang = -1;

    check_menu_item(6, IDM_POCSAGFLEX, FALSE);
    check_menu_item(6, IDM_ACARS, FALSE);
    check_menu_item(6, IDM_MOBITEX, FALSE);
    check_menu_item(6, IDM_ERMES, FALSE);

    if (Profile.monitor_paging)		// Check POCSAG/FLEX... menu item
	check_menu_item(6, IDM_POCSAGFLEX, TRUE);
    else if (Profile.monitor_acars)	// Check ACARS... menu item
		check_menu_item(6, IDM_ACARS, TRUE);
    else if (Profile.monitor_mobitex)	// Check Mobitex... menu item
		check_menu_item(6, IDM_MOBITEX, TRUE);
    else if (Profile.monitor_ermes)	// Check Ermes... menu item
		check_menu_item(6, IDM_ERMES, TRUE);
    check_menu_item(4, IDM_FILTERFILE_EN, Profile.filterfile_enabled);
    check_menu_item(4, IDM_FILTERCOMMANDFILE, Profile.filter_cmd_file_enabled);

    // Check correct Langauge item.
    // Also set the current language table.
    if (Profile.lang_mi_index > lang_cnt)
	Profile.lang_mi_index = 0;
    Profile.reverse_msg = FALSE;
    Profile.lang_tbl_index = 0;
    if (Profile.lang_mi_index) {
	Profile.lang_tbl_index = (Profile.lang_mi_index-1);
	Profile.reverse_msg = lang_rev[Profile.lang_tbl_index];
    }

    if (prev_lang != Profile.lang_mi_index) {
	if (prev_lang != -1)  // Uncheck previous selection
		check_menu_item(7, (IDM_ENGLISH+prev_lang), FALSE);

	// Check new item
	check_menu_item(7, (IDM_ENGLISH+Profile.lang_mi_index), TRUE);

	prev_lang = Profile.lang_mi_index;
    }
}


// Add required language menu items.
void
set_lang_menu(void)
{
    MENUITEMINFO mii;
    HMENU hmenu, hsubmenu;
    int count, i, id = 1;

    /* Get handle of main menu. */
    hmenu = GetMenu(ghWnd);

    /* Get handle of 1st popup menu. */
    hsubmenu = GetSubMenu(hmenu, 7); 

    /* Get number of items in the popup. */
    count = GetMenuItemCount(hsubmenu);
   
    for (i = 0; i < lang_cnt; i++) {
	/* Append new menu item. */
	mii.cbSize = sizeof(MENUITEMINFO);
	mii.fMask = MIIM_TYPE | MIIM_ID;
	mii.fType = MFT_STRING;
	mii.fState = 0;
	mii.wID = IDM_ENGLISH + id;
	mii.hSubMenu = NULL;
	mii.hbmpChecked = NULL;
	mii.hbmpUnchecked = NULL;
	mii.dwItemData = 0;
	mii.dwTypeData = lang_name[i];
	InsertMenuItem(hsubmenu, count, 1, &mii);

	count++;
	id++;
    }   
}
