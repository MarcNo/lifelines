/* 
   Copyright (c) 1991-1999 Thomas T. Wetmore IV

   Permission is hereby granted, free of charge, to any person
   obtaining a copy of this software and associated documentation
   files (the "Software"), to deal in the Software without
   restriction, including without limitation the rights to use, copy,
   modify, merge, publish, distribute, sublicense, and/or sell copies
   of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/
/*=============================================================
 * screen.c -- Curses user interface to LifeLines
 * Copyright(c) 1992-96 by T.T. Wetmore IV; all rights reserved
 *===========================================================*/

#include "llstdlib.h"
#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif
#include "table.h"
#include "liflines.h"
#include "arch.h"
#include "lloptions.h"
#include "interp.h"
#include "llinesi.h"
#include "menuitem.h"
#include "screen.h"
#include "screeni.h"
#include "cscurses.h"
#include "zstr.h"
#include "cache.h"
#ifdef WIN32_ICONV_SHIM
#include "iconvshim.h"
#endif
#include "codesets.h"
#include "charprops.h"
#include "listui.h"

#define LINESREQ 24
#define COLSREQ  80
/*
OVERHEAD_MENU: 
1 line across top of screen
1 line above menu
1 line below menu
1 line of prompt message
1 line at very bottom of screen
This is how many lines can't be used by person (or whatever)
*/
#define OVERHEAD_MENU 5
INT MAINWIN_WIDTH=0;

/* center windows on real physical screen (LINES x COLS) */
#define NEWWIN(r,c)   newwin(r,c,(LINES - (r))/2,(COLS - (c))/2)
#define SUBWIN(w,r,c) subwin(w,r,c,(LINES - (r))/2,(COLS - (c))/2)

/*********************************************
 * global/exported variables
 *********************************************/

INT ll_lines = -1; /* update to be number of lines in screen */
INT ll_cols = -1;	 /* number of columns in screen used by LifeLines */
BOOLEAN stdout_vis = FALSE;
INT cur_screen = 0;
UIWINDOW main_win = NULL;
UIWINDOW stdout_win=NULL;
static UIWINDOW debug_win=NULL, debug_box_win=NULL;
static UIWINDOW ask_win=NULL, ask_msg_win=NULL;
static UIWINDOW add_menu_win=NULL, del_menu_win=NULL;
static UIWINDOW utils_menu_win=NULL, tt_menu_win=NULL;
static UIWINDOW extra_menu_win=NULL;

/*********************************************
 * external/imported variables
 *********************************************/

extern INT alldone;
extern BOOLEAN progrunning;
extern STRING empstr,empstr71,readpath,readpath_file,qSronlye,qSdataerr;
extern STRING qSabverr,qSuoperr,qSbadttnum,qSnosuchtt,qSmouttt,qSmintt;
extern STRING qSmtitle,qScright,qSdbname,qSdbimmut,qSdbrdonly,qSplschs;
extern STRING qSmn_unkcmd,qSronlya,qSronlyr;
extern STRING qSaskynq,qSaskynyn,qSaskyY;
extern STRING qSmn_quit, qSmn_changedb, qSmn_ret, qSmn_exit;
extern STRING qSmn_mmbrws,qSmn_mmsear,qSmn_mmadd,qSmn_mmdel;
extern STRING qSmn_mmprpt,qSmn_mmrpt,qSmn_mmcset;
extern STRING qSmn_mmtt,qSmn_mmut,qSmn_mmex;
extern STRING qSmn_cstsort,qSmn_cspref,qSmn_cschar,qSmn_cslcas,qSmn_csucas;
extern STRING qSmn_csrpt;
extern STRING qSmn_csrpttl;
extern STRING idsortttl,idloc;
extern STRING qSmn_edttttl,qSmn_svttttl;
extern STRING qSmn_utsave,qSmn_utread,qSmn_utgdchoo;
extern STRING qSmn_utkey,qSmn_utkpers,qSmn_utdbstat,qSmn_utmemsta;
extern STRING qSmn_utplaces,qSmn_utusropt;
extern STRING qSmn_xxbsour, qSmn_xxbeven, qSmn_xxbothr, qSmn_xxasour, qSmn_xxesour;
extern STRING qSmn_xxaeven, qSmn_xxeeven, qSmn_xxaothr, qSmn_xxeothr;
extern STRING qSerrlist,qSdefttl,qSiddefpath;

extern STRING qSmn_uttl;
extern STRING qSmn_xttl;
extern STRING qSmn_notimpl;
extern STRING qShitkey;

extern STRING qSmn_add_ttl,qSmn_add_indi,qSmn_add_fam,qSmn_add_chil,qSmn_add_spou;
extern STRING qSmn_del_ttl,qSmn_del_chil,qSmn_del_spou;
extern STRING qSmn_del_indi,qSmn_del_fam,qSmn_del_any;
extern STRING qSmn_tt_ttl,qSmn_tt_edit,qSmn_tt_load,qSmn_tt_save,qSmn_tt_exp;
extern STRING qSmn_tt_imp,qSmn_tt_dir;
extern STRING qSmn_tt_edin,qSmn_tt_ined,qSmn_tt_gdin,qSmn_tt_ingd;
extern STRING qSmn_tt_dsin,qSmn_tt_inds,qSmn_tt_inrp;


/*********************************************
 * local function prototypes
 *********************************************/

/* alphabetical */
static void add_shims_info(LIST list);
static void add_uiwin(UIWINDOW uiwin);
static void append_to_msg_list(STRING msg);
static BOOLEAN ask_for_filename_impl(STRING ttl, STRING path, STRING prmpt
	, STRING buffer, INT buflen);
static void begin_action(void);
static void check_menu(DYNMENU dynmenu);
static void check_stdout(void);
static INT choose_or_view_array (STRING ttl, INT no, STRING *pstrngs
	, BOOLEAN selecting, DETAILFNC detfnc, void *param);
static INT choose_tt(STRING prompt);
static void clear_msgs(void);
static void clear_status(void);
static void clearw(void);
static void color_hseg(WINDOW *win, INT row, INT x1, INT x2, char ch);
static void create_boxed_newwin2(UIWINDOW * puiw, CNSTRING name, INT rows, INT cols);
static void create_newwin(UIWINDOW * puiw, CNSTRING name, INT rows, INT cols, INT begy, INT begx);
static void create_uisubwindow(UIWINDOW * puiw, CNSTRING name, UIWINDOW parent, INT rows, INT cols, INT begy, INT begx);
static void create_uiwindow_impl(UIWINDOW * puiw, CNSTRING name, WINDOW * win, INT rows, INT cols);
static void create_windows(void);
static void deactivate_uiwin(void);
static void delete_uiwindow(UIWINDOW * uiw);
static void destroy_windows(void);
static void disp_trans_table_choice(UIWINDOW uiwin, INT row, INT col, INT indx);
static void display_status(STRING text);
static BOOLEAN does_match(VPTR param, VPTR el);
static void edit_tt_menu(void);
static void edit_user_options(void);
static void edit_place_table(void);
static void end_action(void);
BOOLEAN get_answer(UIWINDOW uiwin, INT row, INT col, STRING buffer, INT buflen);
static INT get_brwsmenu_size(INT screen);
static RECORD invoke_add_menu(void);
static void invoke_cset_display(void);
static void invoke_del_menu(void);
static INT invoke_extra_menu(RECORD *rec);
static void invoke_utils_menu(void);
static void output_menu(UIWINDOW uiwin, DYNMENU dynmenu);
static void place_cursor_main(void);
static void place_std_msg(void);
static void platform_postcurses_init(void);
static void refresh_main(void);
static void register_screen_lang_callbacks(BOOLEAN registering);
static void remove_uiwin(UIWINDOW uiwin);
static void repaint_add_menu(UIWINDOW uiwin);
static void repaint_delete_menu(UIWINDOW uiwin);
/*static void repaint_tt_menu(UIWINDOW uiwin);*/
static void repaint_utils_menu(UIWINDOW uiwin);
static void repaint_extra_menu(UIWINDOW uiwin);
static void repaint_main_menu(UIWINDOW uiwin);
static int resize_screen_impl(char * errmsg, int errsize);
static void run_report(BOOLEAN picklist);
static void screen_on_lang_change(VPTR uparm);
static RECORD search_for_one_record(void);
static void show_fam (UIWINDOW uiwin, RECORD frec, INT mode, INT row, INT hgt, INT width, INT * scroll, BOOLEAN reuse);
BOOLEAN show_record(UIWINDOW uiwin, STRING key, INT mode, LLRECT
	, INT * scroll, BOOLEAN reuse);
static void show_tandem_line(UIWINDOW uiwin, INT row);
static void switch_to_uiwin(UIWINDOW uiwin);
static void touch_all(BOOLEAN includeCurrent);
static void uicolor(UIWINDOW, LLRECT rect, char ch);
static INT update_browse_menu(INT screen);
static void update_screen_size(void);
static BOOLEAN yes_no_value(INT c);

/*********************************************
 * local variables
 *********************************************/

/* what is showing now in status bar */
static char status_showing[150];
/* flag if it is not important to keep */
static BOOLEAN status_transitory = FALSE;


/* total screen lines used */
static INT LINESTOTAL = LINESREQ;
/* number of lines for various menus */
static INT EMPTY_MENU = -1; /* save one horizontal line */
/* the following values are default (larger screens get more) */
static int TANDEM_LINES_DEF = 6;     /* number of lines of tandem info */
static int AUX_LINES_DEF = 15;       /* number of lines in aux window */
/* working values */
static int TANDEM_LINES=0;
static int AUX_LINES=0;

int winx=0, winy=0; /* user specified window size */

static LIST msg_list = 0;
static BOOLEAN msg_flag = FALSE; /* need to show msg list */
static BOOLEAN viewing_msgs = FALSE; /* user is viewing msgs */
static BOOLEAN lock_std_msg = FALSE; /* to hold status message */
static UIWINDOW active_uiwin = 0;
static LIST list_uiwin = 0; /* list of all uiwindows */

/* we ought to use chtype, but only if it is typedef'd, but there is no
test to see if a type is typedef'd */
static llchtype gr_btee='+', gr_ltee='+', gr_rtee='+', gr_ttee='+';
static llchtype gr_hline='-', gr_vline= '|';
static llchtype gr_llx='*', gr_lrx='*', gr_ulx='*', gr_urx='*';

/*********************************************
 * local & exported function definitions
 * body of module
 *********************************************/

/*============================
 * set_screen_graphical -- Specify whether to use ncurses box characters
 *  graphical:   [IN]  whether to use ncurses graphical box lines
 *==========================*/
void
set_screen_graphical (BOOLEAN graphical)
{
	if (graphical) {
		gr_btee = ACS_BTEE;
		gr_hline = ACS_HLINE;
		gr_ltee = ACS_LTEE;
		gr_rtee = ACS_RTEE;
		gr_ttee = ACS_TTEE;
		gr_vline = ACS_VLINE;
		gr_llx = ACS_LLCORNER;
		gr_lrx = ACS_LRCORNER;
		gr_ulx = ACS_ULCORNER;
		gr_urx = ACS_URCORNER;
	}
	else {
		gr_btee = '+';
		gr_hline = '-';
		gr_ltee = '+';
		gr_rtee = '+';
		gr_ttee = '+';
		gr_vline = '|';
		gr_llx = '*';
		gr_lrx = '*';
		gr_ulx = '*';
		gr_urx = '*';
	}
	
}
/*============================
 * init_screen -- Init screens
 *  returns 0 if current terminal is not large enough, or size requested too small
  *  errmsg:      [OUT] buffer to hold failure message
 *  errsize:     [IN]  size of errmsg buffer
 *==========================*/
int
init_screen (char * errmsg, int errsize)
{
	int rtn = resize_screen_impl(errmsg, errsize);
	if (rtn) { /* success */
		register_screen_lang_callbacks(TRUE);
		platform_postcurses_init();
	}
	return rtn;
}
/*============================
 * update_screen_size -- Recreate windows etc if screen size has changed
 *==========================*/
static void
update_screen_size (void)
{
	char errmsg[512];
	resize_screen_impl(errmsg, sizeof(errmsg)/sizeof(errmsg[0]));
}
/*============================
 * resize_screen_impl -- setup windows & things dependent on screen size
 *  returns 0 if current terminal is not large enough, or size requested too small
 *  errmsg:      [OUT] buffer to hold failure message
 *  errsize:     [IN]  size of errmsg buffer
 * Safe to be called over and over again
 * Doesn't do anything if screen size is unchanged from previous size, or if unsupported size
 *==========================*/
