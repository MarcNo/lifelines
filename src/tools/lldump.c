/* 
   Copyright (c) 2019 Matthew Emmerton

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
/*=================================================================
 * lldump.c -- Command that dumps LifeLines databaes.
 *===============================================================*/

#include "llstdlib.h"
#include "btree.h"
#include "../btree/btreei.h"	/* path2fkey */
#include "gedcom.h"
#include "version.h"
#include "toolsi.h"

/*********************************************
 * required global variables
 *********************************************/
/* defined in liflines/main.c */
STRING readpath_file = NULL;    /* normally defined in liflines/main.c */
STRING readpath = NULL;         /* normally defined in liflines/main.c */
BOOLEAN readonly = FALSE;       /* normally defined in liflines/main.c */
BOOLEAN writeable = FALSE;      /* normally defined in liflines/main.c */
BOOLEAN immutable = FALSE;  /* normally defined in liflines/main.c */
int opt_finnish = 0;
int opt_mychar = 0;
extern BTREE BTR;

/*==================================
 * work -- what the user wants to do
 *================================*/
struct work {
        INT dump_btree;
        INT dump_key;
        INT dump_record;
        INT dump_xref;
};
static struct work todo;

/*********************************************
 * local function prototypes
 *********************************************/

/* alphabetical */
void crashlog (STRING fmt, ...);
void crashlogn (STRING fmt, ...);
void dump_block(STRING dir);
void dump_index(STRING dir);
void dump_keyfile(STRING dir);
void dump_xref(STRING dir);
BOOLEAN tf_print_block(BTREE btree, BLOCK block, void *param);
void print_block(BLOCK block, INT32 *offset);
BOOLEAN tf_print_index(BTREE btree, INDEX index, void *param);
void print_index(INDEX index, INT32 *offset);
void print_keyfile1(KEYFILE1 kfile1);
void print_keyfile2(KEYFILE2 kfile2);
static void print_usage(void);
void print_xrefs(void);
static void vcrashlog (int newline, const char * fmt, va_list args);

/*********************************************
 * local function definitions
 * body of module
 *********************************************/

/*=========================================
 * main -- Main procedure of lldump command
 *=======================================*/
int
main (int argc,
      char **argv)
{
	char *ptr, *flags, *dbname;
	BOOLEAN cflag=FALSE; /* create new db if not found */
	BOOLEAN writ=1; /* request write access to database */
	BOOLEAN immut=FALSE; /* immutable access to database */
	INT lldberrnum=0;
	int rtn=0;
	int i=0;

	set_signals(sighand_cmdline);

	/* TODO: needs locale & gettext initialization */

#ifdef WIN32
	/* TO DO - research if this is necessary */
	_fmode = O_BINARY;	/* default to binary rather than TEXT mode */
#endif

	/* handle conventional arguments --version and --help */
	/* needed for help2man to synthesize manual pages */
	for (i=1; i<argc; ++i) {
		if (!strcmp(argv[i], "--version")
			|| !strcmp(argv[i], "-v")) {
			print_version("lldump");
			return 0;
		}
		if (!strcmp(argv[i], "--help")
			|| !strcmp(argv[i], "-h")
			|| !strcmp(argv[i], "-?")) {
			print_usage();
			return 0;
		}
	}

       if (argc != 3 || argv[1][0] != '-' || argv[1][1] == '\0') {
                print_usage();
                goto finish;
        }
        flags = argv[1];
        dbname = argv[2];
        for (ptr=&flags[1]; *ptr; ++ptr) {
                switch(*ptr) {
                case 'a': todo.dump_btree=TRUE; todo.dump_key=TRUE; todo.dump_record=TRUE; todo.dump_xref=TRUE; break;
                case 'b': todo.dump_btree=TRUE; break;
                case 'k': todo.dump_key=TRUE; break;
                case 'r': todo.dump_record=TRUE; break;
                case 'x': todo.dump_xref=TRUE; break;
                default: print_usage(); goto finish;
                }
        }

	if (!(BTR = bt_openbtree(dbname, cflag, writ, immut, &lldberrnum))) {
		printf(_("Failed to open btree: %s."), dbname);
		puts("");
		return 20;
	}

	rtn = 0;

	if (todo.dump_btree)  { dump_index(dbname);   }
	if (todo.dump_key)    { dump_keyfile(dbname); }
	if (todo.dump_record) { dump_block(dbname);   }
	if (todo.dump_xref)   { dump_xref(dbname);    }

finish:
	closebtree(BTR);
	BTR = NULL;
	return rtn;
}
/*=============================
 * __fatal -- Fatal error routine
 *  handles null or empty details input
 *===========================*/
