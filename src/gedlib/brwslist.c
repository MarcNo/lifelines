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
/*==============================================================
 * brwslist.c -- Browse list operations
 * Copyright (c) 1993-94 by T.T. Wetmore IV; all rights reserved
 *   3.0.2 - 27 Nov 94
 *============================================================*/

#include "llstdlib.h"
#include "table.h"
#include "translat.h"
#include "gedcom.h"
#include "indiseq.h"
#include "vtable.h"

/*********************************************
 * global/exported variables
 *********************************************/

INDISEQ current_seq = NULL;

/*********************************************
 * local types
 *********************************************/

struct tag_blel {
	struct tag_vtable *vtable; /* generic object */
	INT refcnt; /* ref-countable object */
	STRING bl_name;
	INDISEQ bl_seq;
};
typedef struct tag_blel *BLEL;

/*********************************************
 * local function prototypes
 *********************************************/

static void blel_destructor(VTABLE *obj);
static BLEL create_new_blel(void);
/* unused
static void destroy_blel(BLEL blel);
*/

/*********************************************
 * local variables
 *********************************************/

static struct tag_vtable vtable_for_blel = {
	VTABLE_MAGIC
	, "blel"
	, &blel_destructor
	, &refcountable_isref
	, &refcountable_addref
	, &refcountable_release
	, 0 /* copy_fnc */
	, &generic_get_type_name
};
static LIST browse_lists=0;

/*********************************************
 * local & exported function definitions
 * body of module
 *********************************************/

/*=====================================================
 *  init_browse_lists -- Initialize named browse lists
 *===================================================*/
void
init_browse_lists (void)
{
	browse_lists = create_list();
}
/*=====================================================
 *  term_browse_lists -- Termiante named browse lists
 *===================================================*/
void
term_browse_lists (void)
{
	destroy_list(browse_lists);
}
/*===========================================
 *  create_new_blel -- Create browse list entry
 *=========================================*/
static BLEL
create_new_blel (void)
{
	BLEL blel = (BLEL) stdalloc(sizeof(*blel));
	memset(blel, 0, sizeof(*blel));
	blel->vtable = &vtable_for_blel;
	blel->refcnt = 1;
	return blel;
}
/*===========================================
 *  add_browse_list -- Add named browse list.
 *=========================================*/
void
add_browse_list (STRING name, INDISEQ seq)
{
	BLEL blel;
	BOOLEAN done = FALSE;
	if (!name) return;
	FORLIST(browse_lists, e)
		blel = (BLEL) e;
		if (blel->bl_name && eqstr(name, blel->bl_name)) {
			remove_indiseq(blel->bl_seq);
			blel->bl_seq = seq;
			done = TRUE;
			STOPLIST
			break;
		}
	ENDLIST
	if (done) return;
	FORLIST(browse_lists, e)
		blel = (BLEL) e;
		if (!blel->bl_name) {
			blel->bl_name = name;
			blel->bl_seq = seq;
			STOPLIST
			return;
		}
	ENDLIST
	blel = create_new_blel();
	blel->bl_name = name;
	blel->bl_seq = seq;
	enqueue_list(browse_lists, blel);
}
/*=================================================
 *  remove_browse_list -- Remove named browse list.
 *===============================================*/
void
remove_browse_list (STRING name,
                    INDISEQ seq)
{
	BLEL blel;
	remove_indiseq(seq);
	if (!name) return;
	FORLIST(browse_lists, e)
		blel = (BLEL) e;
		if (blel->bl_name && eqstr(name, blel->bl_name)) {
			blel->bl_name = NULL;
			blel->bl_seq = NULL;
		}
	ENDLIST
}
/*===========================================
 * find_named_seq -- Find named browse list.
 *=========================================*/
INDISEQ
find_named_seq (STRING name)
{
	BLEL blel;
	FORLIST(browse_lists, e)
		blel = (BLEL) e;
		if (eqstr(name, blel->bl_name)) {
			STOPLIST
			return copy_indiseq(blel->bl_seq);
		}
	ENDLIST
	return NULL;
}
/*===================================================
 * new_name_browse_list -- Rename named browse list.
 *=================================================*/
void
new_name_browse_list (STRING oldstr, STRING newstr)
{
	BLEL blel;
	FORLIST(browse_lists, e)
		blel = (BLEL) e;
		if (eqstr(oldstr, blel->bl_name)) {
			stdfree(blel->bl_name);
			blel->bl_name = newstr;
			STOPLIST
			return;
		}
	ENDLIST
}
/*===================================================
 *  update_browse_list -- Assign name to browse list.
 *=================================================*/
void
update_browse_list (STRING name,
                    INDISEQ seq)
{
	BLEL blel;
	if (!name) {	/* remove anonymous lists */
		remove_indiseq(seq);
		return;
	}
	FORLIST(browse_lists, e)
		blel = (BLEL) e;
		if (eqstr(name, blel->bl_name))
			blel->bl_seq = seq;
	ENDLIST
}
/*==============================================================
 * remove_from_browse_lists -- Remove stale elements from lists.
 *============================================================*/
void
remove_from_browse_lists (STRING key)
{
	BLEL blel;
	INDISEQ seq;
	if (current_seq) {
		seq = current_seq;
		while (delete_indiseq(seq, key, NULL, 0))
			;
	}
	FORLIST(browse_lists, e)
		blel = (BLEL) e;
		seq = blel->bl_seq;
		while (delete_indiseq(seq, key, NULL, 0))
			;
	ENDLIST
}
/*================================================================
 * rename_from_browse_lists -- Re-figures name of possible element
 *   in browse lists.
 *==============================================================*/
void
rename_from_browse_lists (STRING key)
{
	INDISEQ seq;
	BLEL blel;
	if (current_seq) {
		seq = current_seq;
		rename_indiseq(seq, key);
	}
	FORLIST(browse_lists, e)
		blel = (BLEL) e;
		seq = blel->bl_seq;
		rename_indiseq(seq, key);
	ENDLIST
}
/*=================================================
 * blel_destructor -- destructor for blel (browse list element)
 *  (destructor entry in vtable)
 *===============================================*/
static void
blel_destructor (VTABLE *obj)
{
	BLEL blel = (BLEL)obj;
	ASSERT(blel->vtable == &vtable_for_blel);
	stdfree(blel);
}
/*=================================================
 * destroy_blel -- destroy and free blel (browse list element)
 * All blels are destroyed in this function
 *===============================================*/
/* not used
static void
destroy_blel (BLEL blel)
{
	stdfree(blel->bl_name);
	remove_indiseq(blel->bl_seq);	
	stdfree(blel);
}
*/