static int
resize_screen_impl (char * errmsg, int errsize)
{
	INT extralines=0;
	INT newlines=0, newcols=0;

	/* stop & restart ncurses to update LINES & COLS */
	if (ll_lines > 0) {
		/* need endwin before initscr below */
		endwin();
	}
	if (1) {
		WINDOW *win = initscr();
		if (!win) {
			snprintf(errmsg, errsize, _("initscr failed"));
			return 0; /* fail */
		}
		noecho();
		keypad(win, 1);
	}

	/* by default, use full screen (as many rows & cols as they have */
	newlines=LINES;
	newcols = COLS;

	/* if user specified window size, use it, but check its validity */
	if (winx) {
		newlines = winy;
		newcols = winx;
		if (newcols > COLS || newlines > LINES) {
			snprintf(errmsg, errsize
				, _("The requested window size (" FMT_INT "," FMT_INT ") is too large for your terminal (%d,%d).\n")
				, newcols, newlines, COLS, LINES);
			return 0; /* fail */
		}
	}
	/* check that terminal meet minimum requirements */
	if (newcols < COLSREQ || newlines < LINESREQ) {
		snprintf(errmsg, errsize
			, _("The requested window size (" FMT_INT "," FMT_INT ") is too small for LifeLines (%d,%d).\n")
			, newcols, newlines, COLSREQ, LINESREQ);
		return 0; /* fail */
	}

	if (ll_lines == newlines && ll_cols == newcols) {
		/* screen size is already configured & hasn't changed */
		return 1; /* succeed */
	}

	
	ll_lines = newlines;
	ll_cols = newcols;

	extralines = ll_lines - LINESREQ;
	LINESTOTAL = ll_lines;

	/* initialize browse window heights to default */
	TANDEM_LINES = TANDEM_LINES_DEF;
	AUX_LINES = AUX_LINES_DEF;
	/* increase for larger screens */
	if(extralines > 0) {
		TANDEM_LINES = TANDEM_LINES_DEF + (extralines / 2);
		AUX_LINES = AUX_LINES_DEF + extralines;
	}
	listui_init_windows(extralines);
	create_windows();
	brwsmenu_initialize(ll_lines, ll_cols);

	
	return 1; /* succeed */
}
/*============================
 * term_screen -- Terminate screens
 *  complement of init_screen
 *==========================*/
void
term_screen (void)
{
	register_screen_lang_callbacks(FALSE);
	menuitem_terminate();
	active_uiwin = 0;
	destroy_windows();

	/* must clear window size so resize_screen_impl will
	start from scratch, in case we're closing and then
	starting up again (ie, Q option from main menu) */
	ll_lines = -1;
	ll_cols = -1;
}
/*=======================================
 * draw_win_box -- wrapper for curses box
 *  handles the case that user didn't want
 *  curses graphics
 *=====================================*/
void
draw_win_box (WINDOW * win)
{
	wborder(win, gr_vline, gr_vline, gr_hline, gr_hline, gr_ulx, gr_urx, gr_llx, gr_lrx);
}
/*=======================================
 * repaint_main_menu -- Display choices for main menu
 *  See function main_menu for actions
 *=====================================*/
static void
repaint_main_menu (UIWINDOW uiwin)
{
	WINDOW *win = uiw_win(uiwin);
	INT row;
	char title[80];
	INT width=sizeof(title);
	STRING str;

	uierase(uiwin);
	draw_win_box(win);
	show_horz_line(uiwin, 4, 0, ll_cols);
	show_horz_line(uiwin, ll_lines-3, 0, ll_cols);
	if (width > ll_cols-4)
		width = ll_cols-4;
	llstrncpyf(title, width, uu8, _(qSmtitle), get_lifelines_version(ll_cols-4));
	mvccwaddstr(win, 1, 2, title);
	mvccwaddstr(win, 2, 4, _(qScright));
	str = getlloptint("FullDbPath", 1) ? readpath : readpath_file;
	mvccwprintw(win, 3, 4, _(qSdbname), str);
	if (immutable)
		wprintw(win, _(qSdbimmut));
	else if (readonly)
		wprintw(win, _(qSdbrdonly));
	row = 5;
	/* i18n problem: the letters are not being read from the menu strings */
	mvccwaddstr(win, row++, 2, _(qSplschs));
	mvccwaddstr(win, row++, 4, _(qSmn_mmbrws));
	mvccwaddstr(win, row++, 4, _(qSmn_mmsear));
	mvccwaddstr(win, row++, 4, _(qSmn_mmadd));
	mvccwaddstr(win, row++, 4, _(qSmn_mmdel));
	mvccwaddstr(win, row++, 4, _(qSmn_mmprpt));
	mvccwaddstr(win, row++, 4, _(qSmn_mmrpt));
	mvccwaddstr(win, row++, 4, _(qSmn_mmtt));
	mvccwaddstr(win, row++, 4, _(qSmn_mmut));
	mvccwaddstr(win, row++, 4, _(qSmn_mmex));
	mvccwaddstr(win, row++, 4, _(qSmn_changedb));
	mvccwaddstr(win, row++, 4, _(qSmn_exit));
}
/*==========================================
 * create_uiwindow_impl -- Create (or update) the WINDOW wrapper
 *  safe to call on existing UIWINDOW
 *========================================*/
static void
create_uiwindow_impl (UIWINDOW * puiw, CNSTRING name, WINDOW * win, INT rows, INT cols)
{
	UIWINDOW uiwin=0;
	ASSERT(puiw);
	uiwin = *puiw;
	if (!uiwin) {
		*puiw = uiwin = (UIWINDOW)stdalloc(sizeof(*uiwin));
		memset(uiwin, 0, sizeof(*uiwin));
		add_uiwin(uiwin);
	}
	if (uiwin->name)
		stdfree((STRING)uiwin->name);
	uiwin->name = strsave(name);
	if (uiw_win(uiwin) != win) {
		if (uiw_win(uiwin))
			delwin(uiw_win(uiwin));
		uiw_win(uiwin) = win;
	}
	uiw_rows(uiwin) = rows;
	uiw_cols(uiwin) = cols;
}
/*==========================================
 * add_uiwin -- Record new uiwin into master list
 *========================================*/
static void
add_uiwin (UIWINDOW uiwin)
{
	if (!list_uiwin)
		list_uiwin = create_list2(LISTNOFREE);
	enqueue_list(list_uiwin, uiwin);
}
/*==========================================
 * remove_uiwin -- Remove uiwin from master list
 *========================================*/
static void
remove_uiwin (UIWINDOW uiwin)
{
	VPTR param = uiwin;
	BOOLEAN deleteall = FALSE;
	ASSERT(list_uiwin);
	find_delete_list_elements(list_uiwin, param, &does_match, deleteall);
	// If the list is empty, then delete the list.  This avoids a memory leak.
	// The list will be created again in add_uiwin if needed.
	if (is_empty_list(list_uiwin))
		destroy_empty_list(list_uiwin);
}
/*==========================================
 * does_match -- Used as callback to remove_uiwin
 *  in finding an element in a list
 *========================================*/
static BOOLEAN
does_match (VPTR param, VPTR el)
{
	return param == el;
}
/*==========================================
 * create_boxed_newwin2 -- Create a window with
 *  an auxiliary box window outside it
 *========================================*/
static void
create_boxed_newwin2 (UIWINDOW * puiw, CNSTRING name, INT rows, INT cols)
{
	INT begy = (LINES - rows)/2;
	INT begx = (COLS - cols)/2;
	WINDOW * boxwin = newwin(rows, cols, begy, begx);
	WINDOW * win=0;
	ASSERT(puiw);
	++begy;
	++begx;
	win = subwin(boxwin, rows-2, cols-2, begy, begx);
	create_uiwindow_impl(puiw, name, win, rows-2, cols-2);
	uiw_boxwin(*puiw) = boxwin;
}
/*==========================================
 * delete_uiwindow -- Delete WINDOW wrapper & contents
 * Created: 2002/01/23
 *========================================*/
static void
delete_uiwindow (UIWINDOW * uiw)
{
	if (*uiw) {
		UIWINDOW w = *uiw;
		// remove window from master list
		remove_uiwin(w);
		// delete window (curses)
		ASSERT(uiw_win(w));
		delwin(uiw_win(w));
		// delete boxwin (curses)
		if (uiw_boxwin(w))
			delwin(uiw_boxwin(w));
		// delete window name
		ASSERT(w->name);
		stdfree((STRING)w->name);
		// delete window
		stdfree(w);
		*uiw = 0;
	}
}
/*==========================================
 * create_newwin -- Create our WINDOW wrapper
 * Create ncurses window of specified size & location
 *  and wrap it with a WUIWINDOW wrapper
 *  and return that
 *========================================*/
static void
create_newwin (UIWINDOW * puiw, CNSTRING name, INT rows, INT cols, INT begy, INT begx)
{
	WINDOW * win = newwin(rows, cols, begy, begx);
	create_uiwindow_impl(puiw, name, win, rows, cols);
}
/*==========================================
 * create_newwin2 -- Create our WINDOW wrapper
 * Create ncurses window of specified size, centered on current physical screen
 *  and wrap it with a WUIWINDOW wrapper
 *  and return that
 *========================================*/
void
create_newwin2 (UIWINDOW * puiw, CNSTRING name, INT rows, INT cols)
{
	/* NEWWIN centers window on current physical screen */
	WINDOW * win = NEWWIN(rows, cols);
	create_uiwindow_impl(puiw, name, win,rows,cols);
}
/*==========================================
 * create_uisubwindow -- Create our WINDOW wrapper
 *  for a true (& permanent) subwindow
 *========================================*/
static void
create_uisubwindow (UIWINDOW * puiw, CNSTRING name, UIWINDOW parent
	, INT rows, INT cols
	, INT begy, INT begx)
{
	WINDOW * win = subwin(uiw_win(parent), rows, cols, begy, begx);
	create_uiwindow_impl(puiw, name, win, rows, cols);
	uiw_parent(*puiw) = parent;
	uiw_permsub(*puiw) = TRUE;
}
/*==========================================
 * destroy_windows -- Undo create_windows
 *========================================*/
static void
destroy_windows (void)
{
	delete_uiwindow(&ask_msg_win);
	delete_uiwindow(&ask_win);
	delete_uiwindow(&tt_menu_win);
	delete_uiwindow(&main_win);
	delete_uiwindow(&debug_win);
	delete_uiwindow(&debug_box_win);
	delete_uiwindow(&stdout_win);
}
/*==========================================
 * create_windows -- Create and init windows
 *========================================*/
static void
create_windows (void)
{
	INT col;
	
	create_boxed_newwin2(&stdout_win, "stdout_win", ll_lines-4, ll_cols-4);
	scrollok(uiw_win(stdout_win), TRUE);
	
	col = COLS/4;
	create_newwin(&debug_box_win, "debug_box", 8, ll_cols-col-2, 1, col);

	create_uisubwindow(&debug_win, "debug", debug_box_win, 6, ll_cols-col-4, 2, col+1);
	scrollok(uiw_win(debug_win), TRUE);

	MAINWIN_WIDTH = ll_cols;
	create_newwin2(&main_win, "main", ll_lines, MAINWIN_WIDTH);

	create_newwin2(&tt_menu_win, "tt_menu", 12,MAINWIN_WIDTH-7);

	create_newwin2(&ask_win, "ask", 4, 73);

	create_newwin2(&ask_msg_win, "ask_msg", 5, 73);


	/* tt_menu_win is drawn dynamically */
	draw_win_box(uiw_win(ask_win));
	draw_win_box(uiw_win(ask_msg_win));
	draw_win_box(uiw_win(debug_box_win));
}
/*=================================
 * display_screen -- 
 * There are six screens that all use
 * the main_win. MAIN_SCREEN is the
 * intro/main menu. The other 6 are all
 * browse screens.
 * cur_screen tells which is active.
 *===============================*/
void
display_screen (INT new_screen)
{
	UIWINDOW uiwin = main_win;
	WINDOW * win = uiw_win(uiwin);
	cur_screen = new_screen;
	check_stdout();
	if (!status_showing[0] || status_transitory)
		place_std_msg();
	else
		mvccwaddstr(win, ll_lines-2, 2, status_showing);
	place_cursor_main();
	switch_to_uiwin(uiwin);
}
/*=====================================
 * do_check_stdout -- Pause for stdout/err display
 *  if it is up
 *===================================*/
static INT
do_prompt_stdout (STRING prompt)
{
	INT ch=0;
	llwprintf("\n%s\n", prompt);
	crmode();
	ch = wgetch(uiw_win(stdout_win));
	nocrmode();
	/* ok the status string was available until they struck a key */
	clear_status();
	return ch;
}
/*=====================================
 * check_stdout -- If stdout populated,
 *  prompt user to acknowledge, close & deactivate it
 *===================================*/
static void
check_stdout (void)
{
	if (active_uiwin == stdout_win) {
		if (stdout_vis) {
			do_prompt_stdout(_(qShitkey));
			stdout_vis = FALSE;
		}
		deactivate_uiwin_and_touch_all();
	}
}
/*=====================================
 * prompt_stdout -- Prompt user in stdout
 * This is like check_stdout, except it always
 * prompts, and it returns the response char,
 * and it doesn't clear stdout
 *===================================*/
INT
prompt_stdout (STRING prompt)
{
	INT i;
	if (active_uiwin != stdout_win)
		activate_uiwin(stdout_win);
	stdout_vis = TRUE;
	i = do_prompt_stdout(prompt);
	return i;
}
/*=====================================
 * search_for_one_record -- Invoke search menu & trim to one record
 *===================================*/
