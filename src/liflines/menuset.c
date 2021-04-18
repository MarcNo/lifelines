/* 
   Copyright (c) 1999-2002 Perry Rapp
   "The MIT license"
   Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
/*=============================================================
 * menuset.c -- dynamic menus
 *  This module holds the code to construct a menu from strings
 *  and to search for a keystroke & return the corresponding command
 *   Created: 1999/03 by Perry Rapp
 *   Brought into repository: 2001/01/28 by Perry Rapp
 *==============================================================*/

#include "llstdlib.h"
#include "feedback.h"
#include "gedcom.h"
#include "menuitem.h"


/*********************************************
 * external/imported variables
 *********************************************/

extern STRING qSttlfambrw, qSttl2perbrw, qSttl2fambrw;
extern STRING qSttlauxbrw, qSttllstbrw;

/*********************************************
 * local types
 *********************************************/

struct tag_cmditem {
	uchar c;
	BOOLEAN direct; /* (T: command value, F: pointer) */
	UNION value; /* command value, or pointer to CommandArray */
};
typedef struct tag_cmditem * CMDITEM;

struct tag_cmdarray {
	INT alloc; /* size allocated */
	INT used; /* size in use */
	CMDITEM array;
};

/*********************************************
 * local function prototypes
 *********************************************/

/* alphabetical */
static void add_menu_item(CMDARRAY cmds, MenuItem * mitem);
static CMDARRAY create_cmd_array(INT alloc);
static void copy_cmditem(CMDITEM dest, CMDITEM src);
static BOOLEAN find_cmd(CMDARRAY cmds, uchar c, INT * pos);
static void free_cmds(CMDARRAY cmds);
static void get_menu_choice(STRING display, STRING choice, INT max);
static void grow_cmd_array(CMDARRAY cmds);
static void insert_cmd(CMDARRAY cmds, STRING str, INT cmdnum
	, STRING display);
static INT menuitem_find_cmd(CMDARRAY cmds, STRING cmd);

/*********************************************
 * local variables
 *********************************************/

static STRING f_current_title=0;

/*********************************************
 * local function definitions
 * body of module
 *********************************************/

/*============================
 * menuset_init - Load menu items into cmd array
 * Clears menuset and reloads it, so can be called with empty or full menuset
 * Created: 2001/01/28, Perry Rapp
 *==========================*/
void
menuset_init (MENUSET menuset, STRING title, MenuItem ** MenuItems, MenuItem ** extraItems)
{
	INT i;
	CMDARRAY cmds = create_cmd_array(32);
	f_current_title = title; /* for use in error messages */
	menuset_clear(menuset);
	menuset->Commands = cmds;
	menuset->items = MenuItems;
	menuset->extraItems = extraItems;
	for (i=0; MenuItems[i]; ++i)
		add_menu_item(cmds, MenuItems[i]);
	for (i=0; extraItems[i]; ++i)
		add_menu_item(cmds, extraItems[i]);
	f_current_title = 0;
}
/*============================
 * menuset_clear - Free memory in menuset
 * reentrant
 * Created: 2002/10/24, Perry Rapp
 *==========================*/
void
menuset_clear (MENUSET menuset)
{
	INT i;
	if (menuset->Commands) {
		free_cmds(menuset->Commands);
		menuset->Commands = 0;
	}
	if (menuset->items) {
		for (i=0; menuset->items[i]; ++i)
			strfree(&menuset->items[i]->LocalizedDisplay);
	}
	if (menuset->extraItems) {
		for (i=0; menuset->extraItems[i]; ++i)
			strfree(&menuset->extraItems[i]->LocalizedDisplay);
	}
}
/*============================
 * add_menu_item - add cmd for menu to cmdarray
 *  Title: [IN]  title of menu (only used for log msgs)
 *  cmds:  [I/O] cmdarray (tree used for command recognition)
 *  mitem: [IN]  new menu item to add to cmds
 * Created: 2002/01/24
 *==========================*/
