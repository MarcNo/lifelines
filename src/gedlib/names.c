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
/* modified 05 Jan 2000 by Paul B. McBride (pmcbride@tiac.net) */
/*=============================================================
 * names.c -- Handle name values and name indexing
 * Copyright(c) 1992-94 by T.T. Wetmore IV; all rights reserved
 *   2.3.4 - 24 Jun 93    2.3.5 - 02 Sep 93
 *   3.0.0 - 13 Sep 94    3.0.2 - 31 Dec 94
 *===========================================================*/

#include "llstdlib.h"
#include "table.h"
#include "btree.h"
#include "translat.h"
#include "gedcom.h"
#include "gedcomi.h"

/*********************************************
 * external/imported variables
 *********************************************/

extern BOOLEAN opt_finnish;
extern BTREE BTR;

/*********************************************
 * local function prototypes
 *********************************************/

static INT sxcodeof(int);
static BOOLEAN exactmatch(STRING, STRING);
static STRING parts_to_name(STRING*);
static void name_to_parts(STRING, STRING*);
static RKEY name2rkey(STRING);
static STRING upsurname(STRING);
static void squeeze(STRING, STRING);
static STRING nextpiece(STRING);
static void cmpsqueeze(STRING, STRING);

/*********************************************
 * local variables
 *********************************************/

static INT oldsx = 0;

/*===================================================================
 * name records -- Name indexing information is kept in the database
 *   in name records; all persons with the same SOUNDEX code and the
 *   same first letter in their first given name, are indexed
 *   together
 *===================================================================
 * database record format -- The first INT of the record holds the
 *   number of names indexed in the record
 *-------------------------------------------------------------------
 *        1 INT  nnames  - number of names indexed in this record
 *   nnames RKEY rkeys   - RKEYs of the INDI records with the names
 *   nnames INT  noffs   - offsets into following strings where names
 *			   begin
 *   nnames STRING names - char buffer where the names are stored
 *			   based on char offsets
 *-------------------------------------------------------------------
 * internal format -- At any time there can be only one name record
 *   stored internally; the data is stored in global data structures
 *-------------------------------------------------------------------
 *   RKEY    NRkey   - RKEY of the current name record
 *   STRING  NRrec   - current name record
 *   INT     NRsize  - size of current name record
 *   INT     NRcount - number of entries in current name record
 *   INT    *NRoffs  - char offsets to names in current name
 *			  record
 *   RKEY   *NRkeys  - RKEYs of the INDI records with the names
 *   STRING *NRnames - name values from INDI records that the
 *			  index is based upon
 *   INT     NRmax   - max allocation size of internal arrays
 *-------------------------------------------------------------------
 * When a name record is used to match a search name, the internal
 *   structures are modified to remove all entries that don't match
 *   the name; in addition, other global data structures are used
 *-------------------------------------------------------------------
 *   STRING *LMkeys  - keys (strings) of all INDI records that match
 *   INT     LMcount - number of entries in modified record arrays
 *   INT     LMmax   - max allocation size of LMkeys array
 *=================================================================*/

static RKEY    NRkey;
static STRING  NRrec = NULL;
static INT     NRsize;
static INT     NRcount;
static INT    *NRoffs;
static RKEY   *NRkeys;
static STRING *NRnames;
static INT     NRmax = 0;

static STRING *LMkeys = NULL;
static INT     LMcount = 0;
static INT     LMmax = 0;

/*********************************************
 * local function definitions
 * body of module
 *********************************************/

/*====================================================
 * parsenamerec -- Store name rec in file buffers
 *==================================================*/
static void
parsenamerec (RKEY rkey, STRING p)
{
	INT i;
	NRkey = rkey;
/* Store name record in data structures */
	memcpy (&NRcount, p, sizeof(INT));
	ASSERT(NRcount < 1000000); /* 1000000 names in a given slot ? */
	p += sizeof(INT);
	if (NRcount >= NRmax - 1) {
		if (NRmax != 0) {
			stdfree(NRkeys);
			stdfree(NRoffs);
			stdfree(NRnames);
		}
		NRmax = NRcount + 10;
		NRkeys = (RKEY *) stdalloc((NRmax)*sizeof(RKEY));
		NRoffs = (INT *) stdalloc((NRmax)*sizeof(INT));
		NRnames = (STRING *) stdalloc((NRmax)*sizeof(STRING));
	}
	for (i = 0; i < NRcount; i++) {
		memcpy(&NRkeys[i], p, sizeof(RKEY));
		p += sizeof(RKEY);
	}
	for (i = 0; i < NRcount; i++) {
		memcpy(&NRoffs[i], p, sizeof(INT));
		p += sizeof(INT);
	}
	for (i = 0; i < NRcount; i++)
		NRnames[i] = p + NRoffs[i];
}
/*====================================================
 * getnamerec -- Read name record and store in file buffers
 *==================================================*/