static RECORD
search_for_one_record (void)
{
	INDISEQ seq = invoke_search_menu();
	if (!seq) return NULL;
	if (!length_indiseq(seq)) {
		remove_indiseq(seq);
		return NULL;
	}
	/* namesort uses canonkeysort for non-persons */
	namesort_indiseq(seq);
	return choose_from_indiseq(seq, DOASK1, 
		_("Search results"), _("Search results"));
}
/*=====================================
 * main_menu -- Handle main_menu screen
 *===================================*/
void
main_menu (void)
{
	INT c;
	UIWINDOW uiwin = main_win;
	WINDOW * win = uiw_win(uiwin);
	repaint_main_menu(uiwin);
	display_screen(MAIN_SCREEN);
	c = interact_choice_string(uiwin, "bsadprtuxqQ");
	place_std_msg();
	wrefresh(win);
	switch (c) {
	case 'b': main_browse(NULL, BROWSE_INDI); break;
	case 's':
		{
			RECORD rec = search_for_one_record();
			if (rec)
				main_browse(rec, BROWSE_UNK);
		}
		break;
	case 'a': 
		{
			RECORD rec = 0;
			if (readonly) {
				msg_error("%s", _(qSronlya));
				break;
			}
			rec = invoke_add_menu();
			if (rec)
				main_browse(rec, BROWSE_UNK);
		}
		break;
	case 'd':
		{
			if (readonly) {
				msg_error("%s", _(qSronlyr));
				break;
			}
			invoke_del_menu();
		}
		break;
	case 'p': run_report(TRUE); break;
	case 'r': run_report(FALSE); break;
	case 't': edit_tt_menu(); break;
	case 'u': invoke_utils_menu(); break;
	case 'x': 
		{
			RECORD rec=0;
			c = invoke_extra_menu(&rec);
			if (c != BROWSE_QUIT)
				main_browse(rec, c);
			/* main_browse consumed rec */
		}
		break;
	case 'q': alldone = 1; break;
	case 'Q': 
		uierase(main_win);
		alldone = 2;
		break;
	}
}
/*=========================================
 * run_report -- run a report program
 *  @picklist:  [IN]  display list of reports to user ?
 *=======================================*/
void
run_report (BOOLEAN picklist)
{
	LIST progfiles = NULL; /* will prompt for report */
	STRING ofile = NULL; /* will prompt for output file */
	BOOLEAN timing = TRUE;
	begin_action();
	interp_main(progfiles, ofile, picklist, timing);
	end_action(); /* displays any errors that happened */
}
/*=========================================
 * update_browse_menu -- redraw menu if needed
 *  This is browse menu using dynamic menu 
 *  in rectangle at bottom of screen 
 * Returns number lines used by menu
 *=======================================*/
static INT
update_browse_menu (INT screen)
{
	DYNMENU dynmenu = get_screen_dynmenu(screen);
	INT lines = LINESTOTAL-OVERHEAD_MENU - get_brwsmenu_size(screen);
	if (dynmenu->dirty || (cur_screen != screen)) {
		UIWINDOW uiwin = main_win;
		WINDOW *win = uiw_win(uiwin);
		uierase(uiwin);
		draw_win_box(win);
		show_horz_line(uiwin, ll_lines-3,  0, ll_cols);
		if (!dynmenu->hidden) {
			/* display title */
			INT width = ll_cols;
			char prompt[128];
			check_menu(dynmenu);
			/* display prompt immediately above menu */
			llstrncpy(prompt, _(qSplschs), sizeof(prompt), uu8);
			dynmenu->cur_x = strlen(_(qSplschs))+3;
			dynmenu->cur_y = dynmenu->top - 1;
			llstrapps(prompt, sizeof(prompt), uu8, "             ");
			llstrappf(prompt, sizeof(prompt), uu8, _("(pg " FMT_INT "/" FMT_INT ")")
				, dynmenu->page+1, dynmenu->pages);
			/* display line across */
			show_horz_line(uiwin, dynmenu->top-2, 0, width);
			/* display prompt */
			mvccwaddstr(win, dynmenu->top-1, dynmenu->left-1, prompt);
			/* draw menu */
			output_menu(uiwin, dynmenu);
		}
	}
	dynmenu->dirty = FALSE;
	return lines;
}
/*=========================================
 * show_indi -- Show indi according to mode
 *  @uiwin:  [IN]  where to display
 *  @indi:   [IN]  whom to display
 *  @mode:   [IN]  how to display (eg, traditional, gedcom, ...)
 *  @rect:   [IN]  rectangular area in which to display
 *  @scroll: [I/O] how far down display is scrolled
 *  @reuse:  [IN]  flag to save recalculating display strings
 *=======================================*/
void
show_indi (UIWINDOW uiwin, RECORD irec, INT mode, LLRECT rect
	, INT * scroll, BOOLEAN reuse)
{
	CACHEEL icel;
	icel = indi_to_cacheel(irec);
	lock_cache(icel);
	if (mode=='g')
		show_gedcom(uiwin, irec, GDVW_NORMAL, rect, scroll, reuse);
	else if (mode=='x')
		show_gedcom(uiwin, irec, GDVW_EXPANDED, rect, scroll, reuse);
	else if (mode=='t')
		show_gedcom(uiwin, irec, GDVW_TEXT, rect, scroll, reuse);
	else if (mode=='a')
		show_ancestors(uiwin, irec, rect, scroll, reuse);
	else if (mode=='d')
		show_descendants(uiwin, irec, rect, scroll, reuse);
	else
		show_indi_vitals(uiwin, irec, rect, scroll, reuse);
	unlock_cache(icel);
}
/*=========================================
 * show_fam -- Show family
 *  fam:   [IN]  whom to display
 *  mode:  [IN]  how to display
 *  row:   [IN]  starting row to use
 *  hgt:   [IN]  how many rows allowed
 *  width: [IN]  how many columns allowed
 *  reuse: [IN]  flag to save recalculating display strings
 *=======================================*/
static void
show_fam (UIWINDOW uiwin, RECORD frec, INT mode, INT row, INT hgt
	, INT width, INT * scroll, BOOLEAN reuse)
{
	struct tag_llrect rect;
	CACHEEL fcel;
	fcel = fam_to_cacheel(frec);
	lock_cache(fcel);
	rect.top = row;
	rect.bottom = row+hgt-1;
	rect.left = 1;
	rect.right = width-1;
	if (mode=='g')
		show_gedcom(uiwin, frec, GDVW_NORMAL, &rect, scroll, reuse);
	else if (mode=='x')
		show_gedcom(uiwin, frec, GDVW_EXPANDED, &rect, scroll, reuse);
	else if (mode=='t')
		show_gedcom(uiwin, frec, GDVW_TEXT, &rect, scroll, reuse);
	else
		show_fam_vitals(uiwin, frec, row, hgt, width, scroll, reuse);
	unlock_cache(fcel);
}
/*=========================================
 * display_indi -- Paint indi on-screen
 *=======================================*/
void
display_indi (RECORD indi, INT mode, BOOLEAN reuse)
{
	INT screen = ONE_PER_SCREEN;
	INT lines=0;
	struct tag_llrect rect;
	update_screen_size(); /* ensure screen size is current */
	lines = update_browse_menu(screen);
	/* leave room for box all around */
	rect.top = 1;
	rect.bottom = lines;
	rect.left = 1;
	rect.right = MAINWIN_WIDTH-2;
	show_indi(main_win, indi, mode, &rect, &Scroll1, reuse);
	display_screen(screen);
}
/*=========================================
 * interact_indi -- Get menu choice for indi browse
 *=======================================*/
INT
interact_indi (void)
{
	return interact_screen_menu(main_win, ONE_PER_SCREEN);
}
/*=======================================
 * display_fam -- Paint fam on-screen
 *=====================================*/
void
display_fam (RECORD frec, INT mode, BOOLEAN reuse)
{
	INT width=0;
	INT screen = ONE_FAM_SCREEN;
	INT lines=0;
	update_screen_size(); /* ensure screen size is current */
	lines = update_browse_menu(screen);
	width = MAINWIN_WIDTH;
	show_fam(main_win, frec, mode, 1, lines, width, &Scroll1, reuse);
	display_screen(screen);
}
/*=========================================
 * interact_fam -- Get menu choice for indi browse
 *=======================================*/
INT
interact_fam (void)
{
	return interact_screen_menu(main_win, ONE_FAM_SCREEN);
}
/*=============================================
 * display_2indi -- Paint tandem indi screen
 *===========================================*/
void
display_2indi (RECORD irec1, RECORD irec2, INT mode)
{
	INT screen = TWO_PER_SCREEN;
	INT lines=0;
	INT lines1=0,lines2=0;
	BOOLEAN reuse = FALSE; /* can't reuse display strings in tandem */
	struct tag_llrect rect;

	update_screen_size(); /* ensure screen size is current */
	
	lines = update_browse_menu(screen);
	lines--; /* for tandem line */
	lines2 = lines/2;
	lines1 = lines - lines2;

	rect.top = 1;
	rect.bottom = lines1;
	rect.left = 1;
	rect.right = MAINWIN_WIDTH-2;

	show_indi(main_win, irec1, mode, &rect, &Scroll1, reuse);
	show_tandem_line(main_win, lines1+1);
	switch_scrolls();
	rect.top = lines1+2;
	rect.bottom = lines+1;
	show_indi(main_win, irec2, mode, &rect, &Scroll1, reuse);
	switch_scrolls();

	display_screen(screen);
}
/*=========================================
 * interact_2indi -- Get menu choice for tandem indi
 *=======================================*/
INT
interact_2indi (void)
{
	return interact_screen_menu(main_win, TWO_PER_SCREEN);
}
/*====================================
 * show_tandem_line -- Display horizontal line between top & bottom
 * PR 1999/03
 *==================================*/
static void
show_tandem_line (UIWINDOW win, INT row)
{
	show_horz_line(win, row, 0, ll_cols);
}
/*=============================================
 * display_2fam -- Paint tandem families
 *===========================================*/
void
display_2fam (RECORD frec1, RECORD frec2, INT mode)
{
	UIWINDOW uiwin = main_win;
	INT width=0;
	INT screen = TWO_FAM_SCREEN;
	INT lines=0;
	INT lines1=0,lines2=0;
	BOOLEAN reuse = FALSE; /* can't reuse display strings in tandem */

	update_screen_size(); /* ensure screen size is current */
	width=MAINWIN_WIDTH;

	lines = update_browse_menu(screen);
	lines--; /* for tandem line */
	lines2 = lines/2;
	lines1 = lines - lines2;

	show_fam(uiwin, frec1, mode, 1, lines1, width, &Scroll1, reuse);
	show_tandem_line(main_win, lines1+1);
	switch_scrolls();
	show_fam(uiwin, frec2, mode, lines1+2, lines2, width, &Scroll1, reuse);
	switch_scrolls();

	display_screen(screen);
}
/*=========================================
 * interact_2fam -- Get menu choice for tandem fam
 *=======================================*/
INT
interact_2fam (void)
{
	return interact_screen_menu(main_win, TWO_FAM_SCREEN);
}
/*=========================================
 * interact_popup -- Get menu choice for a popup window
 *=======================================*/
INT
interact_popup (UIWINDOW uiwin, STRING str)
{
	return interact_choice_string(uiwin, str);
}
/*=======================================
 * aux_browse -- Handle aux_browse screen
 * This is used for browsing S, E, or X records.
 *=====================================*/
INT
aux_browse (RECORD rec, INT mode, BOOLEAN reuse)
{
	UIWINDOW uiwin = main_win;
	INT lines = update_browse_menu(AUX_SCREEN);
	struct tag_llrect rect;
	rect.top = 1;
	rect.bottom = lines;
	rect.left = 1;
	rect.right = MAINWIN_WIDTH-1;
	show_aux(uiwin, rec, mode, &rect,  &Scroll1, reuse);
	display_screen(AUX_SCREEN);
	return interact_screen_menu(uiwin, AUX_SCREEN);
}
/*=========================================
 * list_browse -- Handle list_browse screen
 *  cur is passed for GUI doing
 *  direct navigation in list
 *  this curses implementation does not use them
 *=======================================*/
INT
list_browse (INDISEQ seq, INT top, INT * cur, INT mark)
{
	if (cur_screen != LIST_SCREEN) paint_list_screen();
	show_big_list(seq, top, *cur, mark);
	display_screen(LIST_SCREEN);
	return interact_choice_string(main_win, "jkeimrtbanx$^udUDq");
}
/*======================================
 * ask_for_db_filename -- Ask user for lifelines database directory
 *  ttl:   [IN]  title of question (1rst line)
 *  prmpt: [IN]  prompt of question (2nd line)
 *====================================*/
BOOLEAN
ask_for_db_filename (CNSTRING ttl, CNSTRING prmpt, CNSTRING basedir, STRING buffer, INT buflen)
{
	basedir=basedir; /* unused */
	/* This could have a list of existing ones like askprogram.c */
	return ask_for_string(ttl, prmpt, buffer, buflen);
}
/*======================================
 * ask_for_output_filename -- Ask user for filename to which to write
 *  returns static buffer
 *  ttl1:    [IN]  title of question (1rst line)
 *  prmpt:   [IN]  prompt of question (3rd line)
 *  buffer:  [OUT] response
 *  buflen:  [IN]  max size of response
 *====================================*/