static void
add_menu_item (CMDARRAY cmds, MenuItem * mitem)
{
	INT i;
	char display[32];

	/* localize string into current target language */
	llstrncpy(display, _(mitem->Display), ARRSIZE(display), uu8);
	if (mitem->LocalizedDisplay)
		strfree(&mitem->LocalizedDisplay);
	mitem->LocalizedDisplay = strsave(display);
	if (mitem->Command == CMD_CHILD_DIRECT0) {
		/* CMD_CHILD_DIRECT0 is always hooked up to digits */
		for (i=1; i<=9; i++) {
			char choice[FMT_INT_LEN+1];
			snprintf(choice, sizeof(choice), FMT_INT, i);
			insert_cmd(cmds, choice, CMD_CHILD_DIRECT0+i, display);
		}
	} else {
		char choice[9];
		if (mitem->Choices)
			strcpy(choice, mitem->Choices);
		else
			get_menu_choice(display, choice, sizeof(choice));
		/* add to nested menu arrays (stored by choice keys */
		insert_cmd(cmds, choice, mitem->Command, display);
	}
}
/*============================
 * get_menu_choice -- extract menu key sequence
 *  This must be first characters of display, ending with space
 * Created: 2001/12/23, Perry Rapp
 *==========================*/
/* This will work now, but it will break if we add arrows, PageUp, ... */
static void
get_menu_choice (STRING display, STRING choice, INT max)
{
	INT i;
	for (i=0; i<max && display[i] && display[i]!=' ' ; ++i) {
		choice[i] = display[i];
	}
	if (i == max) {
		msg_error(_("Menu (%s) choice sequence too long: %s"), f_current_title, display);
		FATAL();
	}
	if (display[i] != ' ') {
		msg_error(_("Menu (%s) item lacked choice sequence: %s"), f_current_title, display);
		FATAL();
	}
	choice[i]=0;
}
/*============================
 * create_cmd_array -- create an empty array of commands
 * Created: 2001/02/01, Perry Rapp
 *==========================*/
static CMDARRAY
create_cmd_array (INT alloc)
{
	CMDARRAY cmds = (CMDARRAY)stdalloc(sizeof(*cmds));
	cmds->alloc = alloc;
	cmds->used = 0;
	cmds->array = (CMDITEM)stdalloc(alloc * sizeof(cmds->array[0]));
	return cmds;
}
/*============================
 * grow_cmd_array -- grow an array of commands
 * Created: 2001/02/01, Perry Rapp
 *==========================*/
static void
grow_cmd_array (CMDARRAY cmds)
{
	INT alloc = cmds->alloc + cmds->alloc/2;
	CMDITEM old = cmds->array;
	INT i;
	cmds->alloc = alloc;
	cmds->array = (CMDITEM)stdalloc(alloc * sizeof(cmds->array[0]));
	for (i=0; i<cmds->used; i++)
		copy_cmditem(&cmds->array[i], &old[i]);
	stdfree(old);
}
/*============================
 * copy_cmditem -- copy a CmdItem_s struct
 * Created: 2001/02/01, Perry Rapp
 *==========================*/
static void
copy_cmditem (CMDITEM dest, CMDITEM src)
{
	dest->c = src->c;
	dest->direct = src->direct;
	dest->value = src->value;
}
/*============================
 * find_cmd -- search commands for command by character
 * Created: 2001/02/01, Perry Rapp
 *==========================*/
static BOOLEAN
find_cmd (CMDARRAY cmds, uchar c, INT * pos)
{
	INT lo=0, hi=cmds->used-1, i;
	while (lo<=hi) {
		i=(lo+hi)/2;
		if (cmds->array[i].c < c)
			lo=i+1;
		else if (cmds->array[i].c > c)
			hi=i-1;
		else {
			*pos = i;
			return TRUE;
		}
	}
	*pos = lo;
	return FALSE;
}
/*============================
 * insert_cmd -- add cmd to array (recursive)
 *  cmds:    [I/O] cmd tree or subtree to which we add
 *  str:     [IN]  remaining part of cmd hotkey sequence
 *  cmdnum:  [IN]  cmd code to store (eg, CMD_QUIT)
 *  display: [IN]  menu item text (for log msgs)
 * Created: 2001/02/01, Perry Rapp
 *==========================*/
