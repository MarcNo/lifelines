/* 
   menuitem.h
   Copyright (c) 1999-2002 Perry Rapp
   "The MIT license"
   Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
/*
 menuitem.h - declarations for menuset, dynmenu, brwsmenu, & all command constants
  Created in 1999/03 private build of LifeLines
  Added to repository during 3.0.6 development
*/

#ifndef _MENUITEM_H
#define _MENUITEM_H

#define MAIN_SCREEN        1
#define ONE_PER_SCREEN     2
#define ONE_FAM_SCREEN     3
#define TWO_PER_SCREEN     4
#define TWO_FAM_SCREEN     5
#define LIST_SCREEN        7
#define AUX_SCREEN         8
/* must be at bottom of list */
#define MAX_SCREEN         8


/*
  Each menu item has display text and selection character(s)
  If Choices is 0 (as it is for all as of 2002.01), then the
  first characters of Display up to whitespace are used for the
  choice characters, eg, "f  Browse to fathers"
  NB: The direct-to-child item is specially coded to use digits 1-9,
  and does not use the Choices string.
*/
typedef struct MenuItem_s {
	STRING Display;
	STRING Choices;
	INT Command;
	STRING LocalizedDisplay;
} MenuItem;
/*
A LongDisplay could be added (for, eg, status bar,
or for some type of extended command info display).
- Perry Rapp, 2001/02/10
*/
/*
typedef struct MenuItemOption_struct {
	STRING Display1;
	STRING Display2;
	STRING Choices;
	INT Command;
} MenuItemOption;
*/


/* special menu items added on the fly */
extern MenuItem g_MenuItemOther, g_MenuItemQuit;
/* this is for navigating top & bottom simultaneously
in tandem screens, and is not implemented in this version! */
extern MenuItem f_MenuItemSyncMoves;

typedef struct tag_cmdarray * CMDARRAY;

/* One set of menus */
/* This is the dynamically resizable menu system */
/* Currently used only by browse screens, not list screens or main menus */
/* need to move most of this into layout structure, & out of here, as it is only for curses */
struct tag_menuset {
	CMDARRAY Commands;
	MenuItem ** items;  /* array of pointers to items */
	MenuItem ** extraItems;  /* array of pointers to extra items */
};
typedef struct tag_menuset *MENUSET;



/* dynamically resizing & pageable menu */
struct tag_dynmenu {
	struct tag_menuset menuset;
	INT rows;      /* height of menu (at start) */
	INT cols;      /* (menu) columns in this menu (3 for big, 1 for list) */
	INT size;      /* total #items in this menu */
	INT page;      /* which page of menu currently displayed */
	INT pages;     /* # of pages total */
	INT pageitems; /* # of items per page */
	INT mincols;   /* minimum width in colums*/
	INT maxcols;   /* maximum width in columns */
	INT minrows;   /* minimum height */
	INT maxrows;   /* maximum height */
	INT hidden;    /* for hideable menus */
	INT dirty;     /* for repainting code */
	/* character coordinates of menu size & location */
	INT top;
	INT bottom;
	INT left;
	INT width;
	INT cur_y;     /* row for input cursor */
	INT cur_x;     /* col for input cursor */
};
typedef struct tag_dynmenu *DYNMENU;


/*
global array of menu information, produced by menuitem.c
and used by both screen.c and menuitem.c
*/

/* menuset.c */
void menuset_init(MENUSET menu, STRING title, MenuItem ** MenuItems, MenuItem ** extraItems);
INT menuset_check_cmd(MENUSET menuset, STRING str);
void menuset_clear(MENUSET menuset);
MenuItem ** menuset_get_items(MENUSET menuset);

/* dynmenu.c */
void dynmenu_adjust_height(DYNMENU dynmenu, INT delta);
void dynmenu_adjust_menu_cols(DYNMENU dynmenu, INT delta);
void dynmenu_clear(DYNMENU dynmenu);
MENUSET dynmenu_get_menuset(DYNMENU dynmenu);
void dynmenu_init(DYNMENU dynmenu , STRING title, INT MenuRows, INT MenuCols
	, INT MinCols, INT MaxCols
	, INT MinRows, INT MaxRows
	, INT MenuTop, INT MenuLeft, INT MenuWidth
	, INT MenuSize, MenuItem ** MenuItems);
void dynmenu_next_page(DYNMENU dynmenu);
void dynmenu_toggle_menu(DYNMENU dynmenu);