static BOOLEAN
getnamerec (STRING name)
{
	STRING p;
/* Convert name to key and read name record */
	NRkey = name2rkey(name);
	if (NRrec) stdfree(NRrec);
	p = NRrec = getrecord(BTR, NRkey, &NRsize);
	if (!NRrec) {
		NRcount = 0;
		if (NRmax == 0) {
			NRmax = 10;
			NRkeys = (RKEY *) stdalloc(10*sizeof(RKEY));
			NRoffs = (INT *) stdalloc(10*sizeof(INT));
			NRnames = (STRING *) stdalloc(10*sizeof(STRING));
		}
		return FALSE;
	}
	parsenamerec(NRkey, p);
	return TRUE;
}
/*============================================
 * name2rkey - Convert name to name record key
 *==========================================*/
static RKEY
name2rkey (STRING name)
{
	RKEY rkey;
	STRING sdex = soundex(getsxsurname(name));
	char finitial = getfinitial(name);
	rkey.r_rkey[0] = rkey.r_rkey[1] = ' ';
	rkey.r_rkey[2] = 'N';
	rkey.r_rkey[3] = finitial;
	rkey.r_rkey[4] = *sdex++;
	rkey.r_rkey[5] = *sdex++;
	rkey.r_rkey[6] = *sdex++;
	rkey.r_rkey[7] = *sdex;
	return rkey;
}
/*=======================================
 * name_lo - Lower limit for name records
 *=====================================*/
static RKEY
name_lo (void)
{
	RKEY rkey;
	INT i;
	for (i=0; i<8; i++)
		rkey.r_rkey[i] = ' ';
	rkey.r_rkey[2] = 'N';
	return rkey;
}
/*=======================================
 * name_hi - Upper limit for name records
 *=====================================*/
static RKEY
name_hi (void)
{
	RKEY rkey;
	INT i;
	for (i=0; i<8; i++)
		rkey.r_rkey[i] = ' ';
	rkey.r_rkey[2] = 'O';
	return rkey;
}
/*======================================================
 * getsurname_impl -- Implement getsxsurname & getasurname
 *  returns static buffer, cycling through 3 such
 *  name:    [in] full name to search for surname
 *  soundex: [in] flag if doing soundex
 * The soundex flag is because soundex doesn't test
 *  the first letter, it wants surname to do it
 *
 * TODO: convert to Unicode
 *  But may need to disambiguate name index use
 *  b/c that needs an isletter that is locale-independent
 *====================================================*/
static STRING
getsurname_impl (STRING name, BOOLEAN soundex)
{
	INT c;
	static char buffer[3][MAXLINELEN+1];
	static INT dex = 0;
	STRING p, surname;
	if (++dex > 2) dex = 0;
	p = surname = buffer[dex];
	while ((c = (uchar)*name++) && c != NAMESEP)
		;
	if (c == 0) return (STRING) "____";
	while (iswhite(c = (uchar)*name++))
		;
	if (c == 0 || c == NAMESEP) return (STRING) "____";
	if (soundex && !isletter(c)) return (STRING) "____";
	*p++ = c;
	while ((c = (uchar)*name++) && c != NAMESEP)
		*p++ = c;
	*p = 0;
	return surname;
}
/*=============================
 * getsxsurname -- Return surname for soundex
 *  returns static buffer
 * This funtion returns ____ if first non-white character
 * surname is not a letter.
 *===========================*/
STRING
getsxsurname (STRING name)        /* GEDCOM name */
{
	return getsurname_impl(name, TRUE);
}
/*=============================
 * getasurname -- Return a surname 
 * This should generally be used for surnames.
 * The alternative is getsxsurname above.
 *  returns static buffer
 *===========================*/