void
__fatal (STRING file, int line, CNSTRING details)
{
	printf("FATAL ERROR: ");
	if (details && details[0]) {
		printf("%s", details);
		printf("\nAT: ");
	}
	printf("%s: line %d\n", file, line);
	exit(1);
}
/*===============================================
 * print_usage -- display program help/usage
 *  displays to stdout
 *=============================================*/
void
print_usage (void)
{
#ifdef WIN32
	char * fname = _("\"\\My Documents\\LifeLines\\Databases\\MyFamily\"");
#else
	char * fname = _("/home/users/myname/lifelines/databases/myfamily");
#endif

	printf(_("lifelines `lldump' dumpss a lifelines database file.\n"));
	printf("\n\n");
	printf(_("Usage lldump [options] <database>"));
	printf("\n\n");
        printf(_("options:\n"));
        printf(_("\t-a = Dump ALL records\n"));
        printf(_("\t-b = Dump btree (INDEX)\n"));
        printf(_("\t-k = Dump key files (KEYFILE1, KEYFILE2)\n"));
        printf(_("\t-x = Dump xref file (DELETESET)\n"));
        printf(_("\t-r = Dump records (BLOCK)\n"));
	printf("\n");
	printf(_("\t--help\tdisplay this help and exit"));
	printf("\n");
	printf(_("\t--version\toutput version information and exit"));
	printf("\n\n");
	printf(_("Examples:"));
	printf("\n");
	printf(_("\tlldump %s"), fname);
	printf("\n\n");
	printf(_("Report bugs to https://github.com/MarcNo/lifelines/issues"));
	printf("\n");
}
/*===============================================
 * dump_index -- open and print index to stdout
 *=============================================*/
void dump_index(STRING dir)
{
	INDEX index;
	FKEY mkey = path2fkey("aa/aa");
        
	bbasedir(BTR) = dir;
	index = readindex(BTR, mkey, TRUE);

	traverse_index_blocks(BTR, index, NULL, tf_print_index, NULL);

	return;
}
/*===============================
 * tf_print_index -- traversal function wrapper for print_index
 *=============================*/
BOOLEAN tf_print_index(BTREE btree, INDEX index, void *param)
{
	INT32 offset = 0;

	btree = btree;	/* UNUSED */
	param = param;	/* UNUSED */

	print_index(index, &offset);
	return TRUE;
}
/*===============================
 * print_index -- print INDEX to stdout
 *=============================*/
void print_index(INDEX index, INT32 *offset)
{
	INT n;

	printf("INDEX - DIRECTORY\n");
	printf(FMT_INT32_HEX ": ix_self: " FMT_INT32_HEX " (%s)\n", *offset, index->ix_self, fkey2path(index->ix_self));
	*offset += sizeof(index->ix_self);

	printf(FMT_INT32_HEX ": ix_type: " FMT_INT32 " (%s)\n", *offset, index->ix_type,
               (index->ix_type == 1 ? "INDEX" : (index->ix_type == 2 ? "BLOCK" : "UNKNOWN")));
	*offset += sizeof(index->ix_type);

#if __WORDSIZE != 16
	printf(FMT_INT32_HEX ": ix_pad1: " FMT_INT16_HEX "\n", *offset, index->ix_pad1);
	*offset += sizeof(index->ix_pad1);
#endif

	printf(FMT_INT32_HEX ": ix_parent: " FMT_INT32_HEX " (%s)\n", *offset, index->ix_parent, fkey2path(index->ix_parent));
	*offset += sizeof(index->ix_parent);

	printf(FMT_INT32_HEX ": ix_nkeys: " FMT_INT16 "\n", *offset, index->ix_nkeys);
	*offset += sizeof(index->ix_nkeys);

	for (n=0; n<NOENTS; n++) {
		printf(FMT_INT32_HEX ": ix_rkey[" FMT_INT_04 "]: '%-8.8s'\n", *offset, n, (char *)&index->ix_rkeys[n]);
		*offset += sizeof(index->ix_rkeys[n]);
	}

#if __WORDSIZE != 16
	printf(FMT_INT32_HEX ": ix_pad2: " FMT_INT16_HEX "\n", *offset, index->ix_pad2);
	*offset += sizeof(index->ix_pad2);
#endif

	for (n=0; n<NOENTS; n++) {
		printf(FMT_INT32_HEX ": ix_fkey[" FMT_INT_04 "]: " FMT_INT32_HEX " (%s)\n", *offset, n, index->ix_fkeys[n], fkey2path(index->ix_fkeys[n]));
		*offset += sizeof(index->ix_fkeys[n]);
	}

	printf(FMT_INT32_HEX ": EOF (0x%04x)\n", *offset, BUFLEN);
	printf("\n");
}
/*===============================================
 * dump_block -- open and print block to stdout
 *=============================================*/