BOOLEAN
ask_for_output_filename (STRING ttl, STRING path, STRING prmpt, STRING buffer, INT buflen)
{
	/* curses version doesn't differentiate input from output prompts */
	return ask_for_filename_impl(ttl, path, prmpt, buffer, buflen);
}
/*======================================
 * ask_for_input_filename -- Ask user for filename from which to read
 *  returns static buffer
 *  ttl1:    [IN]  title of question (1rst line)
 *  prmpt:   [IN]  prompt of question (3rd line)
 *  buffer:  [OUT] response
 *  buflen:  [IN]  max size of response
 *====================================*/
BOOLEAN
ask_for_input_filename (STRING ttl, STRING path, STRING prmpt, STRING buffer, INT buflen)
{
	/* curses version doesn't differentiate input from output prompts */
	return ask_for_filename_impl(ttl, path, prmpt, buffer, buflen);
}
/*======================================
 * ask_for_input_filename_impl -- Ask user for a filename
 *  (in curses version, we don't differentiate input from output prompts)
 *  ttl1:    [IN]  title of question (1rst line)
 *  path:    [IN]  path prompt (2nd line)
 *  prmpt:   [IN]  prompt of question (3rd line)
 *  buffer:  [OUT] response
 *  buflen:  [IN]  max size of response
 *====================================*/
static BOOLEAN
ask_for_filename_impl (STRING ttl, STRING path, STRING prmpt, STRING buffer, INT buflen)
{
	/* display current path (truncated to fit) */
	char curpath[120];
	INT len = sizeof(curpath);
	if (len > uiw_cols(ask_msg_win)-2)
		len = uiw_cols(ask_msg_win)-2;
	curpath[0] = 0;
	llstrapps(curpath, len, uu8, _(qSiddefpath));
	llstrapps(curpath, len, uu8, compress_path(path, len-strlen(curpath)-1));

	return ask_for_string2(ttl, curpath, prmpt, buffer, buflen);
}
/*======================================
 * refresh_main -- touch & refresh main or stdout
 *  as appropriate
 *====================================*/
static void
refresh_main (void)
{
	WINDOW *win = stdout_vis ? uiw_win(stdout_win) : uiw_win(main_win);
	wrefresh(win);
}
/*======================================
 * ask_for_string -- Ask user for string
 *  returns static buffer
 *  ttl:     [IN]  title of question (1rst line)
 *  prmpt:   [IN]  prompt of question (2nd line)
 *  buffer:  [OUT] response
 *  buflen:  [IN]  max size of response
 *====================================*/
BOOLEAN
ask_for_string (CNSTRING ttl, CNSTRING prmpt, STRING buffer, INT buflen)
{
	UIWINDOW uiwin = ask_win;
	WINDOW *win = uiw_win(uiwin);
	BOOLEAN rtn;
	uierase(uiwin);
	draw_win_box(win);
	mvccuwaddstr(uiwin, 1, 1, ttl);
	mvccuwaddstr(uiwin, 2, 1, prmpt);
	activate_uiwin(uiwin);
	rtn = get_answer(uiwin, 2, strlen(prmpt) + 2, buffer, buflen);
	deactivate_uiwin_and_touch_all();
	return rtn;
}
/*======================================
 * ask_for_string2 -- Ask user for string
 * Two lines of title
 *  returns static buffer
 *  ttl1:    [IN]  title of question (1rst line)
 *  ttl2:    [IN]  2nd line of title
 *  prmpt:   [IN]  prompt of question (3rd line)
 *  buffer:  [OUT] response
 *  buflen:  [IN]  max size of response
 *====================================*/
BOOLEAN
ask_for_string2 (CNSTRING ttl1, CNSTRING ttl2, CNSTRING prmpt, STRING buffer, INT buflen)
{
	UIWINDOW uiwin = ask_msg_win;
	WINDOW *win = uiw_win(uiwin);
	BOOLEAN rtn;
	uierase(uiwin);
	draw_win_box(win);
	mvccwaddstr(win, 1, 1, ttl1);
	mvccwaddstr(win, 2, 1, ttl2);
	mvccwaddstr(win, 3, 1, prmpt);
	wrefresh(win);
	activate_uiwin(uiwin);
	rtn = get_answer(uiwin, 3, strlen(prmpt) + 2, buffer, buflen);
	deactivate_uiwin_and_touch_all();
	return rtn;
}
/*========================================
 * yes_no_value -- Convert character to TRUE if y(es)
 *======================================*/
static BOOLEAN
yes_no_value (INT c)
{
	STRING ptr;
	for (ptr = _(qSaskyY); *ptr; ptr++) {
		if (c == *ptr) return TRUE;
	}
	return FALSE;
}
/*========================================
 * ask_yes_or_no -- Ask yes or no question
 *  ttl:  [IN]  title to display
 *======================================*/
BOOLEAN
ask_yes_or_no (STRING ttl)
{
	INT c = ask_for_char(ttl, _(qSaskynq), _(qSaskynyn));
	return yes_no_value(c);
}
/*=========================================================
 * ask_yes_or_no_msg -- Ask yes or no question with message
 *  msg:   [IN]  top line displayed
 *  ttl:   [IN]  2nd line displayed
 *=======================================================*/
BOOLEAN
ask_yes_or_no_msg (STRING msg, STRING ttl)
{
	INT c = ask_for_char_msg(msg, ttl, _(qSaskynq), _(qSaskynyn));
	return yes_no_value(c);
}
/*=======================================
 * ask_for_char -- Ask user for character
 *  ttl:   [IN]  1nd line displayed
 *  prmpt: [IN]  2nd line text before cursor
 *  ptrn:  [IN]  List of allowable character responses
 *=====================================*/
INT
ask_for_char (STRING ttl, STRING prmpt, STRING ptrn)
{
	return ask_for_char_msg(NULL, ttl, prmpt, ptrn);
}
/*===========================================
 * ask_for_char_msg -- Ask user for character
 *  msg:   [IN]  top line displayed (optional)
 *  ttl:   [IN]  2nd line displayed 
 *  prmpt: [IN]  3rd line text before cursor
 *  ptrn:  [IN]  List of allowable character responses
 *=========================================*/
INT
ask_for_char_msg (STRING msg, STRING ttl, STRING prmpt, STRING ptrn)
{
	UIWINDOW uiwin = (msg ? ask_msg_win : ask_win);
	WINDOW *win = uiw_win(uiwin);
	INT y;
	INT rv;
	uierase(uiwin);
	draw_win_box(win);
	y = 1;
	if (msg)
		mvccwaddstr(win, y++, 2, msg);
	mvccwaddstr(win, y++, 2, ttl);
	mvccwaddstr(win, y++, 2, prmpt);
	wrefresh(win);
	rv = interact_choice_string(uiwin, ptrn);
	return rv;
}
/*============================================
 * choose_from_array -- Choose from string list
 *  ttl:      [IN] title for choice display
 *  no:       [IN] number of choices
 *  pstrngs:  [IN] array of choices
 * returns 0-based index chosen, or -1 if cancelled
 *==========================================*/
INT
choose_from_array (STRING ttl, INT no, STRING *pstrngs)
{
	BOOLEAN selecting = TRUE;
	if (!ttl) ttl=_(qSdefttl);
	return choose_or_view_array(ttl, no, pstrngs, selecting, 0, 0);
}
/*============================================
 * display_list -- Show user list of information
 *  ttl:    [IN] title for display
 *  list    [IN] list of string entries
 * returns 0-based index chosen, or -1 if cancelled
 *==========================================*/
INT
display_list (STRING ttl, LIST list)
{
	/* TODO: Need to set some flag to suppress i & <enter> */
	return choose_from_list(ttl, list);
}
/*============================================
 * choose_from_list -- Choose from string list
 *  ttl:    [IN] title for display
 *  list    [IN] list of string choices
 * returns 0-based index chosen, or -1 if cancelled
 *==========================================*/
INT
choose_from_list (STRING ttl, LIST list)
{
	STRING * array=0;
	STRING choice=0;
	INT i=0, rtn=-1;
	INT len = length_list(list);

	if (len < 1) return -1;
	if (!ttl) ttl=_(qSdefttl);

	array = (STRING *) stdalloc(len*sizeof(STRING));
	i = 0;
	FORLIST(list, el)
		choice = (STRING)el;
		ASSERT(choice);
		array[i] = strsave(choice);
		++i;
	ENDLIST

	rtn = choose_from_array(ttl, len, array);

	for (i=0; i<len; ++i)
		strfree(&array[i]);
	stdfree(array);
	return rtn;
}
/*============================================
 * choose_from_array_x -- Choose from string list
 *  ttl:      [IN]  title for choice display
 *  no:       [IN]  number of choices
 *  pstrngs:  [IN]  array of choices
 *  detfnc:   [IN]  callback for details about items
 *  param:    [IN]  opaque type for callback
 * returns 0-based index chosen, or -1 if cancelled
 *==========================================*/
INT
choose_from_array_x (STRING ttl, INT no, STRING *pstrngs, DETAILFNC detfnc
	, void *param)
{
	BOOLEAN selecting = TRUE;
	if (!ttl) ttl=_(qSdefttl);
	return choose_or_view_array(ttl, no, pstrngs, selecting, detfnc, param);
}
/*============================================
 * view_array -- Choose from string list
 *  ttl:      [IN] title for choice display
 *  no:       [IN] number of choices
 *  pstrngs:  [IN] array of choices
 * returns 0-based index chosen, or -1 if cancelled
 *==========================================*/
void
view_array (STRING ttl, INT no, STRING *pstrngs)
{
	BOOLEAN selecting = FALSE;
	choose_or_view_array(ttl, no, pstrngs, selecting, 0, 0);
}
/*============================================
 * choose_or_view_array -- Implement choose/view from array
 *  ttl:       [IN]  title for choice display
 *  no:        [IN]  number of choices
 *  pstrngs:   [IN]  array of choices
 *  selecting: [IN]  if FALSE then view-only
 *  detfnc:    [IN]  callback for details about items
 *  param:     [IN]  opaque type for callback
 * returns 0-based index chosen, or -1 if cancelled
 *==========================================*/
static INT
choose_or_view_array (STRING ttl, INT no, STRING *pstrngs, BOOLEAN selecting
	, DETAILFNC detfnc, void *param)
{
	INT rv;
	rv = array_interact(ttl, no, pstrngs, selecting, detfnc, param);
	refresh_main();
	return rv;
}
/*=============================================================
 * choose_one_from_indiseq -- 
 * Choose a single person from indiseq
 * Returns index of selected item (or -1 if user quit)
 *  ttl:  [IN]  title
 *===========================================================*/
INT
choose_one_from_indiseq (STRING ttl, INDISEQ seq)
{
	return choose_one_or_list_from_indiseq(ttl, seq, FALSE);
}
/*==========================================================
 * choose_list_from_indiseq -- User chooses subsequence from
 *   person sequence
 * returns input sequence, but may have deleted elements
 * called by both reports & interactive use
 *  ttl:  [IN]  title/caption for choice list
 *  seq:  [I/O] list from which to choose (user may delete items)
 * returns index of where user choose select (or -1 if quit)
 *========================================================*/
INT
choose_list_from_indiseq (STRING ttl, INDISEQ seq)
{
	return choose_one_or_list_from_indiseq(ttl, seq, TRUE);
}
/*==============================
 * draw_tt_win -- Draw menu for edit translations
 *============================*/
static void
draw_tt_win (STRING prompt)
{
	UIWINDOW uiwin = tt_menu_win;
	WINDOW *win = uiw_win(uiwin);
	INT row = 0;
	uierase(uiwin);
	draw_win_box(win);
	row = 1;
	mvccwaddstr(win, row++, 2, prompt);
	disp_trans_table_choice(uiwin, row++, 4, MEDIN);
	disp_trans_table_choice(uiwin, row++, 4, MINED);
	disp_trans_table_choice(uiwin, row++, 4, MGDIN);
	disp_trans_table_choice(uiwin, row++, 4, MINGD);
	disp_trans_table_choice(uiwin, row++, 4, MDSIN);
	disp_trans_table_choice(uiwin, row++, 4, MINDS);
	disp_trans_table_choice(uiwin, row++, 4, MRPIN);
	disp_trans_table_choice(uiwin, row++, 4, MINRP);
	mvccwaddstr(win, row++, 4, _(qSmn_ret));
}
/*==============================
 * disp_trans_table_choice -- Display line in
 * translation table menu, & show current info
 * Created: 2001/07/20
 *============================*/
