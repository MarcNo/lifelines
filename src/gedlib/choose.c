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
/*============================================================
 * choose.c -- Implements the choose operations
 * Copyright(c) 1992-4 by T.T. Wetmore IV; all rights reserved
 *   3.0.2 - 06 Dec 94    3.0.3 - 08 May 95
 *==========================================================*/

#include "llstdlib.h"
#include "table.h"
#include "translat.h"
#include "gedcom.h"
#include "indiseq.h"
#include "liflines.h"
#include "screen.h"

/*=================================================
 * choose_child -- Choose child of person or family
 *===============================================*/
NODE
choose_child (NODE indi,
              NODE fam,
              STRING msg0,
              STRING msgn,
              BOOLEAN ask1)
{
	INDISEQ seq = NULL;
	NODE node;
	if (indi) seq = indi_to_children(indi);
	if (!indi && fam) seq = fam_to_children(fam);
	if (!seq) {
		message(msg0);
		return NULL;
	}
	node = choose_from_indiseq(seq, ask1, msgn, msgn);
	remove_indiseq(seq, FALSE);
	return node;
}
/*========================================
 * choose_spouse -- Choose person's spouse
 *======================================*/
NODE
choose_spouse (NODE indi,
               STRING msg0,
               STRING msgn)
{
	INDISEQ seq;
	NODE node;
	if (!indi) return NULL;
	if (!(seq = indi_to_spouses(indi))) {
		message(msg0);
		return NULL;
	}
	node = choose_from_indiseq(seq, FALSE, NULL, msgn);
	remove_indiseq(seq, FALSE);
	return node;
}
/*========================================
 * choose_source -- Choose any referenced source from some,
 *  presumably top level, node
 *======================================*/
NODE
choose_source (NODE what, STRING msg0, STRING msgn)
{
	INDISEQ seq;
	NODE node;
	if (!what) return NULL;
	if (!(seq = node_to_sources(what))) {
		message(msg0);
		return NULL;
	}
	node = choose_from_indiseq(seq, TRUE, msgn, msgn);
	remove_indiseq(seq, FALSE);
	return node;
}
/*==========================================================
 * choose_family -- Choose family from person's FAMS/C lines
 *========================================================*/
NODE
choose_family (NODE indi,
               STRING msg0,
               STRING msgn,
               BOOLEAN fams)
{
	NODE node;
	INDISEQ seq = indi_to_families(indi, fams);
	if (!seq) {
		if (msg0)
			message(msg0);
		return NULL;
	}
	node = choose_from_indiseq(seq, FALSE, NULL, msgn);
	remove_indiseq(seq, FALSE);
	return node;
}
/*===================================================
 * choose_father -- Choose father of person or family
 *=================================================*/
NODE
choose_father (NODE indi,
               NODE fam,
               STRING msg0,
               STRING msgn,
               BOOLEAN ask1)
{
	INDISEQ seq = NULL;
	NODE node;
	if (indi) seq = indi_to_fathers(indi);
	if (!indi && fam) seq = fam_to_fathers(fam);
	if (!seq) {
		message(msg0);
		return NULL;
	}
	node = choose_from_indiseq(seq, ask1, msgn, msgn);
	remove_indiseq(seq, FALSE);
	return node;
}
/*===================================================
 * choose_mother -- Choose mother of person or family
 *=================================================*/
NODE
choose_mother (NODE indi,
               NODE fam,
               STRING msg0,
               STRING msgn,
               BOOLEAN ask1)
{
	INDISEQ seq = NULL;
	NODE node;
	if (indi) seq = indi_to_mothers(indi);
	if (!indi && fam) seq = fam_to_mothers(fam);
	if (!seq) {
		message(msg0);
		return NULL;
	}
	node = choose_from_indiseq(seq, ask1, msgn, msgn);
	remove_indiseq(seq, FALSE);
	return node;
}