void dump_block(STRING dir)
{
	INDEX index;
	FKEY mkey = path2fkey("aa/aa");
        
	bbasedir(BTR) = dir;
	index = readindex(BTR, mkey, TRUE);

	traverse_index_blocks(BTR, index, NULL, NULL, tf_print_block);

	return;
}
/*===============================
 * tf_print_block -- traversal function wrapper for print_block
 *=============================*/
BOOLEAN tf_print_block(BTREE btree, BLOCK block, void *param)
{
	INT32 offset = 0;

	btree = btree;	/* UNUSED */
	param = param;	/* UNUSED */

	print_block(block, &offset);
	return TRUE;
}
/*===============================
 * print_block -- print BLOCK to stdout
 *=============================*/
void print_block(BLOCK block, INT32 *offset)
{
	INT n;

	printf("BLOCK - DIRECTORY\n");
	printf(FMT_INT32_HEX ": ix_self: " FMT_INT32_HEX " (%s)\n", *offset, block->ix_self, fkey2path(block->ix_self));
	*offset += sizeof(block->ix_self);

	printf(FMT_INT32_HEX ": ix_type: " FMT_INT32 " (%s)\n", *offset, block->ix_type,
               (block->ix_type == 1 ? "INDEX" : (block->ix_type == 2 ? "BLOCK" : "UNKNOWN")));
	*offset += sizeof(block->ix_type);

#if __WORDSIZE != 16
	printf(FMT_INT32_HEX ": ix_pad1: " FMT_INT16_HEX "\n", *offset, block->ix_pad1);
	*offset += sizeof(block->ix_pad1);
#endif

	printf(FMT_INT32_HEX ": ix_parent: " FMT_INT32_HEX " (%s)\n", *offset, block->ix_parent, fkey2path(block->ix_parent));
	*offset += sizeof(block->ix_parent);

	printf(FMT_INT32_HEX ": ix_nkeys: %d\n", *offset, block->ix_nkeys);
	*offset += sizeof(block->ix_nkeys);

	for (n=0; n<NORECS; n++) {
		printf(FMT_INT32_HEX ": ix_rkey[" FMT_INT_04 "]: '%-8.8s'\n", *offset, n, (char *)&block->ix_rkeys[n]);
		*offset += sizeof(block->ix_rkeys[n]);
	}

#if __WORDSIZE != 16
	printf(FMT_INT32_HEX ": ix_pad2: " FMT_INT16_HEX "\n", *offset, block->ix_pad2);
	*offset += sizeof(block->ix_pad2);
#endif

	for (n=0; n<NORECS; n++) {
		printf(FMT_INT32_HEX ": ix_offs[" FMT_INT_04 "]: " FMT_INT32_HEX "\n", *offset, n, block->ix_offs[n]);
		*offset += sizeof(block->ix_offs[n]);

		printf(FMT_INT32_HEX ": ix_lens[" FMT_INT_04 "]: " FMT_INT32_HEX "\n", *offset, n, block->ix_lens[n]);
		*offset += sizeof(block->ix_lens[n]);
	}

	printf(FMT_INT32_HEX ": EOF (0x%04x)\n", *offset, BUFLEN);
	printf("\n");

#if 0
	printf("BLOCK - DATA\n");
	for (n=0; n<NORECS; n++) {
		/* TODO: Read the actual data! */
		printf(FMT_INT32_HEX ": rkey[" FMT_INT_04 "]: '%-8.8s' off: " FMT_INT32_HEX " len: " FMT_INT32_HEX " '%s'\n", *offset, n, (char *)&block->ix_rkeys[n], block->ix_offs[n], block->ix_lens[n], "");
		*offset += block->ix_lens[n];
	}
	printf("\n");
#endif
}
/*===============================
 * dump_keyfile -- open and print KEYFILE1 and KEYFILE2 to stdout
 *=============================*/