static void
disp_trans_table_choice (UIWINDOW uiwin, INT row, INT col, INT trnum)
{
	XLAT xlat;
	ZSTR zstr, z1;

	xlat = transl_get_predefined_xlat(trnum);
	zstr = transl_get_predefined_menukey(trnum);
	z1 = transl_get_predefined_name(trnum);
	zs_apps(zstr, " ");
	zs_appz(zstr, z1);
	zs_apps(zstr, " : ");
	if (xlat) {
		zs_appz(zstr, transl_get_description(xlat));
	} else {
		zs_sets(zstr, _("No conversion"));
	}
	mvccuwaddstr(uiwin, row, col, zs_str(zstr));
	zs_free(&zstr);
}
/*============================
 * invoke_add_menu -- Handle add menu
 * returns addref'd record
 *==========================*/
static RECORD
invoke_add_menu (void)
{
	UIWINDOW uiwin=0;
	WINDOW * win=0;
	RECORD rec=0;
	INT code;

	if (!add_menu_win) {
		create_newwin2(&add_menu_win, "add_menu", 8, 66);
		add_menu_win->outdated = TRUE; /* needs drawing */
	}

	if (add_menu_win->outdated) {
		repaint_add_menu(add_menu_win);
	}
	uiwin = add_menu_win;
	win = uiw_win(uiwin);

	activate_uiwin(uiwin);
	wmove(win, 1, 27);
	code = interact_choice_string(uiwin, "pfcsq");
	deactivate_uiwin_and_touch_all();

	switch (code) {
	case 'p':
		rec = add_indi_by_edit(&disp_long_rfmt);
		break;
	case 'f': add_family_by_edit(NULL, NULL, NULL, &disp_long_rfmt); break;
	case 'c': my_prompt_add_child(NULL, NULL); break;
	case 's': prompt_add_spouse(NULL, NULL, TRUE); break;
	case 'q': break;
	}
	return rec;
}
/*===============================
 * invoke_del_menu -- Handle delete menu
 *=============================*/
void
invoke_del_menu (void)
{
	INT code;
	UIWINDOW uiwin=0;
	WINDOW * win=0;
	if (!del_menu_win) {
		create_newwin2(&del_menu_win, "del_menu", 9, 66);
		del_menu_win->outdated = TRUE; /* needs drawing */
	}
	if (del_menu_win->outdated) {
		repaint_delete_menu(del_menu_win);
	}
	uiwin = del_menu_win;
	win = uiw_win(uiwin);

	activate_uiwin(uiwin);
	wmove(win, 1, 30);
	code = interact_choice_string(uiwin, "csifoq");
	deactivate_uiwin_and_touch_all();

	switch (code) {
	case 'c': choose_and_remove_child(NULL, NULL, FALSE); break;
	case 's': choose_and_remove_spouse(NULL, NULL, FALSE); break;
	case 'i': choose_and_remove_indi(NULL, DOCONFIRM); break;
	case 'f': choose_and_remove_family(); break;
	case 'o': choose_and_remove_any_record(NULL, DOCONFIRM); break;
	case 'q': break;
	}
}
/*======================================
 * invoke_cset_display -- Handle character set menu
 *====================================*/
static void
invoke_cset_display (void)
{
	LIST list = create_list2(LISTDOFREE);
	ZSTR zstr=zs_newn(80);

	zs_setf(zstr, "%s: %s", _("Internal codeset"), int_codeset);
	enqueue_list(list, strsave(zs_str(zstr)));
	if (uu8) {
		enqueue_list(list, strsave(_("Internal UTF-8: Yes")));
	} else {
		enqueue_list(list, strsave(_("Internal UTF-8: No")));
	}

	if (are_locales_supported())
		enqueue_list(list, strsave(_("Locales are enabled.")));
	else
		enqueue_list(list, strsave(_("Locales are disabled.")));
	
	if (is_nls_supported()) {
		enqueue_list(list, strsave(_("NLS (National Language Support) is compiled in.")));
		zs_setf(zstr, "LocaleDir (default): %s", LOCALEDIR);
		enqueue_list(list, strsave(zs_str(zstr)));
		zs_setf(zstr,  "LocaleDir (override): %s", getlloptstr("LocaleDir", ""));
		enqueue_list(list, strsave(zs_str(zstr)));
	} else {
		enqueue_list(list, strsave(_("NLS (National Language Support) is not compiled in.")));
	}

	if (1) {
		CNSTRING str = get_gettext_codeset();
		str = str ? str : "";
		zs_setf(zstr, "bind_textdomain_codeset: %s", str);
		enqueue_list(list, strsave(zs_str(zstr)));
	}

	add_shims_info(list);

	if (is_iconv_supported())
		enqueue_list(list, strsave(_("iconv (codeset conversion) is compiled in.")));
	else
		enqueue_list(list, strsave(_("iconv (codeset conversion) is not compiled in.")));
	
	zs_setf(zstr, _("Startup collate locale: %s"), get_original_locale_collate());
	enqueue_list(list, strsave(zs_str(zstr)));
	
	zs_setf(zstr, _("Startup messages locale: %s")	, get_original_locale_msgs());
	enqueue_list(list, strsave(zs_str(zstr)));
	
	zs_setf(zstr, _("Current collate locale: %s"), get_current_locale_collate());
	enqueue_list(list, strsave(zs_str(zstr)));
	
	zs_setf(zstr, _("Current messages locale: %s"), get_current_locale_msgs());
	enqueue_list(list, strsave(zs_str(zstr)));

	zs_setf(zstr, _("Collation routine: %s"), ll_what_collation());
	enqueue_list(list, strsave(zs_str(zstr)));
	
	if (eqstr_ex(gui_codeset_in, gui_codeset_out)) {
		zs_setf(zstr, _("GUI codeset: %s"), gui_codeset_in);
		enqueue_list(list, strsave(zs_str(zstr)));
	} else {
		zs_setf(zstr, _("GUI output codeset: %s"), gui_codeset_out);
		enqueue_list(list, strsave(zs_str(zstr)));
		zs_setf(zstr, _("GUI input codeset: %s"), gui_codeset_in);
		enqueue_list(list, strsave(zs_str(zstr)));
	}

	if (eqstr_ex(editor_codeset_in, editor_codeset_out)) {
		zs_setf(zstr, _("editor codeset: %s"), editor_codeset_in);
		enqueue_list(list, strsave(zs_str(zstr)));
	} else {
		zs_setf(zstr, _("editor output codeset: %s"), editor_codeset_out);
		enqueue_list(list, strsave(zs_str(zstr)));
		zs_setf(zstr, _("editor input codeset: %s"), editor_codeset_in);
		enqueue_list(list, strsave(zs_str(zstr)));
	}

	if (eqstr_ex(report_codeset_in, report_codeset_out)) {
		zs_setf(zstr, _("report codeset: %s"), report_codeset_in);
		enqueue_list(list, strsave(zs_str(zstr)));
	} else {
		zs_setf(zstr, _("report output codeset: %s"), report_codeset_out);
		enqueue_list(list, strsave(zs_str(zstr)));
		zs_setf(zstr, _("report input codeset: %s"), report_codeset_in);
		enqueue_list(list, strsave(zs_str(zstr)));
	}

	if (eqstr_ex(gedcom_codeset_in, gedcom_codeset_out)) {
		zs_setf(zstr, _("GEDCOM codeset: %s"), gedcom_codeset_in);
		enqueue_list(list, strsave(zs_str(zstr)));
	} else {
		zs_setf(zstr, _("gedcom output codeset: %s"), gedcom_codeset_out);
		enqueue_list(list, strsave(zs_str(zstr)));
		zs_setf(zstr, _("gedcom input codeset: %s"), gedcom_codeset_in);
		enqueue_list(list, strsave(zs_str(zstr)));
	}

	zs_setf(zstr, "TTPATH: %s", getlloptstr("TTPATH", "."));
	enqueue_list(list, strsave(zs_str(zstr)));

	if (charprops_is_loaded()) {
		enqueue_list(list, strsave(_("UTF-8 charprops loaded" )));
	} else {
		enqueue_list(list, strsave(_("UTF-8 charprops not loaded" )));
	}


	display_list(_("Codeset information"), list);
	destroy_list(list);
	zs_free(&zstr);
}
/*======================================
 * add_shims_info -- Add information about gettext and iconv dlls
 *====================================*/
static void
add_shims_info (LIST list)
{
	ZSTR zstr=zs_newn(80);
	list=list; /* only used on MS-Windows */
#ifdef WIN32_INTL_SHIM
	{
		char value[MAXPATHLEN];
		if (intlshim_get_property("dll_path", value, sizeof(value)))
		{
			zs_setf(zstr, _("gettext dll: %s"), value);
			enqueue_list(list, strsave(zs_str(zstr)));
			if (intlshim_get_property("dll_version", value, sizeof(value)))
			{
				zs_setf(zstr, _("gettext dll version: %s"), value);
				enqueue_list(list, strsave(zs_str(zstr)));
			}
			else
			{
				enqueue_list(list, strsave(_("gettext dll had no version")));
			}
		}
		else
		{
			enqueue_list(list, strsave(_("no gettext dll found")));
		}
	}
#endif
#ifdef WIN32_ICONV_SHIM
	{
		char value[MAXPATHLEN];
		if (iconvshim_get_property("dll_path", value, sizeof(value)))
		{
			zs_setf(zstr, _("iconv dll: %s"), value);
			enqueue_list(list, strsave(zs_str(zstr)));
			if (iconvshim_get_property("dll_version", value, sizeof(value)))
			{
				zs_setf(zstr, _("iconv dll version: %s"), value);
				enqueue_list(list, strsave(zs_str(zstr)));
			}
			else
			{
				enqueue_list(list, strsave(_("iconv dll had no version")));
			}
		}
		else
		{
			enqueue_list(list, strsave(_("no iconv dll found")));
		}
	}
#endif
	zs_free(&zstr);
}
/*======================================
 * invoke_trans_menu -- menu for translation tables
 * TODO: decide whether to bring this back or not
 *====================================*/
#ifdef UNUSED_CODE
static void
invoke_trans_menu (void)
{
	INT code;
	UIWINDOW uiwin=0;
	BOOLEAN done=FALSE;

	if (!trans_menu_win) {
		trans_menu_win = create_newwin2("trans_menu", 10,66);
	}
	uiwin = trans_menu_win;

	while (!done) {
		stdout_vis=FALSE;
		repaint_trans_menu(uiwin);
		reactivate_uiwin(uiwin);
		wmove(uiw_win(uiwin), 1, strlen(_(qSmn_tt_ttl))+3);
		code = interact(uiwin, "elsxiq", -1);

		begin_action();
		switch (code) {
		case 'e': edit_tt_menu(); break;
		case 'l': load_tt_action(); break;
		case 's': save_tt_action(); break;
		case 'x': export_tts(); break;
		case 'i': import_tts(); break;
		case 'q': done=TRUE; break;
		}
		end_action(); /* displays any errors that happened */
	}
	deactivate_uiwin_and_touch_all();
}
#endif
/*======================================
 * edit_tt_menu -- menu for "Edit translation table"
 *====================================*/
static void
edit_tt_menu (void)
{
	INT ttnum;
	while ((ttnum = choose_tt(_(qSmn_edttttl))) != -1) {
		edit_mapping(ttnum);
		stdout_vis = FALSE; /* don't need to see errors after done */
	}
}
/*======================================
 * load_tt_action -- menu for "Load translation table"
 *====================================*/
#ifdef UNUSED_CODE
static void
load_tt_action (void)
{
	FILE * fp;
	STRING fname=0;
	INT ttnum;
	STRING ttimportdir;

	if (readonly) {
		msg_error("%s", _(qSronlye));
		return;
	}

	/* Ask which table */
	ttnum = choose_tt(_(qSmn_svttttl));
	if (ttnum == -1) return;
	if (ttnum < 0 || ttnum >= NUM_TT_MAPS) {
		msg_error("%s", _(qSbadttnum));
		return;
	}

	/* Ask whence to load it */
	ttimportdir = getlloptstr("TTPATH", ".");
	fp = ask_for_input_file(LLREADTEXT, _(qSmintt), &fname, ttimportdir, ".tt");
	if (fp) {
		fclose(fp);
		/* Load it */
		if (!load_new_tt(fname, ttnum))
			msg_error("%s", _(qSdataerr));
	}
	strfree(&fname);
}
/*======================================
 * save_tt_action -- save a translation table
 * to a file
 *====================================*/
static void
save_tt_action (void)
{
	FILE * fp;
	STRING fname=0;
	INT ttnum;
	STRING ttexportdir;
	
	/* Ask which table */
	ttnum = choose_tt(_(qSmn_svttttl));
	if (ttnum == -1) return;
	if (ttnum < 0 || ttnum >= NUM_TT_MAPS) {
		msg_error("%s", _(qSbadttnum));
		return;
	}
	if (!transl_get_legacy_tt(ttnum)) {
		msg_error("%s", _(qSnosuchtt));
		return;
	}
	/* Ask whither to save it */
	ttexportdir = getlloptstr("LLTTEXPORT", ".");
	fp = ask_for_output_file(LLWRITETEXT, _(qSmouttt), &fname, ttexportdir, ".tt");
	if (fp) {
		fclose(fp);
		/* Save it */
		if (!save_tt_to_file(ttnum, fname)) {
			msg_error("%s", _(qSdataerr));
			strfree(&fname);
			return;
		}
	}
	strfree(&fname);
}
#endif
/*======================================
 * choose_tt -- select a translation table (-1 for none)
 *====================================*/
