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
 * main.c -- Main program of LifeLines
 * Copyright(c) 1992-95 by T.T. Wetmore IV; all rights reserved
 * pre-SourceForge version information:
 *   2.3.4 - 24 Jun 93    2.3.5 - 07 Aug 93
 *   2.3.6 - 02 Oct 93    3.0.0 - 11 Oct 94
 *   3.0.1 - 11 Oct 93    3.0.2 - 01 Jan 95
 *   3.0.3 - 02 Jul 96
 *===========================================================*/

#include "llstdlib.h"
/* llstdlib.h pulls in standard.h, config.h, sys_inc.h */
#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif
#include "btree.h"
#include "table.h"
#include "translat.h"
#include "gedcom.h"
#include "liflines.h"
#include "arch.h"
#include "lloptions.h"
#include "interp.h"
#include "version.h"

#include "llinesi.h"
#include "screen.h" /* calling initscr, noecho, ... */

/* for parser debugging */
extern int yydebug;

#ifdef HAVE_GETOPT
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif /* HAVE_GETOPT_H */
#endif /* HAVE_GETOPT */

/*********************************************
 * external variables (no header)
 *********************************************/

extern STRING qSmtitle,qSnorwandro,qSnofandl,qSbdlkar;
extern STRING qSusgFinnOpt,qSusgFinnAlw,qSusgNorm;
extern STRING qSbaddb;

extern INT csz_indi;
extern INT csz_fam;
extern INT csz_sour;
extern INT csz_even;
extern INT csz_othr;
extern INT winx, winy;

extern int opterr;

/*********************************************
 * required global variables
 *********************************************/


static STRING usage_summary = "";      /* usage string */
int opt_finnish  = FALSE;      /* Finnish Language sorting order if TRUE */
int opt_mychar = FALSE;        /* Custom character set handling (bypass libc) */
BOOLEAN debugmode = FALSE;     /* no signal handling, so we can get coredump */
BOOLEAN opt_nocb  = FALSE;     /* no cb. data is displayed if TRUE */
BOOLEAN keyflag   = TRUE;      /* show key values */
BOOLEAN readonly  = FALSE;     /* database is read only */
BOOLEAN writeable = FALSE;     /* database must be writeable */
BOOLEAN immutable = FALSE;     /* make no changes at all to database, for access to truly read-only medium */
INT alldone       = 0;         /* completion flag */
BOOLEAN progrunning = FALSE;   /* program is running */
BOOLEAN progparsing = FALSE;   /* program is being parsed */
INT     progerror = 0;         /* error count during report program */
BOOLEAN traditional = TRUE;    /* use traditional family rules */
BOOLEAN showusage = FALSE;     /* show usage */
BOOLEAN showversion = FALSE;   /* show version */
STRING  readpath_file = NULL;  /* last component of readpath */
STRING  readpath = NULL;       /* database path used to open */

/*********************************************
 * local function prototypes
 *********************************************/

/* alphabetical */
static void load_usage(void);
static void main_db_notify(STRING db, BOOLEAN opening);
static void parse_arg(const char * optarg, char ** optname, char **optval);
static void print_usage(void);

/*********************************************
 * local function definitions
 * body of module
 *********************************************/

/*==================================
 * main -- Main routine of LifeLines
 *================================*/