STRING
getasurname (STRING name)   /* GEDCOM name */
{
	return getsurname_impl(name, FALSE);
}
/*============================================
 * getfinitial -- Return first initial of name
 *  name:  [in] GEDCOM name
 *  TODO: convert to Unicode
 *==========================================*/
INT
getfinitial (STRING name)
{
	INT c;
	while (TRUE) {
		while (iswhite(c = (uchar)*name++))
			;
		if (isletter(c)) return ll_toupper(c);
		if (c == 0) return '$';
		if (c != NAMESEP) return '$';
		while ((c = (uchar)*name++) && c != NAMESEP)
			;
		if (c == 0) return '$';
	}
}
/*========================================
 * soundex -- Return name's SOUNDEX code.
 *  returns static buffer
 *======================================*/
STRING
soundex (STRING name)   /* surname */
{
	static char scratch[6];
	STRING p = name, q = scratch;
	INT c, i, j;
	if (!name || !name[0] || eqstr(name, "____"))
		return (STRING) "Z999";
	/* always copy first letter directly */
	*q++ = ll_toupper((uchar)*p++);
	i = 1;
	oldsx = 0;
	while (*p && (c = ll_toupper((uchar)*p++)) && i < 4) {
		if ((j = sxcodeof(c)) == 0) continue;
		*q++ = j;
		i++;
	}
	while (i < 4) {
		*q++ = '0';
		i++;
	}
	*q = 0;
	return scratch;
}
/*========================================
 * sxcodeof -- Return letter's SOUNDEX code.
 *  letter:  should be capitalized letter
 * returns soundex code, or 0 if not coded
 * Also returns 0 if same as last call (uses static oldsx variable).
 * Note that Finnish version uses a different SOUNDEX
 * scheme here, making databases (name indices)
 * not portable between Finnish & normal LifeLines.
 *======================================*/
static INT
sxcodeof (int letter)
{
	int newsx = 0;

	if(opt_finnish) {
	/* Finnish Language */
		switch (letter) {
		case 'B': case 'P': case 'F': case 'V': case 'W':
			newsx = '1'; break;
		case 'C': case 'S': case 'K': case 'G': case '\337':
		case 'J': case 'Q': case 'X': case 'Z': case '\307':
			newsx = '2'; break;
		case 'D': case 'T': case '\320': case '\336':
			newsx = '3'; break;
		case 'L':
			newsx = '4'; break;
		case 'M': case 'N': case '\321':
			newsx = '5'; break;
		case 'R':
			newsx = '6'; break;
		default:	/* new stays zero */
			break;
		}
	} else {
		/* English Language (Default) */
		switch (letter) {
		case 'B': case 'P': case 'F': case 'V':
			newsx = '1'; break;
		case 'C': case 'S': case 'K': case 'G':
		case 'J': case 'Q': case 'X': case 'Z':
			newsx = '2'; break;
		case 'D': case 'T':
			newsx = '3'; break;
		case 'L':
			newsx = '4'; break;
		case 'M': case 'N':
			newsx = '5'; break;
		case 'R':
			newsx = '6'; break;
		default:	/* new stays zero */
			break;
		}
	}
  
	if (newsx == 0) {
		oldsx = 0;
		return 0;
	}
	if (newsx == oldsx) return 0;
	oldsx = newsx;
	return newsx;
}
/*=========================================
 * add_name -- Add new entry to name record
 *  name:  [in] person's name
 *  key:   [in] person's INDI key
 *=======================================*/