static INT
choose_tt (STRING prompt)
{
	INT code;
	UIWINDOW uiwin = tt_menu_win;
	while (1) {
		draw_tt_win(prompt);
		activate_uiwin(uiwin);
		wmove(uiw_win(uiwin), 1, strlen(prompt)+3);
		code = interact_choice_string(uiwin, "emixgdrq");
		deactivate_uiwin_and_touch_all();
		switch (code) {
		case 'e': return MEDIN;
		case 'm': return MINED;
		case 'i': return MGDIN;
		case 'x': return MINGD;
		case 'g': return MDSIN;
		case 'd': return MINDS;
		case 'r': return MINRP;
		case 'q': return -1;
		}
	}
}
/*====================================
 * invoke_utils_menu -- Handle utilities menu
 *==================================*/
static void
invoke_utils_menu (void)
{
	INT code;
	UIWINDOW uiwin=0;
	WINDOW *win=0;

	if (!utils_menu_win) {
		create_newwin2(&utils_menu_win, "utils_menu", 14, 66);
		utils_menu_win->outdated = TRUE; /* needs drawing */
	}
	if (utils_menu_win->outdated) {
		repaint_utils_menu(utils_menu_win);
	}
	uiwin = utils_menu_win;
	win = uiw_win(uiwin);
	activate_uiwin(uiwin);

	wmove(win, 1, strlen(_(qSmn_uttl))+3);
	code = interact_choice_string(uiwin, "srRkidmeocq");
	deactivate_uiwin_and_touch_all();

	begin_action();
	switch (code) {
	case 's': save_gedcom(); break;
	case 'r': load_gedcom(FALSE); break;
	case 'R': load_gedcom(TRUE); break;
	case 'k': key_util(); break;
	case 'i': who_is_he_she(); break;
	case 'd': show_database_stats(); break;
	case 'm': display_cache_stats(); break;
	case 'e': edit_place_table(); break;
	case 'o': edit_user_options(); break;
	case 'c': invoke_cset_display(); break;
		/*
		we could add edit_global_config pretty easily, but the difficulty is
		that we don't know what to do about codeset with it :( [2002.06.18, Perry]
		*/
	case 'q': break;
	}
	end_action();
}
/*================================
 * invoke_extra_menu -- Handle extra menu
 *==============================*/
static INT
invoke_extra_menu (RECORD *prec)
{
	INT code;
	UIWINDOW uiwin=0;
	WINDOW *win=0;

	if (!extra_menu_win) {
		create_newwin2(&extra_menu_win, "extra_menu", 13,66);
		extra_menu_win->outdated = TRUE; /* needs drawing */
	}
	if (extra_menu_win->outdated) {
		repaint_extra_menu(extra_menu_win);
	}
	uiwin = extra_menu_win;
	win = uiw_win(uiwin);

	while (1) {

		activate_uiwin(uiwin);
		wmove(win, 1, strlen(_(qSmn_xttl))+3);
		code = interact_choice_string(uiwin, "sex123456q");
		deactivate_uiwin_and_touch_all();

		switch (code) {
		case 's': return BROWSE_SOUR;
		case 'e': return BROWSE_EVEN;
		case 'x': return BROWSE_AUX;
		case '1': *prec = edit_add_source(); return BROWSE_SOUR;
		case '2': edit_source(NULL, &disp_long_rfmt); return BROWSE_QUIT;
		case '3': *prec = edit_add_event(); return BROWSE_EVEN;
		case '4': edit_event(NULL, &disp_long_rfmt); return BROWSE_QUIT;
		case '5': *prec = edit_add_other(); return BROWSE_AUX;
		case '6': edit_other(NULL, &disp_long_rfmt); return BROWSE_QUIT;
		case 'q': return BROWSE_QUIT;
		}
	}
}
/*===============================
 * uopt_validate -- Validator when user edits 'user options table'
 *  returns descriptive string for failure, 0 for pass
 *=============================*/
static STRING
uopt_validate (TABLE tab, void * param)
{
	STRING codeset = valueof_str(tab, "codeset");
	STRING original_codeset = (STRING)param;
	/*
	our only rule currently is that user may not change codeset
	of a populated database
	*/
	if (!eqstr_ex(codeset, original_codeset)
		&& !eqstr_ex(codeset, int_codeset)) {
		if (num_indis()+num_fams()+num_sours()+num_evens()+num_othrs())
			return _("Impermissible to change codeset in a populated database");
	}
	return 0;
}
/*===============================
 * edit_place_table -- Allow user to edit the table of place abbreviations
 *=============================*/
static void
edit_place_table (void)
{
	edit_valtab_from_db("VPLAC", &placabbvs, ':', _(qSabverr), 0, 0);
}
/*===============================
 * edit_user_options -- Allow user to edit options embedded in current db
 *=============================*/
static void
edit_user_options (void)
{
	TABLE uopts = create_table_str();
	STRING param=0;
	get_db_options(uopts);
	param = valueof_str(uopts, "codeset");
	param = (param ? strsave(param) : 0);

	if (edit_valtab_from_db("VUOPT", &uopts, '=', _(qSuoperr), uopt_validate, (void *)param))
		set_db_options(uopts);
	strfree(&param);
	release_table(uopts);
}
/*============================================
 * get_answer -- Have user respond with string
 *  uiwin:   [IN] which window to use
 *  row:     [IN]  prompt location (vert)
 *  col:     [IN]  prompt location (horiz)
 *  buffer:  [OUT] response
 *  buflen:  [IN]  max size of response
 *  Has not been codeset-converted to internal yet
 *==========================================*/
BOOLEAN
get_answer (UIWINDOW uiwin, INT row, INT col, STRING buffer, INT buflen)
{
	WINDOW *win = uiw_win(uiwin);
	BOOLEAN rtn = FALSE;

	/* TODO: Is this necessary ? It prevents entering long paths */
	if (buflen > uiw_cols(uiwin)-col-1)
		buflen = uiw_cols(uiwin)-col-1;

	echo();
	wmove(win, row, col);
	if (wgetccnstr(win, buffer, buflen) != ERR)
		rtn = TRUE;
	noecho();
	buffer[buflen-1] = 0; /* ensure zero-termination */

	return rtn;
}
/*================================================================
 * show_record -- Display record (any type) in requested mode
 *  uiwin:  [IN]  whither to draw
 *  key:    [IN]  key of record to display
 *  mode:   [IN]  what display mode (eg, vitals vs GEDCOM vs...)
 *  rect:   [IN]  where to draw
 *  scroll: [I/O] current scroll setting
 *  reuse:  [IN]  flag indicating if same record drawn last time
 * returns TRUE if record was found, else FALSE (no record, nothing drawn)
 *==============================================================*/
BOOLEAN
show_record (UIWINDOW uiwin, STRING key, INT mode, LLRECT rect
	, INT * scroll, BOOLEAN reuse)
{
	INT row = rect->top;
	INT hgt = rect->bottom - rect->top + 1;
	INT width = rect->right - rect->left + 1;
	if (key[0]=='I') {
		RECORD irec = key_to_irecord(key);
		if (irec)
			show_indi(uiwin, irec, mode, rect, scroll, reuse);
		return irec != NULL;
	} else if (key[0]=='F') {
		RECORD frec = key_to_frecord(key);
		if (frec)
			show_fam(uiwin, frec, mode, row, hgt, width, scroll, reuse);
		return frec != NULL;

	} else {
		/* could be S,E,X -- show_aux handles all of these */
		RECORD rec = qkey_to_record(key);
		if (rec)
			show_aux(uiwin, rec, mode, rect, scroll, reuse);
		return rec != NULL;
	}
}
/*===================================================
 * message_string -- Return background message string
 *=================================================*/
STRING
message_string (void)
{
	if (!cur_screen) return "";
	if (cur_screen == MAIN_SCREEN)
		return _("LifeLines -- Main Menu");
	ASSERT(cur_screen >= 1);
	ASSERT(cur_screen <= MAX_SCREEN);
	return get_screen_title(cur_screen);
}
/*=================================================
 * place_std_msg - Place standard message on screen
 *===============================================*/
void
place_std_msg (void)
{
	/* msg is placed on main window */
	UIWINDOW uiwin = main_win;
	WINDOW *win = uiw_win(uiwin);
	STRING str = message_string();
	INT row = ll_lines-2;
	clear_hseg(win, row, 2, ll_cols-2);
	mvccwaddstr(win, row, 2, str);
	/* now we need to repaint main window, but if there are
	subwindows up, instead we call the touch_all routine,
	which does them all from ancestor to descendant */
	if (active_uiwin)
		touch_all(TRUE);
	else
		wrefresh(win); 
	place_cursor_main();
}
/*==================================+
 * rpt_print -- Implement report language print function
 * Created: 2003-02-01 (Perry Rapp)
 *=================================*/
void
rpt_print (STRING str)
{
	llwprintf("%s", str);
}
/*=================================================
 * llvwprintf -- Called as wprintf(fmt, argp)
 *===============================================*/
void
llvwprintf (STRING fmt, va_list args)
{
	UIWINDOW uiwin = stdout_win;
	if (uiwin) {
		WINDOW *win = uiw_win(uiwin);
		if (!stdout_vis) {
			clearw();
			activate_uiwin(uiwin);
		}
		vccwprintw(win, fmt, args);
		wrefresh(win);
	} else {
		vccprintf(fmt, args);
	}
	/*
	TO DO
	It would be nice to add this to the msg list
	but we need to deal with embedded carriage returns first
	so we can't do this yet. Also, if we do put it on the msg list,
	it is going to duplicate the stdout display currently being
	used (which is nicer looking, but scrolls off-screen).
	*/
/*	msg_outputv(MSG_ERROR, fmt, args);*/ /* also send to msg list */
}
/*=================================================
 * llwprintf -- Called as wprintf(fmt, arg, arg, ...)
 *===============================================*/
void
llwprintf (STRING fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	llvwprintf(fmt, args);
	va_end(args);
}
/*==============================
 * clearw -- Clear stdout window
 *============================*/
void
clearw (void)
{
	UIWINDOW uiwin = stdout_win;
	WINDOW *win = uiw_win(uiwin);
	WINDOW *boxwin = uiw_boxwin(uiwin);
	uierase(uiwin);
	draw_win_box(boxwin);
	wmove(win, 0, 0);
	stdout_vis = TRUE;
	wrefresh(boxwin);
}
/*=======================================
 * wfield -- Write field in stdout window
 *=====================================*/
void
wfield (INT row, INT col, STRING str)
{
	UIWINDOW uiwin = stdout_win;
	WINDOW *win = uiw_win(uiwin);
	if (!stdout_vis) clearw();
	mvccwaddstr(win, row, col, str);
	wrefresh(win);
}
/*===========================================
 * wpos -- Position to place in stdout window
 *=========================================*/
void
wpos (INT row, INT col)
{
	UIWINDOW uiwin = stdout_win;
	WINDOW *win = uiw_win(uiwin);
	wmove(win, row, col);
}
/*=======================================
 * show_horz_line -- Draw horizontal line
 *=====================================*/
void
show_horz_line (UIWINDOW uiwin, INT row, INT col, INT len)
{
	WINDOW *win = uiw_win(uiwin);
	INT i;
	mvwaddch(win, row, col, gr_ltee);
	for (i = 0; i < len-2; i++)
		waddch(win, gr_hline);
	waddch(win, gr_rtee);
}
/*=====================================
 * show_vert_line -- Draw vertical line
 *===================================*/
void
show_vert_line (UIWINDOW uiwin, INT row, INT col, INT len)
{
	WINDOW *win = uiw_win(uiwin);
	INT i;
	mvwaddch(win, row++, col, gr_ttee);
	for (i = 0; i < len-2; i++)
		mvwaddch(win, row++, col, gr_vline);
	mvwaddch(win, row, col, gr_btee);
}
/*=============================================
 * place_cursor_popup -- Move to cursor input location
 * For use with UIWINDOW menus -- popup menus which live
 * in their own private UIWINDOW (& curses) window,
 * such as the full scan popup
 *===========================================*/
void
place_cursor_popup (UIWINDOW uiwin)
{
	wmove(uiw_win(uiwin), uiw_cury(uiwin), uiw_curx(uiwin));
}
/*=============================================
 * place_cursor_main -- Move to idle cursor location
 * for use with main menu screens
 *===========================================*/
static void
place_cursor_main (void)
{
	INT row=0, col = 30;
	DYNMENU dynmenu = get_screen_dynmenu(cur_screen);

	/* Hide/Display Cursor */
	if (dynmenu && dynmenu->hidden) {
		curs_set(0);
	} else {
		curs_set(1);
	}

	/* Position Cursor */
	switch (cur_screen) {
	case MAIN_SCREEN:    
		row = 5;
		col = strlen(_(qSplschs))+3;
		break;
	case LIST_SCREEN:
		listui_placecursor_main(&row, &col);
		break;
	case ONE_PER_SCREEN:
	case ONE_FAM_SCREEN:
	case AUX_SCREEN:
	case TWO_PER_SCREEN:
	case TWO_FAM_SCREEN:
		{
			/* These screens, which live on the main screen uiwindow,
			all use dynamic menus, and the cursor position in dynamic
			menus is controlled by the dynamic menu, because cursor
			moves up & down with dynamic menu */
			if (dynmenu->hidden) {
				/* no need to position cursor */
			} else {
				row = dynmenu->cur_y;
				col = dynmenu->cur_x;
			}
		}
		break;
	default:
		row = 1;
		col = 1;
		break;
	}
	wmove(uiw_win(main_win), row, col);
}
/*=============================================
 * dbprintf -- Debug printf(fmt, arg, arg, ...)
 *===========================================*/