int
main (int argc, char **argv)
{
	extern char *optarg;
	extern int optind;
	char * msg;
	int c;
	BOOLEAN ok=FALSE;
	STRING dbrequested=NULL; /* database (path) requested */
	STRING dbused=NULL; /* database (path) found */
	BOOLEAN forceopen=FALSE, lockchange=FALSE;
	char lockarg = 0; /* option passed for database lock */
	INT alteration=0;
	LIST exprogs=NULL;
	TABLE exargs=NULL;
	STRING progout=NULL;
	BOOLEAN graphical=TRUE;
	STRING configfile=0;
	STRING crashlog=NULL;
	int i=0;

	/* initialize all the low-level platform code */
	init_arch();

	/* initialize all the low-level library code */
	init_stdlib();

#if HAVE_SETLOCALE
	/* initialize locales */
	setlocale(LC_ALL, "");
#endif /* HAVE_SETLOCALE */
	
#if ENABLE_NLS
	/* setup gettext translation */
	/* NB: later we may revise locale dir or codeset
	based on user settings */
	ll_bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
#endif

	save_original_locales();
	load_usage();

	/* handle conventional arguments --version and --help */
	/* needed for help2man to synthesize manual pages */
	for (i=1; i<argc; ++i) {
		if (!strcmp(argv[i], "--version")
			|| !strcmp(argv[i], "-v")) {
			print_version("llines");
			return 0;
		}
		if (!strcmp(argv[i], "--help")
			|| !strcmp(argv[i], "-h")
			|| !strcmp(argv[i], "-?")) {
			print_usage();
			return 0;
		}
	}

	/* Parse Command-Line Arguments */
	opterr = 0;	/* turn off getopt's error message */
	while ((c = getopt(argc, argv, "adkrwil:fntc:Fu:x:o:zC:I:vh?")) != -1) {
		switch (c) {
		case 'c':	/* adjust cache sizes */
			while(optarg && *optarg) {
				if(isasciiletter((uchar)*optarg) && isupper((uchar)*optarg))
					*optarg = tolower((uchar)*optarg);
				if(*optarg == 'i') {
					INT icsz_indi=0;
					sscanf(optarg+1, SCN_INT "," SCN_INT, &csz_indi, &icsz_indi);
				}
				else if(*optarg == 'f') {
					INT icsz_fam=0;
					sscanf(optarg+1, SCN_INT "," SCN_INT, &csz_fam, &icsz_fam);
				}
				else if(*optarg == 's') {
					INT icsz_sour=0;
					sscanf(optarg+1, SCN_INT "," SCN_INT, &csz_sour, &icsz_sour);
				}
				else if(*optarg == 'e') {
					INT icsz_even=0;
					sscanf(optarg+1, SCN_INT "," SCN_INT, &csz_even, &icsz_even);
				}
				else if((*optarg == 'o') || (*optarg == 'x')) {
					INT icsz_othr=0;
					sscanf(optarg+1, SCN_INT "," SCN_INT, &csz_othr, &icsz_othr);
				}
				optarg++;
				while(*optarg && isdigit((uchar)*optarg)) optarg++;
				if(*optarg == ',') optarg++;
				while(*optarg && isdigit((uchar)*optarg)) optarg++;
			}
			break;
#ifdef FINNISH
# ifdef FINNISHOPTION
		case 'F':	/* Finnish sorting order [toggle] */
			opt_finnish = !opt_finnish;
			/*
			TODO - need to mark Finnish databases, as 
			name records are not interoperable, because of
			different soundex encoding
			2001-02-17, Perry Rapp
			TODO, 2002-11-07, Perry Rapp:
			Need to see if we can fix database so locale sort doesn't affect btree
			because it would be nicer if changing locale didn't hurt the btree !
			Perhaps locale collates can only be used inside name records ? needs research
			*/
			break;
# endif
#endif
		case 'a':	/* debug allocation */
			alloclog = TRUE;
			break;
		case 'd':	/* debug = no signal catchers */
			debugmode = TRUE;
			break;
		case 'k':	/* don't show key values */
			keyflag = FALSE;
			break;
		case 'r':	/* request for read only access */
			readonly = TRUE;
			break;
		case 'w':	/* request for write access */
			writeable = TRUE;
			break;
		case 'i': /* immutable access */
			immutable = TRUE;
			readonly = TRUE;
			break;
		case 'l': /* locking switch */
			lockchange = TRUE;
			lockarg = *optarg;
			break;
		case 'f':	/* force database open in all cases */
			forceopen = TRUE;
			break;
		case 'n':	/* use non-traditional family rules */
			traditional = FALSE;
			break;
		case 't': /* show lots of trace statements for debugging */
			prog_trace = TRUE;
			break;
		case 'u': /* specify screen dimensions */
			sscanf(optarg, SCN_INT "," SCN_INT, &winx, &winy);
			break;
		case 'x': /* execute program */
			if (!exprogs) {
				exprogs = create_list2(LISTDOFREE);
			}
			push_list(exprogs, strdup(optarg ? optarg : ""));
			break;
		case 'I': /* program arguments */
			{
				STRING optname=0, optval=0;
				parse_arg(optarg, &optname, &optval);
				if (optname && optval) {
					if (!exargs) {
						exargs = create_table_str();
					}
					insert_table_str(exargs, optname, optval);
				}
				strfree(&optname);
				strfree(&optval);
			}
			break;
		case 'o': /* output directory */
			progout = optarg;
			break;
		case 'z': /* nongraphical box */
			graphical = FALSE;
			break;
		case 'C': /* specify config file */
			configfile = optarg;
			break;
		case 'v': /* show version */
			showversion = TRUE;
			goto usage;
			break;
		case 'h': /* show usage */
		case '?': /* show usage */
			showusage = TRUE;
			showversion = TRUE;
			goto usage;
			break;
		}
	}

prompt_for_db:

	/* catch any fault, so we can close database */
	if (!debugmode)
	{
		set_signals(sighand_cursesui);
	}
	/* developer wants to drive without seatbelt! */
	else
	{
		stdstring_hardfail();
		/* yydebug = 1; */
	}

	set_displaykeys(keyflag);

	/* initialize options & misc. stuff */
	llgettext_set_default_localedir(LOCALEDIR);
	if (!init_lifelines_global(configfile, &msg, &main_db_notify)) {
		llwprintf("%s", msg);
		goto finish;
	}
	/* setup crashlog in case init_screen fails (eg, bad menu shortcuts) */
	crashlog = getlloptstr("CrashLog_llines", NULL);
	if (!crashlog) { crashlog = "CrashLog_llines.log"; }
	crash_setcrashlog(crashlog);

	/* start (n)curses and create windows */
	{
		char errmsg[512];
		if (!init_screen(errmsg, sizeof(errmsg)/sizeof(errmsg[0])))
		{
			endwin();
			fprintf(stderr, "%s", errmsg);
			goto finish;
		}
		set_screen_graphical(graphical);
	}
	init_interpreter(); /* give interpreter its turn at initialization */

	/* Validate Command-Line Arguments */
	if ((readonly || immutable) && writeable) {
		llwprintf("%s", _(qSnorwandro));
		goto finish;
	}
	if (forceopen && lockchange) {
		llwprintf("%s", _(qSnofandl));
		goto finish;
	}
	if (lockchange && lockarg != 'y' && lockarg != 'n') {
		llwprintf("%s", _(qSbdlkar));
		goto finish;
	}
	if (forceopen)
		alteration = 3;
	else if (lockchange) {
		if (lockarg == 'y')
			alteration = 2;
		else
			alteration = 1;
	}
	c = argc - optind;
	if (c > 1) {
		showusage = TRUE;
		goto usage;
	}

	/* Open database, prompting user if necessary */
	if (1) {
		STRING errmsg=0;
		if (!alldone && c>0) {
			dbrequested = strsave(argv[optind]);
		} else {
			strupdate(&dbrequested, "");
		}
		if (!select_database(&dbrequested, alteration, &errmsg)) {
			if (errmsg) {
				llwprintf("%s", errmsg);
			}
			alldone = 0;
			goto finish;
		}
	}
	strfree(&dbrequested);

	/* Start Program */
	if (!init_lifelines_postdb()) {
		llwprintf("%s", _(qSbaddb));
		goto finish;
	}
	if (!int_codeset[0]) {
		msg_info("%s", _("Warning: database codeset unspecified"));
	} else if (!transl_are_all_conversions_ok()) {
		msg_info("%s", _("Warning: not all conversions available"));
	}

	init_show_module();
	init_browse_module();
	if (exargs) {
		set_cmd_options(exargs);
		release_table(exargs);
		exargs = 0;
	}
	if (exprogs) {
		BOOLEAN picklist = FALSE;
		BOOLEAN timing = FALSE;
		interp_main(exprogs, progout, picklist, timing);
		destroy_list(exprogs);
	} else {
		alldone = 0;
		while (!alldone)
			main_menu();
	}
	term_show_module();
	term_browse_module();
	ok=TRUE;

finish:
	/* we free this not because we care so much about these tiny amounts
	of memory, but to ensure we have the memory management right */
	/* strfree frees memory & nulls pointer */
	strfree(&dbused);
	strfree(&readpath_file);
	shutdown_interpreter();
	close_lifelines();
	shutdown_ui(!ok);
	if (alldone == 2)
		goto prompt_for_db; /* changing databases */
	termlocale();

usage:
	/* Display Version and/or Command-Line Usage Help */
	if (showversion) { print_version("llines"); }
	if (showusage) puts(usage_summary);

	/* Exit */
	return !ok;
}
/*==================================
 * parse_arg -- Break argument into name & value
 *  eg, parse_arg("main_indi=I3", &a, &b)
 *   yields a="main_indi" and b="I3"
 *  (a & b are newly allocated from heap)
 *================================*/