BOOLEAN
add_name (STRING name, STRING key)
{
	STRING rec, p;
	INT i, len, off;
	RKEY rkey;
	rkey = str2rkey(key);
	(void) getnamerec(name);
	for (i = 0; i < NRcount; i++) {
		if (!ll_strncmp(rkey.r_rkey, NRkeys[i].r_rkey, 8) &&
		    eqstr(name, NRnames[i]))
			return TRUE;
	}
	NRkeys[NRcount] = rkey;
	NRnames[NRcount] = name;
	NRcount++;
	p = rec = (STRING) stdalloc(NRsize + sizeof(RKEY) +
	    sizeof(INT) + strlen(name) + 10);
	len = 0;
	memcpy(p, &NRcount, sizeof(INT));
	p += sizeof(INT);
	len += sizeof(INT);
	for (i = 0; i < NRcount; i++) {
		memcpy(p, &NRkeys[i], sizeof(RKEY));
		p += sizeof(RKEY);
		len += sizeof(RKEY);
	}
	off = 0;
	for (i = 0; i < NRcount; i++) {
		memcpy(p, &off, sizeof(INT));
		p += sizeof(INT);
		len += sizeof(INT);
		off += strlen(NRnames[i]) + 1;
	}
	for (i = 0; i < NRcount; i++) {
		memcpy(p, NRnames[i], strlen(NRnames[i]) + 1);
		p += strlen(NRnames[i]) + 1;
		len += strlen(NRnames[i]) + 1;
	}
	addrecord(BTR, NRkey, rec, len);
	stdfree(rec);
	return TRUE;
}
/*=============================================
 * remove_name -- Remove entry from name record
 *  name: [in] person's name
 *  key:  [in] person's INDI key
 *===========================================*/
BOOLEAN
remove_name (STRING name, STRING key)
{
	STRING rec, p;
	INT i, len, off;
	BOOLEAN found;
	RKEY rkey;
	rkey = str2rkey(key);
	(void) getnamerec(name);
	found = FALSE;
	for (i = 0; i < NRcount; i++) {
		if (!ll_strncmp(rkey.r_rkey, NRkeys[i].r_rkey, 8) &&
			eqstr(name, NRnames[i])) {
			found = TRUE;
			break;
		}
	}
	if (!found) return FALSE;
	NRcount--;
	for ( ; i < NRcount; i++) {
		NRkeys[i] = NRkeys[i+1];
		NRnames[i] = NRnames[i+1];
	}
	p = rec = (STRING) stdalloc(NRsize);
	len = 0;
	memcpy(p, &NRcount, sizeof(INT));
	p += sizeof(INT);
	len += sizeof(INT);
	for (i = 0; i < NRcount; i++) {
		memcpy(p, &NRkeys[i], sizeof(RKEY));
		p += sizeof(RKEY);
		len += sizeof(RKEY);
	}
	off = 0;
	for (i = 0; i < NRcount; i++) {
		memcpy(p, &off, sizeof(INT));
		p += sizeof(INT);
		len += sizeof(INT);
		off += strlen(NRnames[i]) + 1;
	}
	for (i = 0; i < NRcount; i++) {
		memcpy(p, NRnames[i], strlen(NRnames[i]) + 1);
		p += strlen(NRnames[i]) + 1;
		len += strlen(NRnames[i]) + 1;
	}
	addrecord(BTR, NRkey, rec, len);
	stdfree(rec);
	return TRUE;
}
#if 0
/*===============================================
 * replace_name -- Replace entry in name records.
 *=============================================*/
BOOLEAN
replace_name (STRING old,       /* person's old name */
              STRING new,       /* person's new name */
              STRING key)       /* person's INDI key */
{
	remove_name(old, key);
	add_name(new, key);
	return TRUE;
}
#endif
/*=========================================================
 * exactmatch -- Check if first name is contained in second
 *  partial:  [in] name from user
 *  complete: [in] GEDCOM name
 *=======================================================*/
static BOOLEAN
exactmatch (STRING partial, STRING complete)
{
	char part[MAXGEDNAMELEN+2], comp[MAXGEDNAMELEN+2], *p, *q;
	BOOLEAN okay;

	if (strlen(partial) > MAXGEDNAMELEN || strlen(complete) > MAXGEDNAMELEN)
		return FALSE;
	squeeze(partial, part);
	squeeze(complete, comp);
	q = comp;
	for (p = part; *p; p += strlen(p) + 1) {
		okay = FALSE;
		for (; !okay && *q; q += strlen(q) + 1) {
			if (piecematch(p, q)) okay = TRUE;
		}
		if (!okay) return FALSE;
	}
	return TRUE;
}
/*================================================================
 * piecematch -- Match partial word with complete; must begin with
 *   same letter; letters in partial must be in same order as in
 *   complete; case insensitive
 *==============================================================*/