void
dbprintf (STRING fmt, ...)
{
	va_list args;
	touchwin(uiw_win(debug_box_win));
	va_start(args, fmt);
	vccwprintw(uiw_win(debug_win), fmt, args);
	va_end(args);
	wrefresh(uiw_win(debug_box_win));
	sleep(2);
	touchwin(uiw_win(main_win));
	wrefresh(uiw_win(main_win));
}
/*==================================================
 * do_edit -- Shift to user's screen editor and back
 *================================================*/
void
do_edit (void)
{
	int rtn=-1;

	endwin();
#ifdef WIN32
	/* use w32system, because it will wait for the editor to finish */
	rtn = w32system(editstr);
#else
	rtn = system(editstr);
#endif
	if (rtn != 0) {
		printf(_("Editor or system call failed."));
		puts("");
		sleep(2);
	}

	clearok(curscr, 1);
	place_cursor_main();
	wrefresh(curscr);
	noecho();
}
/*================================================
 * mvwaddstr_lim -- output a string, like mvwaddstr
 *  except trim it to no more than maxlen wide
 *==============================================*/
static void
mvwaddstr_lim (WINDOW *wp, int x, int y, char *cp, INT maxlen)
{
	char buffer[60];
	if ((INT)strlen(cp)<=maxlen)
		mvccwaddstr(wp, x, y, cp);
	else {
		if (maxlen > (INT)sizeof(buffer)-1)
			maxlen = sizeof(buffer)-1;
		llstrncpy(buffer, cp, maxlen-1, uu8);
		strcat(buffer, "*");
		mvccwaddstr(wp, x, y, buffer);
	}
}
/*================================================
 * check_menu -- update menu layout info
 *  (in case just resized)
 * Created: 2002/10/24 (Perry Rapp)
 *==============================================*/
static void
check_menu (DYNMENU dynmenu)
{
	/* (reserve spots for ? and q on each page) */
	dynmenu->pageitems = dynmenu->rows*dynmenu->cols-2;
	dynmenu->pages = (dynmenu->size-1)/dynmenu->pageitems+1;
	if (dynmenu->size < dynmenu->pageitems + 1) { /* don't need ? if they fit */
		dynmenu->pages = 1;
		dynmenu->page = 0;
	}
}
/*================================================
 * output_menu -- print menu array to screen
 * Caller specifies bottom row to use, & width
 * Menu structure contains all menu items & # of
 * columns to use
 *==============================================*/
static void
output_menu (UIWINDOW uiwin, DYNMENU dynmenu)
{
	WINDOW *win = uiw_win(uiwin);
	INT row;
	INT icol=0;
	INT col=3;
	/* more legible names */
	INT MenuSize = dynmenu->size;
	INT MenuCols = dynmenu->cols;
	INT MenuPage = dynmenu->page;
	INT MenuPages = dynmenu->pages;
	INT pageitems = dynmenu->pageitems;
	MENUSET menuset = dynmenu_get_menuset(dynmenu);
	MenuItem ** items = menuset_get_items(menuset);
	INT width = dynmenu->width;
	/* reserve 2 spaces at each end, and one space in front of each Col */
	INT colwidth = (width-4)/MenuCols-1;
	INT Item = 0;
	Item = MenuPage * pageitems;
	if (Item >= MenuSize)
		Item = ((MenuSize-1)/pageitems)*pageitems;
	icol = 0;
	col = 3;
	row = dynmenu->top;
	/* now display all the menu items we can fit on this page */
	while (1)
	{
		mvwaddstr_lim(win, row, col, items[Item++]->LocalizedDisplay, colwidth);
		if (Item == MenuSize)
			break;
		row++;
		if (icol<MenuCols-1 && row>dynmenu->bottom)
		{
			icol++;
			col += colwidth+1;
			row = dynmenu->top;
			continue;
		}
		if (MenuPages == 1) {
			/* one slot reserved for "q" */
			if (icol==MenuCols-1 && row==dynmenu->bottom)
				break;
		} else {
			/* two slots reserved, "q" & "?" */
			if (icol==MenuCols-1 && row==dynmenu->bottom-1)
				break;
		}
	}
	/* print the "q" and "?" items */
	row = dynmenu->bottom-1;
	col = 3+(MenuCols-1)*(colwidth+1);
	if (dynmenu->pages > 1)
		mvwaddstr_lim(win, row, col, g_MenuItemOther.LocalizedDisplay, colwidth);
	mvwaddstr_lim(win, ++row, col, g_MenuItemQuit.LocalizedDisplay, colwidth);
}
/*==================================================================
 * toggle_browse_menu - toggle display of menu at bottom of screen
 *================================================================*/
void
toggle_browse_menu (void)
{
	dynmenu_toggle_menu(get_screen_dynmenu(cur_screen));
}
/*==================================================================
 * cycle_browse_menu() - show other menu choices on browse menu
 *================================================================*/
void
cycle_browse_menu (void)
{
	dynmenu_next_page(get_screen_dynmenu(cur_screen));
}
/*==================================================================
 * adjust_browse_menu_height - Change height of current browse screen menu
 *================================================================*/
void
adjust_browse_menu_height (INT delta)
{
	dynmenu_adjust_height(get_screen_dynmenu(cur_screen), delta);
}
/*==================================================================
 * adjust_browse_menu_cols - Change # of columns in current menu
 *================================================================*/
void
adjust_browse_menu_cols (INT delta)
{
	dynmenu_adjust_menu_cols(get_screen_dynmenu(cur_screen), delta);
}
/*=========================================
 * get_brwsmenu_size -- How many lines does browse menu take ?
 *=======================================*/
static INT
get_brwsmenu_size (INT screen)
{
	DYNMENU dynmenu = get_screen_dynmenu(screen);
	return dynmenu->hidden ? EMPTY_MENU : dynmenu->rows+1;
}
/*=====================
 * clear_stdout_hseg -- clear a horizontal line segment on stdout win
 *====================*/
void
clear_stdout_hseg (INT row, INT x1, INT x2)
{
	UIWINDOW uiwin = stdout_win;
	WINDOW *win = uiw_win(uiwin);
	clear_hseg(win, row, x1, x2);
}
/*=====================
 * clear_hseg -- clear a horizontal line segment
 *  (used for partial screen clears)
 *====================*/
void
clear_hseg (WINDOW *win, INT row, INT x1, INT x2)
{
	/* workaround for curses bug with spacs */
	if (getlloptint("ForceScreenErase", 0) > 0) {
		/* fill virtual output with dots */
		color_hseg(win, row, x1, x2, '_');
		wnoutrefresh(win);
		/* now fill it back with spaces */
		color_hseg(win, row, x1, x2, ' ');
		wrefresh(win);
	} else {
		color_hseg(win, row, x1, x2, ' ');
	}
}
/*=====================
 * color_hseg -- fill a horizontal line segment
 *  (used for clearing)
 *====================*/
static void
color_hseg (WINDOW *win, INT row, INT x1, INT x2, char ch)
{
	INT i;
	for (i=x1; i<=x2; ++i)
		mvwaddch(win, row, i, ch);
}
/*===============================================
 * display_status -- put string in status line
 * We don't touch the status_transitory flag
 * That is caller's responsibility.
 *=============================================*/
static void
display_status (STRING text)
{
	UIWINDOW uiwin = main_win;
	WINDOW *win = uiw_win(uiwin);
	INT row;
	/* first store it */
	llstrncpy(status_showing, text, sizeof(status_showing), uu8);
	if ((INT)strlen(text)>ll_cols-6) {
		status_showing[ll_cols-8] = 0;
		strcat(status_showing, "...");
	}
	/* then display it */
	row = ll_lines-2;
	clear_hseg(win, row, 2, ll_cols-2);
	wmove(win, row, 2);
	mvccwaddstr(win, row, 2, status_showing);
	place_cursor_main();
	wrefresh(win);
}
/*=========================================
 * message -- handle generic message
 * delegates to msg_outputv
 * TODO: replace with msg_error/info/status
 *=======================================*/
void
message (STRING fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	msg_outputv(MSG_ERROR, fmt, args); 
	va_end(args);
}
/*=========================================
 * msg_error -- handle error message
 * delegates to msg_outputv
 *=======================================*/
void
msg_error (STRING fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	msg_outputv(MSG_ERROR, fmt, args); 
	va_end(args);
}
/*=========================================
 * msg_info -- handle regular messages
 * delegates to msg_outputv
 *=======================================*/
void
msg_info (STRING fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	msg_outputv(MSG_INFO, fmt, args);
	va_end(args);
}
/*=========================================
 * msg_status -- handle transitory/status messages
 * delegates to msg_outputv
 *=======================================*/
void
msg_status (STRING fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	msg_outputv(MSG_STATUS, fmt, args);
	va_end(args);
}
/*=========================================
 * msg_output -- handle any message
 * delegates to msg_outputv
 *=======================================*/
void
msg_output (MSG_LEVEL level, STRING fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	msg_outputv(level, fmt, args);
	va_end(args);
}
/*=====================================
 * msg_width -- get max width of msgs
 *===================================*/
INT
msg_width (void)
{
	return ll_cols-6;
}
/*=========================================
 * msg_outputv -- output message varargs style arguments
 * Actually all other msg functions delegate to here.
 *  @level:     -1=error,0=info,1=status
 *  @fmt:   [IN]  printf style format string
 *  @args:  [IN]  vprintf style varargs
 * Puts into message list and/or into status area
 *=======================================*/
void
msg_outputv (MSG_LEVEL level, STRING fmt, va_list args)
{
	char buffer[250];
	STRING ptr;
	unsigned int width = MAINWIN_WIDTH-5;
	/* prefix errors & infos with * and space respectively */
	switch(level) {
		case MSG_ERROR:
			buffer[0] = '*';
			ptr = &buffer[1];
			break;
		case MSG_INFO:
			buffer[0] = ' ';
			ptr = &buffer[1];
			break;
		default:
			ptr = buffer;
			break;
	}
	/* now make string to show/put on msg list */
	llstrncpyvf(ptr, sizeof(buffer)-1, uu8, fmt, args);
	/* first handle transitory/status messages */
	if (level==MSG_STATUS) {
		if (lock_std_msg)
			return; /* can't display it, status bar is locked */
		if (status_showing[0] && !status_transitory) {
			/* we are overwriting something important 
			so it is already on the msg list, we just need to make
			sure the msg list gets displayed */
			if (!viewing_msgs)
				msg_flag = TRUE;
		}
		display_status(buffer);
		return;
	}
	/* everything important goes onto msg list */
	append_to_msg_list(buffer);
	/* update flag about whether we need to show msg list to user */
	/* being careful in case we are currently *in* the msg list
	show routine */
	if (!viewing_msgs && (length_list(msg_list)>1 || lock_std_msg)) {
		msg_flag = TRUE;
	}
	/* now put it to status area if appropriate */
	if (!lock_std_msg) {
		if (strlen(buffer)>width) {
			buffer[width-4]=0;
			strcat(buffer, "...");
/*
TODO: This doesn't make sense until the msg list handles long strings
			if (!viewing_msgs)
				msg_flag = TRUE;
*/
		}
		display_status(buffer);
	}
}
/*=========================================
 * msg_impl -- put msg on the msg list
 * This is a list that we show the user
 * when the current command completes,
 * unless it only had one item, and it got
 * put on the status bar, and it wasn't too wide.
 *=======================================*/
static void
append_to_msg_list (STRING msg)
{
		if (!msg_list)
			msg_list = create_list2(LISTDOFREE);
		enqueue_list(msg_list, strsave(msg));
}
/*=========================================
 * begin_action -- prepare to process users choice
 *=======================================*/
static
void begin_action (void)
{
	clear_msgs();
}
/*=========================================
 * end_action -- finished processing users choice
 *  show msg list if appropriate
 *=======================================*/
static
void end_action (void)
{
	/* pause for keypress for finish stdout/err if appropriate */
	check_stdout();
	/* put up list of errors if appropriate */
	if (msg_flag && msg_list) {
		STRING * strngs = (STRING *)stdalloc(length_list(msg_list)*sizeof(STRING));
		INT i=0;
		FORLIST(msg_list, el)
			strngs[i++] = el;
		ENDLIST
		viewing_msgs = TRUE; /* suppress msg generation */
		view_array(_(qSerrlist), length_list(msg_list), strngs);
		viewing_msgs = FALSE;
		stdfree(strngs);
		clear_msgs();
	}
}
/*=========================================
 * clear_msgs -- delete msg list
 *  The msg list holds messages when several
 *  occurred during the last operation
 *=======================================*/