static void
parse_arg (const char * optarg, char ** optname, char **optval)
{
	const char * ptr;
	*optname = *optval = 0;
	for (ptr = optarg; *ptr; ++ptr) {
		if (*ptr == '=') {
			char * namebuff = 0;
			char * valbuff = 0;
			INT namelen = ptr - optarg;
			if (!namelen)
				return;
			namebuff = (char *)malloc(namelen+1);
			llstrncpy(namebuff, optarg, namelen+1, 0);
			*optname = namebuff;
			valbuff = strdup(ptr+1);
			*optval = valbuff;
			return;

		}
	}
}
/*===================================================
 * shutdown_ui -- Do whatever is necessary to close GUI
 * Created: 2001/11/08, Perry Rapp
 *=================================================*/
void
shutdown_ui (BOOLEAN pause)
{
	term_screen();
	if (pause) /* if error, give user a second to read it */
		sleep(1);
	/* Terminate Curses UI */
	if (!isendwin())
		endwin();
}
/* Finnish language support modifies the soundex codes for names, so
 * a database created with this support is not compatible with other
 * databases. 
 *
 * define FINNISH for Finnish Language support
 *
 * define FINNISHOPTION to have a runtime option -F which will enable
 * 	  	Finnish language support, but the name indices will all be
 *      wrong if you make modifications whilst in the wrong mode.
 */

static void
load_usage (void)
{
#ifdef FINNISH
# ifdef FINNISHOPTION
	opt_finnish  = FALSE;/* Finnish Language sorting order if TRUE */
	usage_summary = _(qSusgFinnOpt);
# else
	opt_finnish  = TRUE;/* Finnish Language sorting order if TRUE */
	usage_summary = _(qSusgFinnAlw);
# endif
#else
	opt_finnish  = FALSE;/* Finnish Language sorting order id disabled*/
	usage_summary = _(qSusgNorm);
#endif
}
/*===============================================
 * print_usage -- display program help/usage
 *  displays to stdout
 *=============================================*/
static void
print_usage (void)
{
#ifdef WIN32
	char * exename = "Lines";
#else
	char * exename = "llines";
#endif
	print_lines_usage(exename);
}
/*==================================================
 * main_db_notify -- callback called whenever a db is
 *  opened or closed
 * Created: 2002/06/16, Perry Rapp
 *================================================*/
static void
main_db_notify (STRING db, BOOLEAN opening)
{
	/* store name away for reporting in case of crash later */
	if (opening)
		crash_setdb(db);
	else
		crash_setdb("");
}