BOOLEAN
piecematch (STRING part,
            STRING comp)
{
	if(opt_finnish) {
	  if(fi_chrcmp(*part++, *comp++) != 0) return FALSE;
	} else {
	  if (*part++ != *comp++) return FALSE;
	}
	while (*part && *comp) {
		if(opt_finnish) {
			if (fi_chrcmp(*part, *comp++) == 0) part++;
		} else {
			if (*part == *comp++) part++;
		}
	}
	return *part == 0;
}
/*===============================================================
 * squeeze -- Squeeze string to superstring, string of uppercase,
 *   0-terminated words, ending with another 0
 * Ignores ASCII non-letters
 *   eg., `Anna /Van Cott/' maps to `ANNA\0VANCOTT\0\0'.
 *  in:   [in] string of words
 *  out:  [out] superstring of words
 *=============================================================*/
static void
squeeze (STRING in, STRING out)
{
	INT c;
	while ((c = (uchar)*in++) && c<128 && chartype(c) != LETTER)
		;
	if (c == 0) {
		*out++ = 0; *out = 0;
		return;
	}
	while (TRUE) {
		*out++ = ll_toupper(c);
		while ((c = (uchar)*in++) && c != NAMESEP && chartype(c) != WHITE) {
			if (chartype(c) == LETTER || c>127) *out++ = ll_toupper(c);
		}
		if (c == 0) {
			*out++ = 0; *out = 0;
			return;
		}
		*out++ = 0;
		while ((c = (uchar)*in++) && c<128 && chartype(c) != LETTER)
			;
		if (c == 0) {
			*out++ = 0; *out = 0;
			return;
		}
	}
}
/*====================================================
 * get_names -- Find all persons who match name or key
 *  name:  [in] name of person desired
 *  pnum:  [out] number of matches
 *  pkeys: [out] keys matched (0...*pnum-1)
 *  exact: [in] unused!
 *==================================================*/
void
get_names (STRING name, INT *pnum, STRING **pkeys, BOOLEAN exact)
{
	INT i, n;
	RECORD rec;
	static char kbuf[MAXGEDNAMELEN];
	static STRING kaddr;

	exact = FALSE;		/* keep compiler happy */

   /* See if user is asking for person by key instead of name */
	if ((rec = id_by_key(name, 'I'))) {
		STRING key = rmvat(nxref(nztop(rec)));
		llstrncpy(kbuf, key, sizeof(kbuf), uu8);
		kaddr = kbuf;
		*pkeys = &kaddr;
		*pnum = 1;
		return;
	}

   /* Clean up allocated memory from last call */
	if (LMcount) {
		for (i = 0; i < LMcount; i++)
			stdfree(LMkeys[i]);
	}

   /* Load up static name buffers; return if no match */
	LMcount = 0;
	if (!getnamerec(name)) {
		*pnum = 0;
		return;
	}

   /* Compare user's name against all names in name record; the name
      record data structures are modified */
	n = 0;
	for (i = 0; i < NRcount; i++) {
		if (exactmatch(name, NRnames[i])) {
			if (i != n) {
				NRnames[n] = NRnames[i];
				NRkeys[n] = NRkeys[i];
			}
			n++;
		}
	}
	*pnum = NRcount = n;
	if (NRcount > LMmax) {
		if (LMmax) stdfree(LMkeys);
		LMkeys = (STRING *) stdalloc(NRcount*sizeof(STRING));
		LMmax = NRcount;
	}
	for (i = 0; i < NRcount; i++)
		LMkeys[i] = strsave(rkey2str(NRkeys[i]));
	*pkeys = LMkeys;
}
/*====================================
 * namecmp -- Compare two GEDCOM names
 * used for by indiseq's name_compare, which is used
 * by indiseq_namesort
 *==================================*/
int
namecmp (STRING name1, STRING name2)
{
	char sqz1[MAXGEDNAMELEN], sqz2[MAXGEDNAMELEN];
	STRING p1 = sqz1, p2 = sqz2;
	STRING sur1 = getsxsurname(name1);
	STRING sur2 = getsxsurname(name2);
	INT r = cmpstrloc(sur1, sur2);
	if (r) return r;
	if(opt_finnish) {
	  r = fi_chrcmp(getfinitial(name1),  getfinitial(name2));
	} else {
	  r = getfinitial(name1) - getfinitial(name2);
	}
	if (r) return r;
	cmpsqueeze(name1, p1);
	cmpsqueeze(name2, p2);
	while (*p1 && *p2) {
		r = cmpstrloc(p1, p2);
		if (r) return r;
		p1 += strlen(p1) + 1;
		p2 += strlen(p2) + 1;
	}
	if (*p1) return 1;
	if (*p2) return -1;
	return 0;
}
/*===========================================================
 * cmpsqueeze -- Squeeze GEDCOM name to superstring of givens
 *  in:  [in] input string
 *  out: [out] output string
 *=========================================================*/