static void
clear_msgs (void)
{
	if (msg_list) {
		destroy_list(msg_list);
		msg_list = 0;
	}
	msg_flag = FALSE;
	/* also clear status bar */
	clear_status();
}
/*=========================================
 * clear_status -- clear status string
 *  The status string is the last message displayed
 *  and is shown at the very bottom of the main screen
 *=======================================*/
static void
clear_status (void)
{
	if (!lock_std_msg)
		status_showing[0]=0;
}
/*=========================================
 * lock_status_msg -- temporarily hold status message
 *=======================================*/
void
lock_status_msg (BOOLEAN lock)
{
	lock_std_msg = lock;
}
/*=====================================
 * repaint_add_menu -- Draw menu choices for main add item menu
 *===================================*/
static void
repaint_add_menu (UIWINDOW uiwin)
{
	WINDOW *win = uiw_win(uiwin);
	INT row = 1;
	uierase(uiwin);
	draw_win_box(win);
	mvccwaddstr(win, row++, 2, _(qSmn_add_ttl));
	mvccwaddstr(win, row++, 4, _(qSmn_add_indi));
	mvccwaddstr(win, row++, 4, _(qSmn_add_fam));
	mvccwaddstr(win, row++, 4, _(qSmn_add_chil));
	mvccwaddstr(win, row++, 4, _(qSmn_add_spou));
	mvccwaddstr(win, row++, 4, _(qSmn_ret));
	uiwin->outdated = FALSE;
}
/*=====================================
 * repaint_delete_menu -- Draw menu choices for main delete item menu
 *===================================*/
static void
repaint_delete_menu (UIWINDOW uiwin)
{
	WINDOW *win = uiw_win(uiwin);
	INT row = 1;
	uierase(uiwin);
	draw_win_box(win);
	mvccwaddstr(win, row++, 2, _(qSmn_del_ttl));
	mvccwaddstr(win, row++, 4, _(qSmn_del_chil));
	mvccwaddstr(win, row++, 4, _(qSmn_del_spou));
	mvccwaddstr(win, row++, 4, _(qSmn_del_indi));
	mvccwaddstr(win, row++, 4, _(qSmn_del_fam));
	mvccwaddstr(win, row++, 4, _(qSmn_del_any));
	mvccwaddstr(win, row++, 4, _(qSmn_ret));
	uiwin->outdated = FALSE;
}
/*=====================================
 * repaint_utils_menu -- 
 *===================================*/
static void
repaint_utils_menu (UIWINDOW uiwin)
{
	WINDOW *win = uiw_win(uiwin);
	INT row = 1;
	uierase(uiwin);
	draw_win_box(win);
	mvccwaddstr(win, row++, 2, _(qSmn_uttl));
	mvccwaddstr(win, row++, 4, _(qSmn_utsave));
	mvccwaddstr(win, row++, 4, _(qSmn_utread));
	mvccwaddstr(win, row++, 4, _(qSmn_utgdchoo));
	mvccwaddstr(win, row++, 4, _(qSmn_utkey));
	mvccwaddstr(win, row++, 4, _(qSmn_utkpers));
	mvccwaddstr(win, row++, 4, _(qSmn_utdbstat));
	mvccwaddstr(win, row++, 4, _(qSmn_utmemsta));
	mvccwaddstr(win, row++, 4, _(qSmn_utplaces));
	mvccwaddstr(win, row++, 4, _(qSmn_utusropt));
	mvccwaddstr(win, row++, 4, _(qSmn_mmcset));
	mvccwaddstr(win, row++, 4, _(qSmn_quit));
	uiwin->outdated = FALSE;
}
/*=====================================
 * repaint_extra_menu -- 
 *===================================*/
static void
repaint_extra_menu (UIWINDOW uiwin)
{
	WINDOW *win = uiw_win(uiwin);
	INT row = 1;
	uierase(uiwin);
	draw_win_box(win);
	mvccwaddstr(win, row++, 2, _(qSmn_xttl));
	mvccwaddstr(win, row++, 4, _(qSmn_xxbsour));
	mvccwaddstr(win, row++, 4, _(qSmn_xxbeven));
	mvccwaddstr(win, row++, 4, _(qSmn_xxbothr));
	mvccwaddstr(win, row++, 4, _(qSmn_xxasour));
	mvccwaddstr(win, row++, 4, _(qSmn_xxesour));
	mvccwaddstr(win, row++, 4, _(qSmn_xxaeven));
	mvccwaddstr(win, row++, 4, _(qSmn_xxeeven));
	mvccwaddstr(win, row++, 4, _(qSmn_xxaothr));
	mvccwaddstr(win, row++, 4, _(qSmn_xxeothr));
	mvccwaddstr(win, row++, 4, _(qSmn_quit));
	uiwin->outdated = FALSE;
}
/*============================
 * activate_uiwin -- 
 *  push new uiwindow on top of current one
 *==========================*/
void
activate_uiwin (UIWINDOW uiwin)
{
	WINDOW * win = uiw_win(uiwin);
	ASSERT(uiwin);
	ASSERT(win);
	ASSERT(!uiw_parent(uiwin));

	/* link into parent/child chain */
	uiw_parent(uiwin) = active_uiwin;
	if (active_uiwin) {
		ASSERT(!uiw_child(active_uiwin));
		uiw_child(active_uiwin) = uiwin;
		/* refresh current (in case it was obscured by stdout */
		wrefresh(uiw_win(active_uiwin));
	}

	/* switch to new & refresh */
	active_uiwin = uiwin;
	touchwin(win);
	wrefresh(win);
}
/*============================
 * reactivate_uiwin --
 *  push new window on top, if not already on top
 *  and refresh it in any case
 *==========================*/
#ifdef UNUSED_CODE
static void
reactivate_uiwin (UIWINDOW uiwin)
{
	if (active_uiwin != uiwin)
		activate_uiwin(uiwin);
	else {
		WINDOW * win = uiw_win(uiwin);
		touchwin(win);
		wrefresh(win);
	}
}
#endif
/*============================
 * deactivate_uiwin -- Remove currently active
 *  and pop to its parent (if it has one)
 *==========================*/
static void
deactivate_uiwin (void)
{
	UIWINDOW uiw = active_uiwin;
	active_uiwin = uiw_parent(active_uiwin);
	if (active_uiwin) {
		ASSERT(uiw_child(active_uiwin)==uiw);
	}
	uiw_parent(uiw)=0;
	if (uiw_dynamic(uiw))
		delete_uiwindow(&uiw);
	if (active_uiwin && uiw_child(active_uiwin)) {
		uiw_child(active_uiwin)=0;
	}
}
/*=============================================
 * deactivate_uiwin_and_touch_all --
 *  remove current window & repaint ones left
 *===========================================*/
void
deactivate_uiwin_and_touch_all (void)
{
	deactivate_uiwin();
	if (active_uiwin)
		touch_all(TRUE);
}
/*============================
 * touch_all -- Repaint all ancestors of current window
 * from furthest to nearest
 *==========================*/
static void
touch_all (BOOLEAN includeCurrent)
{
	UIWINDOW uiwin=active_uiwin;
	ASSERT(uiwin);
	/* climb to highest window ancestor */
	while (uiw_parent(uiwin)) {
		uiwin = uiw_parent(uiwin);
	}
	/* walk down touching */
	while (uiwin && (includeCurrent || uiwin!=active_uiwin)) {
		touchwin(uiw_win(uiwin));
		wrefresh(uiw_win(uiwin));
		uiwin = uiw_child(uiwin);
	}
}
/*============================
 * switch_to_uiwin -- 
 *  switch away from currently active uiwin
 *  to new uiwin
 *  currently active uiwin (if any) must be solo
 *  new uiwin must be solo
 *==========================*/
static void
switch_to_uiwin (UIWINDOW uiwin)
{
	WINDOW * win = uiw_win(uiwin);
	if (uiwin != active_uiwin)
	{
		ASSERT(uiwin);
		ASSERT(win);	
		ASSERT(!uiw_parent(uiwin));
		ASSERT(!uiw_child(uiwin));

		/* link into parent/child chain */
		uiw_parent(uiwin) = active_uiwin;
		if (active_uiwin)
		{
			/* current active window must be solo, no parent or child */
			ASSERT(!uiw_child(active_uiwin));
			ASSERT(!uiw_parent(active_uiwin));
		}
		/* switch to new & refresh */
		active_uiwin = uiwin;
	}
	touchwin(win);
	wrefresh(win);
}
/*============================
 * refresh_stdout -- 
 *  bring stdout to front
 *==========================*/
void
refresh_stdout (void)
{
	wrefresh(uiw_win(stdout_win));
}
/*============================
 * call_system_cmd -- 
 *  execute a shell command (for report interpreter)
 *==========================*/
void
call_system_cmd (STRING cmd)
{
	int rtn=-1;

	endwin();
#ifndef WIN32
	rtn = system("clear");
#endif
	rtn = system(cmd);

	if (rtn != 0) {
		printf(_("System command failed."));
		puts("");
		sleep(2);
	}
	clearok(curscr, 1);
	place_cursor_main();
	wrefresh(curscr);
}
/*============================
 * uierase -- erase window 
 *  handles manual erasing if broken_curses flag set
 *==========================*/
void
uierase (UIWINDOW uiwin)
{
	LLRECT rect = 0;
	wipe_window_rect(uiwin, rect);
}
/*================================================
 * wipe_window_rect -- Clear a rectangle in a window
 *  handle curses space bug
 *==============================================*/
void
wipe_window_rect (UIWINDOW uiwin, LLRECT rect)
{
	WINDOW * win = uiw_win(uiwin);
	/* workaround for curses bug with spaces */
	if (getlloptint("ForceScreenErase", 0) > 0) {
		/*
		To fix the dirty output on a redhat 6 system
		(with ncurses-5.2-8), required the call to
		redrawwin, instead of using wrefresh.
		Perry, 2002.05.27
		*/
		/*
		uicolor(uiwin, rect, '=');
		wnoutrefresh(win);
		*/
		/* now fill it back with spaces */
		uicolor(uiwin, rect, ' ');
		redrawwin(win);
	} else {
		/* fill it back with spaces */
		if (rect)
			uicolor(uiwin, rect, ' ');
		else
			werase(win); /* let curses do it */
	}
}
/*============================
 * uicolor -- fill window with character 
 *  if rect is nonzero, fill that rectangular area
 *  if rect is zero, fill entire window
 *==========================*/
static void
uicolor (UIWINDOW uiwin, LLRECT rect, char ch)
{
	INT i;
	WINDOW *win = uiw_win(uiwin);
	struct tag_llrect rects;

	if (!rect) {
		rects.top = 0;
		rects.bottom = uiw_rows(uiwin)-1;
		rects.left = 0;
		rects.right = uiw_cols(uiwin)-1;
		rect = &rects;
	}
	for (i=rect->top; i <= rect->bottom; ++i) {
		color_hseg(win, i, rect->left, rect->right, ch);
	}
}
/*==================================================
 * platform_postcurses_init -- platform-specific code
 *  coming after curses initialized
 *================================================*/
static void
platform_postcurses_init (void)
{
#ifdef WIN32
	char buffer[80];
	STRING title = _(qSmtitle);
	snprintf(buffer, sizeof(buffer), title, get_lifelines_version(sizeof(buffer)-1-strlen(title)));
	wtitle(buffer);
#endif
}
/*==================================================
 * get_main_screen_width -- current width of main screen
 *================================================*/
INT
get_main_screen_width (void)
{
	return MAINWIN_WIDTH;
}
/*==================================================
 * get_gr_ttee -- current character used for box corners
 *================================================*/
llchtype
get_gr_ttee (void)
{
	return gr_ttee; /* eg, '+' */
}
/*==================================================
 * clear_status_display -- clear any lingering status display
 * (called by interact code after user has pressed a button)
 *================================================*/
void
clear_status_display (void)
{
	if (progrunning) return;
	if (lock_std_msg) return;
	if (status_showing[0]) {
		status_showing[0] = 0;
		place_std_msg();
	}
}
/*============================
 * register_screen_lang_callbacks -- (un)register our callbacks
 *  for language or codeset changes
 *==========================*/
static void
register_screen_lang_callbacks (BOOLEAN registering)
{
	if (registering) {
		register_uilang_callback(screen_on_lang_change, 0);
		register_uicodeset_callback(screen_on_lang_change, 0);
	} else {
		unregister_uilang_callback(screen_on_lang_change, 0);
		unregister_uicodeset_callback(screen_on_lang_change, 0);
	}
}
/*============================
 * screen_on_lang_change -- UI language  or codeset has changed
 *==========================*/
static void
screen_on_lang_change (VPTR uparm)
{
	LIST_ITER listit=0;
	VPTR ptr=0;
	uparm = uparm; /* unused */
	listit = begin_list(list_uiwin);
	while (next_list_ptr(listit, &ptr)) {
		UIWINDOW uiwin = (UIWINDOW)ptr;
		uiwin->outdated = TRUE;
	}
	end_list_iter(&listit);

}
