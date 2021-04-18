/* gedcomi.h */
/* functions internal to the gedcom subdirectory */

#ifndef _GEDCOM_PRIV_H
#define _GEDCOM_PRIV_H

#include "btree.h" /* for RKEY */

/* charmaps.c */
ZSTR custom_translate(CNSTRING str, TRANTABLE tt);
void custom_translatez(ZSTR zstr, TRANTABLE tt);
BOOLEAN init_map_from_file(CNSTRING file, CNSTRING mapname, TRANTABLE*, ZSTR zerr);

/* keytonod.c */
void cel_remove_record(CACHEEL cel, RECORD rec);
NODE is_cel_loaded(CACHEEL cel);

/* llgettext.c */
void init_win32_gettext_shim(void);
void llgettext_init(CNSTRING domain, CNSTRING codeset);
void update_textdomain_localedir(CNSTRING domain, CNSTRING prefix);
void llgettext_term(void);

/* names.c */
RECORD id_by_key(CNSTRING name, char ctype);
BOOLEAN rkey_eq(const RKEY * rkey1, const RKEY * rkey2);

/* node.c */
void check_node_leaks(void);
void set_record_key_info(RECORD rec, CNSTRING key);

/* record.c */
void check_record_leaks(void);
RECORD create_record_for_keyed_node(NODE node, CNSTRING key);
RECORD create_record_for_unkeyed_node(NODE node);
NODE is_record_temp(RECORD rec);
RECORD make_new_record_with_info(CNSTRING key, CACHEEL cel);
void record_remove_cel(RECORD rec, CACHEEL cel);
void record_set_cel(RECORD rec, CACHEEL cel);

/* xreffile.c */
BOOLEAN xrefs_get_counts_from_unopened_db(CNSTRING path, INT *nindis, INT *nfams
	, INT *nsours, INT *nevens, INT *nothrs, char ** errptr);

#endif /* _GEDCOM_PRIV_H */