void
cmpsqueeze (STRING in, STRING out)
{
	INT c;
	while ((in = nextpiece(in))) {
		while (TRUE) {
			c = (uchar)*in++;
			if (iswhite(c) || c == NAMESEP || c == 0) {
				*out++ = 0;
				--in;
				break;
			}
			*out++ = c;
		}
	}
	*out = 0;
}
/*=====================================
 * givens -- Return given names of name
 *  returns static buffer
 *===================================*/
STRING
givens (STRING name)
{
	INT c;
	static char scratch[MAXGEDNAMELEN+1];
	STRING out = scratch;
	while ((name = nextpiece(name))) {
		while (TRUE) {
			if ((c = (uchar)*name++) == 0) {
				if (*(out-1) == ' ') --out;
				*out = 0;
				return scratch;
			}
			if (iswhite(c) || c == NAMESEP) {
				*out++ = ' ';
				--name;
				break;
			}
			*out++ = c;
		}
	}
	if (*(out-1) == ' ') --out;
	*out = 0;
	return scratch;
}
/*========================================
 * nextpiece -- Return next givenname in string
 *  skip over surname (enclosed in NAMESEP)
 *======================================*/
static STRING
nextpiece (STRING in)
{
	int c;
	while (TRUE) {
		while (iswhite(c = (uchar)*in++))
			;
		if (c == 0) return NULL;
		if (c != NAMESEP) return --in;
		while ((c = (uchar)*in++) && c != NAMESEP)
			;
		if (c == 0) return NULL;
	}
}
/*===================================================================
 * trim_name -- Trim GEDCOM name to less or equal to given length but
 *   not shorter than first initial and surname
 *=================================================================*/
#define MAXPARTS 100
STRING
trim_name (STRING name, INT len)
{
	STRING parts[MAXPARTS];
	INT i, sdex = -1, nparts;
	name_to_parts(name, parts);
	name = parts_to_name(parts);
	if ((INT)strlen(name) <= len + 2) return name;
	for (i = 0; i < MAXPARTS; i++) {
		if (!parts[i]) break;
		if (*parts[i] == NAMESEP) sdex = i;
	}
	nparts = i;
	if (sdex == -1) sdex = nparts; /* can't assume a surname was found */
	for (i = sdex-1; i >= 0; --i) {
		/* chop to initial */
		/* TODO: This doesn't handle composition */
		if (uu8) {
			INT wid = utf8len(parts[i][0]);
			if (wid>1) {
				INT len = strlen(parts[i]);
				if (wid > len) wid = len;
			}
			parts[i][wid] = 0;
		} else {
			parts[i][1] = 0;
		}
		name = parts_to_name(parts);
		if ((INT)strlen(name) <= len + 2) return name;
	}
	for (i = sdex-1; i >= 1; --i) {
		parts[i] = NULL;
		name = parts_to_name(parts);
		if ((INT)strlen(name) <= len + 2) return name;
	}
	for (i = nparts-1; i > sdex; --i) {
		parts[i] = NULL;
		name = parts_to_name(parts);
		if ((INT)strlen(name) <= len + 2) return name;
	}
	return name;
}
/*============================================================
 * name_to_parts -- Convert GEDCOM name to parts; keep slashes
 * name:  [in] name from database (after translation perhaps)
 * parts: [out] array of pointers to parts, zero-terminated 
 *              & stored in local static buffer
 *==========================================================*/