/* brwsmenu.c */
MENUSET get_screen_menuset(INT screen);
DYNMENU get_screen_dynmenu(INT screen);
STRING get_screen_title(INT screen);
void brwsmenu_initialize(INT screenheight, INT screenwidth);


void menuitem_initialize(INT cols);
void menuitem_terminate(void);
INT menuitem_check_cmd(INT screen, STRING cmd);

enum { 
	CMD_NONE /* unrecognized or unimplemented */
	, CMD_PARTIAL /* part of a multichar sequence */
	, CMD_QUIT, CMD_MENU_MORE, CMD_MENU_TOGGLE
	, CMD_MENU_GROW, CMD_MENU_SHRINK, CMD_MENU_MORECOLS, CMD_MENU_LESSCOLS
	, CMD_EDIT, CMD_PERSON, CMD_FATHER
	, CMD_MOTHER, CMD_SPOUSE, CMD_CHILDREN, CMD_UPSIB, CMD_DOWNSIB
	, CMD_FAMILY, CMD_PARENTS, CMD_BROWSE
	, CMD_TOP, CMD_BOTTOM, CMD_ADDASSPOUSE, CMD_ADDASCHILD
	, CMD_ADDSPOUSE, CMD_ADDCHILD, CMD_ADDFAMILY, CMD_PEDIGREE
	, CMD_SWAPFAMILIES, CMD_SWAPCHILDREN, CMD_SWAPTOPBOTTOM
	, CMD_REORDERCHILD
	, CMD_NEWPERSON, CMD_NEWFAMILY, CMD_TANDEM
	, CMD_REMOVEASSPOUSE, CMD_REMOVEASCHILD, CMD_REMOVESPOUSE, CMD_REMOVECHILD
	, CMD_SOURCES, CMD_NOTES, CMD_POINTERS
/*	, CMD_SHOWSOURCES, CMD_HIDESOURCES*/
	, CMD_SCROLL_UP, CMD_SCROLL_DOWN, CMD_DEPTH_UP, CMD_DEPTH_DOWN
	, CMD_NEXT, CMD_PREV, CMD_BROWSE_ZIP_INDI, CMD_BROWSE_ZIP_ANY
	, CMD_ADVANCED
	, CMD_SCROLL_TOP_UP, CMD_SCROLL_TOP_DOWN
	, CMD_SCROLL_BOTTOM_UP , CMD_SCROLL_BOTTOM_DOWN
	, CMD_SCROLL_BOTH_UP, CMD_SCROLL_BOTH_DOWN
	, CMD_MODE_GEDCOM, CMD_MODE_GEDCOMX, CMD_MODE_GEDCOMT
	, CMD_MODE_ANCESTORS , CMD_MODE_DESCENDANTS
	, CMD_MODE_NORMAL, CMD_MODE_CYCLE
	, CMD_MODE_PEDIGREE
	, CMD_TANDEM_CHILDREN, CMD_TANDEM_FATHERS, CMD_TANDEM_MOTHERS
	, CMD_TANDEM_SPOUSES, CMD_TANDEM_FAMILIES, CMD_TANDEM_PARENTS
	, CMD_TOGGLE_CHILDNUMS, CMD_TOGGLE_PEDTYPE
	, CMD_BROWSE_INDI, CMD_BROWSE_FAM
	/* reserve range for direct to children */
	, CMD_CHILD_DIRECT0, CMD_CHILD_DIRECT9=CMD_CHILD_DIRECT0+9
	, CMD_JUMP_HOOK
	, CMD_COPY_TOP_TO_BOTTOM, CMD_MERGE_BOTTOM_TO_TOP
	, CMD_BOTH_FATHERS, CMD_BOTH_MOTHERS
	, CMD_VHISTORY_BACK, CMD_VHISTORY_FWD, CMD_VHISTORY_LIST, CMD_VHISTORY_CLEAR
	, CMD_CHISTORY_BACK, CMD_CHISTORY_FWD, CMD_CHISTORY_LIST, CMD_CHISTORY_CLEAR
	, CMD_ADD_SOUR, CMD_ADD_EVEN, CMD_ADD_OTHR
	/* for hardware keys */
	, CMD_KY_UP=500, CMD_KY_DN
	, CMD_KY_SHPGUP, CMD_KY_SHPGDN, CMD_KY_PGUP, CMD_KY_PGDN
	, CMD_KY_HOME, CMD_KY_END
	, CMD_KY_ENTER
};



#endif /* _MENUITEM_H */