void dump_keyfile(STRING dir)
{
	char scratch[200];
	struct stat sbuf;
	KEYFILE1 kfile1;
	KEYFILE2 kfile2;
	FILE *fk;
	size_t size;

	sprintf(scratch, "%s/key", dir);
        if (stat(scratch, &sbuf) || !S_ISREG(sbuf.st_mode)) {
		printf("Error opening keyfile\n");
		goto error2;
	}

        if (!(fk = fopen(scratch, LLREADBINARY))) {
		printf("Error opening keyfile\n");
		goto error2;
	}

	if (fseek(fk, 0, SEEK_END)) {
		printf("Error seeking to end of keyfile\n");
		goto error1;
	}

        size = ftell(fk);

	if (fseek(fk, 0, SEEK_SET)) {
		printf("Error seeking to start of keyfile\n");
		goto error1;
	}

	if (size != sizeof(kfile1) &&
            size != (sizeof(kfile1) + sizeof(kfile2))) {
		printf("Error: keyfile size invalid (" FMT_SIZET "), valid sizes are " FMT_SIZET " and " FMT_SIZET "\n",
		       size, sizeof(kfile1), sizeof(kfile1)+sizeof(kfile2));
		goto error1;
	}

        if (fread(&kfile1, sizeof(kfile1), 1, fk) != 1) {
		printf("Error reading keyfile\n");
		goto error1;
        }

	print_keyfile1(kfile1);

        if (fread(&kfile2, sizeof(kfile2), 1, fk) != 1) {
		printf("Error reading keyfile2\n");
		goto error1;
        }

	print_keyfile2(kfile2);

error1:
	fclose(fk);
error2:
	return;
}
/*===============================
 * print_keyfile1 -- print KEYFILE1 to stdout
 *=============================*/
void print_keyfile1(KEYFILE1 kfile1)
{
	INT16 offset = 0;

	printf("KEYFILE1\n");
	printf("========\n");

	printf(FMT_INT16_HEX ": mkey:  " FMT_INT32_HEX " (%s)\n", offset, kfile1.k_mkey, fkey2path(kfile1.k_mkey));
	offset += sizeof(kfile1.k_mkey);

	printf(FMT_INT16_HEX ": fkey:  " FMT_INT32_HEX " (%s)\n", offset, kfile1.k_fkey, fkey2path(kfile1.k_fkey));
	offset += sizeof(kfile1.k_fkey);

	printf(FMT_INT16_HEX ": ostat: " FMT_INT32_HEX " (%d)\n", offset, kfile1.k_ostat, kfile1.k_ostat);
	offset += sizeof(kfile1.k_ostat);

	printf(FMT_INT16_HEX ": EOF (" FMT_INT16_HEX ")\n", offset, (INT16)sizeof(kfile1));
	printf("\n");
}
/*===============================
 * print_keyfile2 -- print KEYFILE2 to stdout
 *=============================*/
void print_keyfile2(KEYFILE2 kfile2)
{
	INT16 offset = 0;

	printf("KEYFILE2\n");
	printf("========\n");

	printf(FMT_INT16_HEX ": name:    '%-18.18s'\n", offset, kfile2.name);
	offset += sizeof(kfile2.name);
#if WORDSIZE != 16
	printf(FMT_INT16_HEX ": pad1:    " FMT_INT16_HEX "\n", offset, kfile2.pad1);
	offset += sizeof(kfile2.pad1);
#endif
	printf(FMT_INT16_HEX ": magic:   " FMT_INT32_HEX "\n", offset, kfile2.magic);
	offset += sizeof(kfile2.magic);

	printf(FMT_INT16_HEX ": version: " FMT_INT32_HEX " (%d)\n", offset, kfile2.version, kfile2.version);
	offset += sizeof(kfile2.version);

	printf(FMT_INT16_HEX ": EOF (" FMT_INT16_HEX ")\n", offset, (INT16)sizeof(kfile2));
	printf("\n");
}
/*===============================
 * dump_xref -- open and print xrefs to stdout
 *=============================*/
void dump_xref(STRING dir)
{
	dir = dir;	/* UNUSED */

	if (!openxref(FALSE))
	{
		printf("Error opening/reading xrefs\n");
		goto error1;
	}

	print_xrefs();

error1:
	closexref();
	return;
}
/*===============================
 * print_xrefs -- print xrefs to stdout
 *=============================*/
void print_xrefs(void)
{
	printf("XREFFILE\n");
	printf("========\n");
	dumpxrefs();
}
/*===============================
 * vcrashlog -- Send crash info to screen
 *  internal implementation
 *=============================*/
static void
vcrashlog (int newline, const char * fmt, va_list args)
{
        vprintf(fmt, args);
        if (newline) {
                printf("\n");
        }
}
/*===============================
 * crashlog -- Send string to crash log and screen
 *=============================*/
void
crashlog (STRING fmt, ...)
{
        va_list args;
        va_start(args, fmt);
        vcrashlog(0, fmt, args);
        va_end(args);
}
/*===============================
 * crashlogn -- Send string to crash log and screen
 *  add carriage return to end line
 *=============================*/
void
crashlogn (STRING fmt, ...)
{
        va_list args;
        va_start(args, fmt);
        vcrashlog(1, fmt, args);
        va_end(args);
}

