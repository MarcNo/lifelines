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
/*==========================================================
 * gedcheck.h -- Header for GEDCOM validation
 * Copyright(c) 1994 by T.T. Wetmore IV; all rights reserved
 *   3.0.0 - 28 May 94
 *========================================================*/

#ifndef _GEDCHECK_H
#define _GEDCHECK_H

#define INDI_REC 0
#define FAM_REC  1
#define SOUR_REC 2
#define EVEN_REC 3
#define OTHR_REC 4
#define UNKN_REC 5
#define IGNR_REC 6

#define IS_MALE   1
#define IS_FEMALE 2
#define BE_MALE   4
#define BE_FEMALE 8

#define KNOWNTYPE(t) ((Type(t)>=INDI_REC) && (Type(t)<=OTHR_REC))

typedef char CHAR;

typedef struct {
	CHAR type;
	CHAR sex;
	CHAR male;
	CHAR fmle;
	INT dex;
	INT line;
	STRING key;
	STRING new;
} *ELMNT;

#define Type(e) ((e)->type)
#define Key(e)  ((e)->key)
#define New(e)  ((e)->new)
#define Dex(e)  ((e)->dex)
#define Line(e) ((e)->line)
#define Sex(e)  ((e)->sex)
#define Male(e) ((e)->male)
#define Fmle(e) ((e)->fmle)

extern ELMNT *index_data;

INT xref_to_index(STRING);

#endif /* _GEDCHECK_H */