static void
insert_cmd (CMDARRAY cmds, STRING str, INT cmdnum, STRING display)
{
	INT len = strlen(str);
	INT pos;
	uchar c = str[0];
	if (find_cmd(cmds, c, &pos)) {
		if (len==1) {
			crashlog(_("In menu: %s"), f_current_title);
			if (cmds->array[pos].direct) {
				crashlog(_("Duplicate hotkey for item: %s")
					, display);
			} else {
				crashlog(_("Clash with longer hotkey in item: %s")
					, display);
				
			}
		} else {
			/* multicharacter new cmd */
			if (cmds->array[pos].direct) {
				crashlog(_("In menu: %s"), f_current_title);
				crashlog(_("Clash with shorter hotkey in item: %s")
					, display);
			} else {
				CMDARRAY subarr = (CMDARRAY)cmds->array[pos].value.w;
				insert_cmd(subarr, &str[1], cmdnum, display);
			}
		}
	} else {
		INT i;
		if (cmds->used == cmds->alloc)
			grow_cmd_array(cmds);
		/* not found */
		for (i=cmds->used; i>pos; i--)
			copy_cmditem(&cmds->array[i], &cmds->array[i-1]);
		cmds->array[pos].c = c;
		if (len==1) {
			cmds->array[pos].direct = TRUE;
			cmds->array[pos].value.i = cmdnum;
		} else {
			/* multicharacter new cmd */
			CMDARRAY newcmds = create_cmd_array(8);
			cmds->array[pos].direct = FALSE;
			cmds->array[pos].value.w = newcmds;
			insert_cmd(newcmds, &str[1], cmdnum, display);
		}
		cmds->used++;
	}
}
/*============================
 * free_cmds -- free menu arrays
 * Created: 2001/02/01, Perry Rapp
 *==========================*/
static void
free_cmds (CMDARRAY cmds)
{
	INT i;
	for (i=0; i<cmds->used; i++) {
		if (!cmds->array[i].direct) {
			CMDARRAY subarr = (CMDARRAY)cmds->array[i].value.w;
			free_cmds(subarr);
		}
	}
	stdfree(cmds->array);
	stdfree(cmds);
}
/*============================
 * menuitem_check_cmd -- check input string & return cmd
 * Created: 2001/02/01, Perry Rapp
 *==========================*/
INT
menuset_check_cmd (MENUSET menuset, STRING str)
{
	CMDARRAY cmds = menuset->Commands;
	if (*str == '*') return CMD_MENU_TOGGLE;
	return menuitem_find_cmd(cmds, str);
}
/*============================
 * menuitem_find_cmd -- search cmd array for cmd
 *  recursive
 * Created: 2001/02/01, Perry Rapp
 *==========================*/
static INT
menuitem_find_cmd (CMDARRAY cmds, STRING str)
{
	INT pos;
	if (!find_cmd(cmds, *str, &pos))
		return CMD_NONE;
	if (cmds->array[pos].direct) {
		INT cmd = cmds->array[pos].value.i;
		return cmd;
	} else {
		CMDARRAY subarr = (CMDARRAY)cmds->array[pos].value.w;
		if (!str[1])
			return CMD_PARTIAL;
		return menuitem_find_cmd(subarr, &str[1]);
	}
}
/*============================
 * enuset_get_items -- return array of items
 * Created: 2002/10/28, Perry Rapp
 *==========================*/
MenuItem **
menuset_get_items (MENUSET menuset)
{
	return menuset->items;
}
