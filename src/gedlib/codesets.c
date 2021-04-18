/* 
   Copyright (c) 2002-2007 Perry Rapp
   "The MIT license"
   Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
/*=============================================================
 * codesets.c -- Manage the various codesets we work with
 *   Created: 2002/11 by Perry Rapp
 *==============================================================*/


#include "llstdlib.h"
#include "codesets.h"
#include "gedcom.h"
#include "lloptions.h"
#include "zstr.h"
#include "arch.h"

#ifdef HAVE_LANGINFO_CODESET
# include <langinfo.h>
#else
# include "langinfz.h"
#endif



/*********************************************
 * global/exported variables
 *********************************************/

/* internal codeset of current database */
BOOLEAN uu8=0;            /* flag if internal codeset is UTF-8 */
BOOLEAN gui8=0;            /* flag if display output encoding is UTF-8 */
STRING int_codeset=0;     /* internal codeset */

STRING editor_codeset_out=0; /* output to editor */
STRING editor_codeset_in=0;  /* input from editor */
STRING gedcom_codeset_out=0; /* output GEDCOM files */
STRING gedcom_codeset_in=0;  /* default for reading GEDCOM files */
STRING gui_codeset_in=0;     /* reading characters from GUI */
STRING gui_codeset_out=0;    /* writing characters to GUI */
STRING report_codeset_out=0; /* default for report output */
STRING report_codeset_in=0;  /* default for input from reports */


/*********************************************
 * external/imported variables
 *********************************************/

/*********************************************
 * local function prototypes
 *********************************************/

/* alphabetical */
static void set_codeset_pair(CNSTRING base, CNSTRING defval, STRING *pcsout, STRING *pcsin);

/*********************************************
 * local variables
 *********************************************/
static STRING defcodeset=0;

/*********************************************
 * local & exported function definitions
 * body of module
 *********************************************/

/*=================================================
 * init_codesets -- initialize all codeset variables
 *  config file needs to have been loaded at this point
 * Created: 2002/11/27 (Perry Rapp)
 *===============================================*/
void
init_codesets (void)
{
	STRING e=0;
#if defined(WIN32) && !defined(__CYGWIN__)
	/*
	The Win32 case is special because we care about both Windows & Console
	codepages, at least when running in console mode.
	*/
	char wincs[32];
	int n = w_get_codepage();
	snprintf(wincs, sizeof(wincs), "CP%d", n);
	strupdate(&defcodeset, wincs);
#else
	STRING defval = nl_langinfo (CODESET);
	/* nl_langinfo giving 0 on linux glibc-2.2.4-19.3 (Perry, 2002-12-01) */
	if (!defval)
		defval="";
	defval = norm_charmap(defval);
	if (!defval || !defval[0])
		defval = "ASCII";
	strupdate(&defcodeset, defval);
	/*
	We are using Markus Kuhn's emulator for systems without nl_langinfo
	see arch/langinfo.c
	An alternative would be to use localcharset.c, but it isn't as easy 
	to use; you have to configure config.aliases. Anyway, I have no idea
	if anyone needs this.
	*/
#endif

/* internal */
	/*
	internal codeset is not handled here, becauase it must be checked
	only in the database local options. It is handled in 
	update_db_options() in init.c.
	*/

/* GuiCodesetOut */
	e = getlloptstr("GuiCodesetOut", "");
	if (!e[0])
		e = getlloptstr("GuiCodeset", "");
	if (!e[0]) {
#ifdef WIN32
		char temp[32];
		int cs = (w_get_has_console() ? w_get_oemout_codepage() : w_get_codepage());
		snprintf(temp, sizeof(temp), "CP%d", cs);
		e = temp;
#else
		e = defcodeset;
#endif
	}
	strupdate(&gui_codeset_out, e);
	/* Now set the global variable gui8, which tells
	us if the display output encoding is UTF-8, for
	purposes of string length truncation */
	gui8 = is_codeset_utf8(gui_codeset_out);

/* GuiCodesetIn */
	e = getlloptstr("GuiCodesetIn", "");
	if (!e[0])
		e = getlloptstr("GuiCodeset", "");
	if (!e[0]) {
#ifdef WIN32
		char temp[32];
		int cs = (w_get_has_console() ? w_get_oemin_codepage() : w_get_codepage());
		snprintf(temp, sizeof(temp), "CP%d", cs);
		e = temp;
#else
		e = defcodeset;
#endif
	}
	strupdate(&gui_codeset_in, e);

	/* remaining codesets are all straightforward */
	set_codeset_pair("GedcomCodeset", defcodeset, &gedcom_codeset_out, &gedcom_codeset_in);
	set_codeset_pair("EditorCodeset", defcodeset, &editor_codeset_out, &editor_codeset_in);
	set_codeset_pair("ReportCodeset", defcodeset, &report_codeset_out, &report_codeset_in);

}
/*=================================================
 * set_codeset_pair -- Initialize a pair of codesets
 *  eg, GedcomCodesetOut & GedcomCodesetIn
 * Created: 2002/11/28 (Perry Rapp)
 *===============================================*/
static void
set_codeset_pair (CNSTRING base, CNSTRING defval, STRING *pcsout, STRING *pcsin)
{
	ZSTR zstr = zs_news(base);
	CNSTRING e;
	zs_apps(zstr, "Out");
	e = getlloptstr(zs_str(zstr), "");
	if (!e[0])
		e = getlloptstr(base, "");
	if (!e[0])
		e = defval;
	strupdate(pcsout, e);

	zs_sets(zstr, base);
	zs_apps(zstr, "In");
    e = getlloptstr(zs_str(zstr), "");
	if (!e[0])
		e = getlloptstr(base, "");
	if (!e[0])
		e = defval;
	strupdate(pcsin, e);
	zs_free(&zstr);
}
/*=================================================
 * term_codesets -- free all codeset variables
 * Created: 2002/11/27 (Perry Rapp)
 *===============================================*/
void
term_codesets (void)
{
	strfree(&editor_codeset_out);
	strfree(&editor_codeset_in);
	strfree(&gedcom_codeset_out);
	strfree(&gedcom_codeset_in);
	strfree(&gui_codeset_out);
	strfree(&gui_codeset_in);
	strfree(&report_codeset_out);
	strfree(&report_codeset_in);
	strfree(&defcodeset);
}
/*=================================================
 * get_defcodeset -- Return user's default codeset
 *===============================================*/
CNSTRING
get_defcodeset (void)
{
	return defcodeset;
}
