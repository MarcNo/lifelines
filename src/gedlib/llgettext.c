/* 
   Copyright (c) 2000-2005 Perry Rapp
   "The MIT license"
   Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
/*=============================================================
 * llgettext.c -- Some gettext related functions
 *==============================================================*/

#include "llstdlib.h"
#include "gedcom.h"
#include "gedcomi.h"
#include "codesets.h"
#include "lloptions.h"
#include "zstr.h"


/*********************************************
 * local variables
 *********************************************/

static TABLE gt_localeDirs = NULL; /* most recent */ /* leaks */
static STRING gt_codeset = 0; /* codeset passed to bind_textdomain_codeset */
static STRING gt_defLocaleDir = 0; /* compiled default */ /* leak */

/*********************************************
 * local & exported function definitions
 * body of module
 *********************************************/

/*==================================================
 * llgettext_set_default_localedir -- save LOCALEDIR
 *================================================*/
void
llgettext_set_default_localedir (CNSTRING localeDir)
{
	strupdate(&gt_defLocaleDir, localeDir);
}
/*==================================================
 * llgettext_init -- initialize gettext with initially
 * desired codeset. This may be changed later by user
 * options, but  this is initial best guess.
 *  domain:  [IN]  package domain (eg, "lifelines")
 *  codeset: [IN]  codeset to use
 *================================================*/
void
llgettext_init (CNSTRING domain, CNSTRING codeset)
{
#if ENABLE_NLS
	/* until we have an internal codeset (which is until we open a database)
	we want output in display codeset */
	set_gettext_codeset(domain, codeset);

	update_textdomain_localedir(domain, "Ui");

#else /* ENABLE_NLS */
	domain = domain;
	codeset = codeset;
#endif /* ENABLE_NLS */
}
/*==================================================
 * llgettext_term -- cleans up memory allocation
 * that may have been performed in llgettext_init
 *================================================*/
void
llgettext_term (void)
{
#if ENABLE_NLS
	if (gt_localeDirs) {
		destroy_table(gt_localeDirs);
		gt_localeDirs = 0;
	}
	strfree(&gt_codeset);
#endif
	strfree(&gt_defLocaleDir);
}

/*==================================================
 * update_textdomain_localedir --
 *  call bindtextdomain with current localedir
 *  domain:  [IN] package domain (eg, "lifelines")
 *  prefix:  [IN] "Ui" or "Rpt"
 *================================================*/
void
update_textdomain_localedir (CNSTRING domain, CNSTRING prefix)
{
	STRING newLocaleDir = 0;
	char keyname[30] = ""; /* eg, "UiLocaleDir" */
	/* allow run-time specification of locale directory */

	/* Default to compile-time specified (LOCALEDIR) */
	newLocaleDir = gt_defLocaleDir;

	/* Check for config setting for LocaleDir */
	newLocaleDir = getlloptstr("LocaleDir", newLocaleDir);

	/* Check for Ui or Rpt specific setting, eg, UiLocaleDir */
	if (prefix && prefix[0]) {
		snprintf(keyname, sizeof(keyname), "%sLocaleDir", prefix);
		newLocaleDir = getlloptstr(keyname, newLocaleDir);
	}

	if (newLocaleDir) {
		/* ll_bindtextdomain is caching; it will only submit
		real changes to gettext version */
		ll_bindtextdomain(domain, newLocaleDir);
	}
}
/*=================================
 * ll_bindtextdomain -- interceptor for bindtextdomain calls
 *  to send ui callbacks
 *===============================*/
void
ll_bindtextdomain (CNSTRING domain, CNSTRING localeDir)
{
#if ENABLE_NLS
	STRING oldLocaleDir = 0;

	if (!gt_localeDirs) {
		gt_localeDirs = create_table_str();
	}
	/* skip if already set */
	oldLocaleDir = valueof_str(gt_localeDirs, domain);
	if (eqstr_ex(oldLocaleDir, localeDir))
		return;
	insert_table_str(gt_localeDirs, domain, localeDir);

	bindtextdomain(domain, localeDir);
	locales_notify_language_change();
#endif /* ENABLE_NLS */
}
/*=================================
 * init_win32_gettext_shim -- 
 *  Handle user-specified iconv dll path
 *===============================*/
void
init_win32_gettext_shim (void)
{
#if ENABLE_NLS
#ifdef WIN32_INTL_SHIM
	STRING e;
	/* (re)load gettext dll if specified */
	e = getlloptstr("gettext.path", "");
	if (e && *e)
	{
		if (intlshim_set_property("dll_path", e))
		{
			/* clear cache of bindtextdomain calls */
			if (gt_localeDirs) {
				destroy_table(gt_localeDirs);
				gt_localeDirs = 0;
			}
			ll_bindtextdomain(PACKAGE, LOCALEDIR);
			textdomain(PACKAGE);
		}
		/* tell gettext where to find iconv */
		e = getlloptstr("iconv.path", "");
		if (e && *e)
			gt_set_property("iconv_path", e);
	}
	/*
	We could be more clever, and if our iconv_path is no good, ask gettext
	if it found iconv, but that would make this logic tortuous due to our having
	different shim macros (we'd have to save gettext's iconv path before setting it,
	in case ours is bad & its is good).
	*/
#endif
#endif
}
/*=================================
 * set_gettext_codeset -- Tell gettext what codeset we want
 * Created: 2002/11/28 (Perry Rapp)
 *===============================*/
void
set_gettext_codeset (CNSTRING domain, CNSTRING codeset)
{
#if ENABLE_NLS
	if (eqstr_ex(gt_codeset, codeset))
		return;
	if (codeset && codeset[0]) {
		ZSTR zcsname=zs_new();
		/* extract just the codeset name, without any subcodings */
		/* eg, just "UTF-8" out of "UTF-8//TrGreekAscii//TrCyrillicAscii" */
		transl_parse_codeset(codeset, zcsname, 0);
		if (zs_str(zcsname)) {
			strupdate(&gt_codeset, zs_str(zcsname));
			/* gettext automatically appends //TRANSLIT */
		} else {
			/* what do we do if they gave us an empty one ? */
			strupdate(&gt_codeset, "ASCII");
		}
		zs_free(&zcsname);
	} else {
		/* 
		We need to set some codeset, in case it was set to 
		UTF-8 in last db 
		*/
		strupdate(&gt_codeset, gui_codeset_out);
	}
	bind_textdomain_codeset(domain, gt_codeset);
	if (eqstr(domain, PACKAGE))
		locales_notify_uicodeset_changes();
#else
	domain = domain; /* unused */
	codeset = codeset; /* unused */
#endif /* ENABLE_NLS */
}
/*=================================
 * get_gettext_codeset -- Return last codeset passed to bind_textdomain_codeset
 * (returns null if none, otherwise pointer to private string)
 *===============================*/
CNSTRING
get_gettext_codeset (void)
{
	return gt_codeset;
}