static void
name_to_parts (STRING name, STRING *parts)
{
	static char scratch[MAXGEDNAMELEN+1];
	STRING p = scratch;
	INT c, i = 0;
	ASSERT(strlen(name) <= MAXGEDNAMELEN);
	for (i = 0; i < MAXPARTS; i++)
		parts[i] = NULL;
	i = 0;
	while (TRUE) {
		while (iswhite(c = (uchar)*name++))
			;
		if (c == 0) return;
		ASSERT(i < MAXPARTS);
		parts[i++] = p;
		*p++ = c;
		if (c == NAMESEP) {
			while ((c = *p++ = (uchar)*name++) && c != NAMESEP)
				;
			if (c == 0) return;
			*p++ = 0;
		} else {
			while ((c = (uchar)*name++) && !iswhite(c) && c != NAMESEP)
				*p++ = c;
			*p++ = 0;
			if (c == 0) return;
			if (c == NAMESEP) name--;
		}
	}
}
/*======================================================
 * parts_to_name -- Convert list of parts back to string
 *====================================================*/
static STRING
parts_to_name (STRING *parts)
{
	INT i;
	static char scratch[MAXGEDNAMELEN+1];
	STRING p = scratch;
	for (i = 0; i < MAXPARTS; i++) {
		if (!parts[i]) continue;
		strcpy(p, parts[i]);
		p += strlen(parts[i]);
		*p++ = ' ';
	}
	if (*(p - 1) == ' ')
		*(p - 1) = 0;
	else
		*p = 0;
	return scratch;
}
/*=======================================================
 * upsurname -- Convert GEDCOM surname name to upper case
 *=====================================================*/
static STRING
upsurname (STRING name)
{
	static char scratch[MAXGEDNAMELEN+1];
	STRING p = scratch;
	INT c;
	while ((c = *p++ = (uchar)*name++) && c != NAMESEP)
		;
	if (c == 0) return scratch;
	while ((c = (uchar)*name++) && c != NAMESEP)
		*p++ = ll_toupper(c);
	*p++ = c;
	if (c == 0) return scratch;
	while ((c = *p++ = (uchar)*name++))
		;
	return scratch;
}
/*==================================================
 * manip_name - Convert GEDCOM name to various forms
 *  with codeset conversion
 *  name:    [IN] name
 *  caps:    [IN] make surname into caps?
 *  regorder [IN] regular order? (not surname first)
 *  len:     [IN] max name length
 * returns static buffer
 *================================================*/
STRING
manip_name (STRING name, BOOLEAN caps, BOOLEAN regorder, INT len)
{
	static char scratch[MAXGEDNAMELEN+1];
	if (!name || *name == 0) return NULL;
	llstrsets(scratch, sizeof(scratch), uu8, name);
	name = scratch;
	if (caps) name = upsurname(name);
	name = trim_name(name, regorder ? len: len-1);
	if (regorder) return trim(name_string(name), len);
	return trim(name_surfirst(name), len);
}
/*===============================================
 * name_string -- Remove slashes from GEDCOM name
 *=============================================*/
STRING
name_string (STRING name)
{
	static char scratch[MAXGEDNAMELEN+1];
	STRING p = scratch;
	ASSERT(strlen(name) <= MAXGEDNAMELEN);
	while (*name) {
		if (*name != NAMESEP) *p++ = *name;
		name++;
	}
	*p-- = 0;
	striptrail(scratch);
	return scratch;
}
/*==========================================================
 * name_surfirst - Convert GEDCOM name to surname first form
 *========================================================*/
STRING
name_surfirst (STRING name)
{
	static char scratch[MAXGEDNAMELEN+1];
	STRING p = scratch;
	ASSERT(strlen(name) <= MAXGEDNAMELEN);
	strcpy(p, getasurname(name));
	p += strlen(p);
	strcpy(p, ", ");
	p += strlen(p);
	strcpy(p, givens(name));
	return scratch;
}
/*================================
 * id_by_key -- Find record (if exists) from key
 *  name:  [IN]  name to search for
 *  ctype: [IN]  type of record (eg, 'I') (0 for any)
 * returns record found, or NULL
 *==============================*/
RECORD
id_by_key (STRING name, char ctype)
{
	STRING p = name; /* unsigned for chartype */
	static char kbuf[MAXGEDNAMELEN];
	INT i = 0, c;
	while ((c = (uchar)*p++) && chartype(c) == WHITE)
		;
	if (c == 0) return NULL;
	/* Is name as key compatible with requirement (ctype) ? */
	/* A less long-winded way to do this would be nice */
	switch(c) {
	case 'I':
	case 'i':
		if (ctype && ctype != 'I') return NULL;
		ctype = 'I';
		break;
	case 'F':
	case 'f':
		if (ctype && ctype != 'F') return NULL;
		ctype = 'F';
		break;
	case 'S':
	case 's':
		if (ctype && ctype != 'S') return NULL;
		ctype = 'S';
		break;
	case 'E':
	case 'e':
		if (ctype && ctype != 'E') return NULL;
		ctype = 'E';
		break;
	case 'X':
	case 'x':
		if (ctype && ctype != 'X') return NULL;
		ctype = 'X';
		break;
	default:
		/* can't match numeric key without knowing what type */
		if (!ctype) return NULL;
		--p; /* back up because first character must be first digit */
		break;
	}
	/* now we know what type, and p should point to first digit */
	c = (uchar)*p++;
	if (chartype(c) != DIGIT) return NULL;
	kbuf[i++] = ctype;
	kbuf[i++] = c;
	while ((c = (uchar)*p++) && chartype(c) == DIGIT)
		kbuf[i++] = c;
	if (c != 0) return NULL;
	kbuf[i] = 0;
	return qkey_to_record(kbuf);
}
/*============================================
 * name_to_list -- Convert name to string list
 *  name:  [IN]  GEDCOM name
 *  list:  [I/O] list (must exist)
 *  plen:  [OUT]  returned length
 *  psind: [OUT]  index (rel 1) of surname in list
 * Returns a list whose elements are strings
 *==========================================*/
BOOLEAN
name_to_list (STRING name, LIST list, INT *plen, INT *psind)
{
	INT i;
	STRING str;
	STRING parts[MAXPARTS];
	if (!name || *name == 0 || !list) return FALSE;
	make_list_empty(list);
	set_list_type(list, LISTDOFREE);
	*psind = 0;
	name_to_parts(name, parts);
	for (i = 0; i < MAXPARTS; i++) {
		if (!parts[i]) break;
		if (*parts[i] == NAMESEP) {
			*psind = i + 1;
			str = strsave(parts[i] + 1);
			if (str[strlen(str) - 1] == NAMESEP)
				str[strlen(str) - 1] = 0;
		} else
			str = strsave(parts[i]);
		set_list_element(list, i + 1, str, NULL);
	}
	*plen = i;
	return TRUE;
}
/*====================================================
 * free_string_el -- free alloc'd string element of list
 *==================================================*/
static void
free_string_el(VPTR w)
{
	stdfree((STRING)w);
}
/*====================================================
 * free_name_list -- Free list created by name_to_list
 *==================================================*/
void
free_name_list (LIST list)
{
	/* name list is just string list */
	free_string_list(list);
}
/*====================================================
 * free_string_list -- free list of alloc'd strings
 *==================================================*/
void
free_string_list(LIST list)
{
	remove_list(list, free_string_el);
}
/*====================================================
 * traverse_names -- traverse names in db
 *  delegates to traverse_db_rec_rkeys
 *   passing callback function: traverse_name_callback
 *   and using local data in a TRAV_NAME_PARAM
 *   (newset is true every time it is a callback for a new name)
 *==================================================*/
typedef struct
{
	BOOLEAN(*func)(STRING key, STRING name, BOOLEAN newset, void *param);
	void * param;
} TRAV_NAME_PARAM;
/* see above */
static BOOLEAN
traverse_name_callback (RKEY rkey, STRING data, INT len, void *param)
{
	TRAV_NAME_PARAM *tparam = (TRAV_NAME_PARAM *)param;
	INT i;
	len=len; /* unused */

	parsenamerec(rkey, data);

	for (i=0; i<NRcount; i++)
	{
		if (!tparam->func(rkey2str(NRkeys[i]), NRnames[i], !i, tparam->param))
			return FALSE;
	}
	return TRUE;
}
/* see above */
void
traverse_names (BOOLEAN(*func)(STRING key, STRING name, BOOLEAN newset, void *param), void *param)
{
	TRAV_NAME_PARAM tparam;
	tparam.param = param;
	tparam.func = func;
	traverse_db_rec_rkeys(BTR, name_lo(), name_hi(), &traverse_name_callback, &tparam);
}
