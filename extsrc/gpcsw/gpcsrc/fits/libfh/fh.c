/*                                             -*- c-file-style: "Ellemtel" -*-

`fh.c' - FITS Handling Routines (see fh.h for descriptions)

This file is part of version 1 of the FITS Handling Library.
Read the `License' file for terms of use and distribution.
Copyright 2001, Canada-France-Hawaii Telescope, daprog@cfht.hawaii.edu.

___This header automatically generated from `Index'. Do not edit it here!___ */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>	/* for read() and write() */
#include <fcntl.h>	/* For file locking */
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <float.h>
#include <errno.h>

#include "fh.h"

static const char rcs_id[] = "@(#) $Id: fh.c,v 1.8 2003/05/28 06:47:23 isani Exp isani $";

/*
 * -----------------------------
 * Internal values used by libfh
 * -----------------------------
 */
#define FH_BUFFER_SIZE (128*1024) /* Buffer for copying data between FITS files */
#define IDX_AUTO_INCR 0.001	/* Auto-increment idx numbers if 0.0 used */

#define FH_RESERVE \
"COMMENT  Reserved space.  This line can be used to add a new FITS card.         "
#define FH_RESERVE_LEN 80

#define FH_XTENSION_UNKNOWN 0
#define FH_XTENSION_IMAGE   1
#define FH_XTENSION_TABLE   2

#define MAX_SIG_FIGS 62 /* An 80-column FITS card cannot hold more than
			 * this many significant figures, if a double
			 * precision float value is printed with an E+
			 * exponent that could have up to 3 digits too,
			 * and a decimal place, and a leading - if the
			 * number is negative.
			 */

/*
 * ---------------------------------
 * Internal structures used by libfh
 * ---------------------------------
 */
typedef struct
{
   double idx;		/* Sorting number */
   char card[FH_CARD_SIZE+1]; /* card[80] is always '\0' so library can use strspn() */
} FitsCard;

typedef FitsCard* FitsCardPtr;
/*
 * The `FitsCard' structure contains the 80 bytes, exactly as they
 * will appear in the FITS file, along with an "idx" floating point
 * sorting number (which is removed when writing a file.)
 */

/*
 * NOTE: When adding members to this structure, please check
 * the block of code in fh_file() that checks "if (extname)"
 * because it needs to decide whether to keep each element of
 * the structure from the PHU or the EHU.
 */
#define HU_MAGIC 0x4855 /* "HU" */
typedef struct
{
   unsigned int magic;	/* Just there to help routines make sure they got a HeaderUnit */
   FitsCardPtr* hdr;	/* Table of FITS cards */
   int len;		/* # of cards (excl. END and 1 blank line which will be added) */
   int pos;		/* Loop counter for fh_first()/fh_next() */
   int size;		/* # of slots allocated so far (for realloc) */
   int bitpix;		/* Cached copy of BITPIX keyword */
   int image_bytes;	/* Cached copy of calculated size of image */
   int image_bytes_left;/* Used for fh_read_image and fh_write_image */
   int extensions;	/* Cached copy of NEXTEND keyword (if EXTEND=T) */
   int xtension;	/* Cached copy of XTENSION (0=none 1=IMAGE 2=TABLE) */
   double idx_highest;	/* Highest idx number used (for auto-incrementing) */
   int set_all_units;	/* Should fh_set* functions apply to extensions automatically? */
   HeaderUnit ehu;	/* Headers for first contained extension (if any) */
   HeaderUnit next;	/* Headers for next extension (if _this_ is an extension) */
   HeaderUnit phu;	/* Headers from the primary (if _this_ is an extension) */
   struct flock fl;	/* File lock, if any. */
   int fd;		/* -1 if no file associated with this header unit */
   int fd_locked;	/* -1 if no file has been locked */
   int fd_to_close;	/* Make sure fh_destroy closes fds opened by fh_file */
   int fd_header_blocks; /* # of header blocks in the original file (for fh_rewrite) */
   int file_size;	/* Total size of file for mmap use only */
   int mmap_count;      /* Number of extensions currently mapped */
   void* mmap_addr;     /* -1 if file has not been memory-mapped */
   int reserve;		/* Number of blank cards to reserve (for fh_write) */
   int reserve_found;	/* Number of reserve cards found in existing header */
   off_t off_hdrs;	/* Offset in file to this set of cards */
   off_t off_data;	/* Offset in file to data (or extensions) after the cards */
   int err_memory;	/* Used by fh_set*() to count memory errors */
   int err_invalid;	/* Used by fh_set*() to count invalid argument errors */
   int counting_cards;	/* No, we're not going to Vegas.  When this flag is set, the
			 * library is in a special mode where fh_[re]write() does nothing
			 * to a file, but instead prints the number of cards your program
			 * is going to need to stdout.  This value should be captured and
			 * summed together with other programs to calculate the value
			 * to be passed to fh_reserve() by upstream programs which
			 * actually create the FITS file.
			 */
} HeaderUnitStruct;

/*
 * The following cast is needed because the outside world doesn't
 * see the internals of a HeaderUnitStruct.  Instead, they pass a
 * (void*) pointer (HeaderUnit) which must be cast back to the real type.
 */
#define FH_HU(hu) (((hu)&&((HeaderUnitStruct*)(hu))->magic==HU_MAGIC)?\
		(HeaderUnitStruct*)(hu):0)

/*
 * This table is for converting characters into legal
 * characters for fits card NAMES.  It is equivalent to:
 * 
 * if (isalpha(c) || isdigit(c) || *name=='-' || *name=='_')
 *   return = toupper(c);
 * else
 *   return '_';
 */
static char name_chr[256] =
" _______________"
"________________"
" ____________-__"
"0123456789______"
"_ABCDEFGHIJKLMNO"
"PQRSTUVWXYZ_____"
"_ABCDEFGHIJKLMNO"
"PQRSTUVWXYZ_____"
"________________"
"________________"
"________________"
"________________"
"________________"
"________________"
"________________"
"________________";

#define NAME_CHR(c) (name_chr[(unsigned int)((unsigned char)(c))])

/*
 * The following is the return type of the Keyword name matching function.
 * It is used for several thins internally in this library.  The highest
 * valid return code is the one returned.
 */
typedef enum
{
   MATCH_FAILED = -1, /* Names do not match */
   MATCH_SUBSTR = 0,  /* Name1 begins with all the characters in name2 */
   MATCH_KEYWORD = 1, /* Names are the same for first 8 characters */
   MATCH_FULL = 2     /* Names are the same (up to 80 chars compared) */
} match_result;

/*
 * This library requires a SYSV style sprintf, which returns the number
 * of bytes added to the string, just like printf and fprintf.
 */
#ifdef SUNOS
#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif
static int sysv_sprintf(char* s, const char* fmt, ...)
{
   va_list args;
#ifdef __STDC__
   va_start(args, fmt);
#else
   va_start(args);
#endif
   vsprintf(s, fmt, args);
   return strlen(s);
}
#else
#define sysv_sprintf sprintf
#endif

/*
 * ----------------------------------------------
 * Internal functions for implementation of libfh
 * ----------------------------------------------
 * (These are found at the end of this file.)
 */

/* Functions that do file operations WITH RETRY: */
static int write_file(int fd, const void* buf, int len);
static int read_file(int fd, void* buf, int len);
static int seek_file(int fd, off_t off); /* Performs lseek(SEEK_SET) */
static int fh_lock_file(HeaderUnit hu, int fd, fh_mode mode);
static int fh_unlock_file(HeaderUnit hu);

static const char* padding_block(int type); /* Buffer of \0's or ' ' */
static fh_result fix_characters(char* s, int repair); /* Fix bad chars in card */
static match_result fh_cmp(const char* name1, const char* name2); /* Compares keywords */
static int fh_compare(const void* a, const void* b); /* Compares `idx' */
static void fh_sort(HeaderUnit hu); /* Uses fh_compare to sort HeaderUnit */

/* Internal functions used to get and build cards */
static FitsCardPtr get_hdr(HeaderUnitStruct* list, const char* name, double idx);
static const char* get_value(HeaderUnitStruct* list, const char* name);
static double auto_idx(const char* name);
static char* get_card(HeaderUnitStruct* list, double idx, const char* name,
		      double idxmatch);
static void add_comment(char* s, int col, const char* comment);
static void show_card_count(HeaderUnitStruct* hu) { printf("%d\n", FH_HU(hu)->len); }

/*
 * ----------------------
 * Main library functions
 * ----------------------
 */
#ifdef HAVE_FH_VALIDATE
#include "fh_validate.c"
#endif

/*
 * This is just so rcs_id doesn't turn up "unused".
 * And who knows, maybe someone wants their program to print
 * this information in debug mode.
 */
const char* fh_rcs_version(void); /* proto to make gcc happy */
const char* fh_rcs_version(void) { return rcs_id; }

/*
 * This gets called any time a new header is set to force things like
 * image_bytes and extensions to be re-calculated in case a relevant
 * keyword got changed.  It's also used in fh_create().
 */
static void
invalidate_hu_cache(HeaderUnitStruct* list)
{
   list->bitpix = list->image_bytes = list->extensions = list->xtension = -1;
}

/*
 * ---------------------------
 * Error/warning logging stuff
 * ---------------------------
 */

/*
 * Error logger routines take a single string constant parameter.
 * There is one routine for errors and another for warnings.
 * The library does not generate any other kinds of messages.
 */

static char err_buf[1024]; /* Buffer for building error strings */

static void
dfl_log_error(const char* s) { fprintf(stderr, "error: libfh: %s\n", s); }

static void
dfl_log_perror(const char* s) { perror(s); }

static void
dfl_log_warning(const char* s) { fprintf(stderr, "warning: libfh: %s\n", s); }
/*
 * Default message handlers print to stderr with the prefix "error:" or "warning:"
 */

static void
null_log(const char* s) { if (s) return; }
/*
 * It's also possible to pass a NULL to fh_log_xxx() and rely only on the
 * error returns from the functions themselves.
 */

static fh_logger_t log_error = dfl_log_error;
static fh_logger_t log_perror = dfl_log_perror;
static fh_logger_t log_warning = dfl_log_warning;

/*
 * Functions to install your own warning/error handlers:
 */
void
fh_log_error(fh_logger_t newlogger)
{
   if (newlogger == 0) log_error = null_log;
   else log_error = newlogger;
}

void
fh_log_perror(fh_logger_t newlogger)
{
   if (newlogger == 0) log_perror = null_log;
   else log_perror = newlogger;
}

void
fh_log_warning(fh_logger_t newlogger)
{
   if (newlogger == 0) log_warning = null_log;
   else log_warning = newlogger;
}

HeaderUnit
fh_create(void)
{
   HeaderUnitStruct* rtn; 

   if (!(rtn = (HeaderUnitStruct*)malloc(sizeof(HeaderUnitStruct))) ||
       !(memset(rtn, 0, sizeof(HeaderUnitStruct))) ||
       !(rtn->hdr = (FitsCardPtr*)malloc(sizeof(FitsCardPtr) * 200)))
   {
      if (rtn) free(rtn);
      log_error("out of memory in fh_create()");
      return 0;
   }
   rtn->len = 0;
   rtn->size = 200;
   rtn->idx_highest = 1000.000; /* User keywords start a 1000.001 */
   rtn->fd = -1;
   rtn->fd_locked = -1; /* Set if file descriptor need to be unlocked */
   rtn->fd_to_close = -1; /* Set if fh_destroy() will close fd from fh_file */
   rtn->fd_header_blocks = 0; /* Not valid unless fd >= 0 */
   rtn->file_size = 0; /* Total size of file from stat() */
   rtn->mmap_count = 0; /* Not memory mapped initially */
   rtn->mmap_addr = (void*)-1;
   rtn->reserve = 0;
   rtn->reserve_found = 0;
   rtn->ehu = 0;
   rtn->next = 0;
   rtn->off_hdrs = 0;
   rtn->off_data = 0;
   rtn->fl.l_type = F_UNLCK;
   rtn->err_memory = 0;
   rtn->err_invalid = 0;
   rtn->counting_cards = 0;
   invalidate_hu_cache(rtn);
   rtn->magic = HU_MAGIC; /* Just used as a sanity check */
   return (HeaderUnit)rtn;
}

/*
 * Future fh_set_*() calls apply to any/all EHU's that were
 * found in the same file.
 */
fh_result
fh_set_all_units(HeaderUnit hu)
{
   HeaderUnitStruct* list = FH_HU(hu);
   off_t off;
   int ext;

   if (!list)
   {
      log_error("Invalid HeaderUnit passed to fh_set_all_units()");
      return FH_INVALID;
   }

   if (list->fd == -1)
   {
      log_error("fh_set_all_units only applies to HeaderUnits "
		"read from a file or file descriptor");
      return FH_INVALID;
   }

   /*
    * Save current file position, make sure all extensions are
    * cached, and then restore the current file position.  If this
    * is skipped, then the other fh_set() functions would have the
    * unexpected side-effect of moving the file position around
    * they happen to cause the first access to an EHU.
    */
   off = lseek(list->fd, 0, SEEK_CUR);
   for (ext = 1; ext <= fh_extensions(list); ext++)
   {
      if (fh_ehu(hu, ext) == 0)
	 return FH_NOT_FOUND;
   }
   seek_file(list->fd, off);

   list->set_all_units = 1;
   return FH_SUCCESS;
}

fh_result
fh_destroy(HeaderUnit hu)
{
   fh_result rtn = FH_SUCCESS;
   HeaderUnitStruct* list = FH_HU(hu);

   if (!list)
   {
      log_error("Invalid HeaderUnit passed to fh_destroy()");
      return FH_INVALID;
   }

   if (fh_unlock_file(hu) == -1)
      rtn = FH_IN_ERRNO;

   if (list->fd_to_close != -1 &&
       close(list->fd_to_close) != 0)
   {
      log_perror("close for fh_file() failed");
      rtn = FH_IN_ERRNO;
   }

   while (list->len)
      free(list->hdr[--list->len]);
   free(list->hdr);

   if (list->ehu) fh_destroy(list->ehu);
   if (list->next) fh_destroy(list->next);

   free(list);
   return rtn;
}

fh_result
fh_count_cards(HeaderUnit hu)
{
   HeaderUnitStruct* list = FH_HU(hu);

   if (!list)
   {
      log_error("Invalid HeaderUnit passed to fh_count_cards()");
      return FH_INVALID;
   }
   list->counting_cards++;
   return FH_SUCCESS;
}

fh_result
fh_file(HeaderUnit hu, const char* filespec, fh_mode mode)
{
   HeaderUnitStruct* list = FH_HU(hu);
   char* extname = 0;
   char* filename = 0;
   struct stat st_buf;
   int open_mode = 0, fd;
   fh_result rtn = FH_SUCCESS;

   if (!list)
   {
      log_error("Invalid HeaderUnit passed to fh_file()");
      return FH_INVALID;
   }

   if (list->len)
   {
      log_error("fh_file() requires an empty HeaderUnit");
      return FH_BAD_VALUE;
   }

   if (filespec && !strcmp(filespec, FH_COUNT_CARDS))
   {
      fh_count_cards(hu);
      return FH_SUCCESS;
   }

   switch (mode)
   {
      case FH_FILE_RDONLY:
      case FH_FILE_RDONLY_NOLOCK: open_mode = O_RDONLY; break;
      case FH_FILE_RDWR:
      case FH_FILE_RDWR_NOLOCK: open_mode = O_RDWR; break;
      default:
	 log_error("Invalid `mode' argument passed to fh_file()");
	 return FH_INVALID;
   }

   /*
    * If no filename, or if the filespec is "-", try to read from stdin.
    */
   if (!filespec || !strcmp(filespec, "-"))
   {
      if (isatty(STDIN_FILENO))
      {
	 log_error("cannot read from a terminal");
	 return FH_IS_TTY;
      }
      if ((rtn = fh_read(hu, STDIN_FILENO, FH_AUTO)) != FH_SUCCESS)
      {
	 if (rtn != FH_END_OF_FILE)
	    log_error("failed to read FITS header from stdin");
	 return rtn;
      }
      return FH_SUCCESS; /* Successfully read header from stdin. */
   }

   /*
    * Next test is to see if filespec, exactly as given, exists
    * as a file (even including '[' ']' or other characters.)
    */
   if (stat(filespec, &st_buf) == 0)
   {
      if (S_ISDIR(st_buf.st_mode))
      {
	 sprintf(err_buf, "`%.900s' is a directory", filespec);
	 log_error(err_buf);
	 return FH_IS_DIR;
      }
      filename = strdup(filespec);
   }
   else
   {
      char* bracket_open = 0;
      char* bracket_close = 0;

      if (!(filename = (char*)malloc(strlen(filespec)*2 + 32)))
      {
	 log_error("out of memory in fh_file()");
	 return FH_NO_MEMORY;
      }
      strcpy(filename, filespec);

      /*
       * Extract "[extname]" if present in filespec.
       */
      if ((bracket_open = strchr(filename, '[')) != 0 &&
	  (bracket_close = strchr(bracket_open, ']')) != 0)
      {
	 *bracket_open++ = *bracket_close++ = '\0';
	 extname = strdup(bracket_open);
	 strcat(filename, bracket_close);
      }

      /*
       * Make sure the filename contains ".fits" for the first test.
       */
      if (strlen(filename) <= 5 ||
	  strcmp(filename + strlen(filename) - 5, ".fits"))
	 strcat(filename, ".fits");

      /*
       * Now strip off the ".fits" and try that next.
       */
      if (stat(filename, &st_buf) != 0)
      {
	 filename[strlen(filename) - 5] = '\0';
	 if (stat(filename, &st_buf) != 0)
	 {
	    log_perror(filespec);
	    return FH_IN_ERRNO;
	 }
      }

      if (S_ISDIR(st_buf.st_mode))
      {
	 const char* basename;

	 if (!extname)
	 {
	    sprintf(err_buf, "`%.900s' is a directory", filespec);
	    log_error(err_buf);
	    return FH_IS_DIR;
	 }

	 /*
	  * Get the part of filename without the path elements.
	  */
	 basename = strrchr(filename, '/');
	 if (basename) basename++;
	 else basename = filename;
	 strcpy(filename + strlen(filename) + 1, basename);
	 filename[strlen(filename)] = '/';

	 if (!strncmp(extname, "chip", 4)) strcat(filename, extname + 4);
	 else if (!strncmp(extname, "amp", 3)) strcat(filename, extname + 3);
	 else if (!strncmp(extname, "im", 2)) strcat(filename, extname + 2);
	 else strcat(filename, extname);
	 strcat(filename, ".fits");
	 free(extname); extname = 0;
      }
   }

   /*
    * Now try to open filename.
    */
   if ((fd = open(filename, open_mode, 0)) == -1)
   {
      log_perror(filename);
      free(filename);
      if (extname) free(extname);
      return FH_IN_ERRNO;
   }
   free(filename);
   list->fd_to_close = fd;
   list->file_size = st_buf.st_size;

   if (mode == FH_FILE_RDONLY || mode == FH_FILE_RDWR)
   {
      if (fh_lock_file(hu, fd, mode) == -1)
	 log_perror("failed to lock file");
   }

   /*
    * Read the (first) header unit from the file.
    */
   if ((rtn = fh_read(hu, fd, FH_AUTO)) != FH_SUCCESS)
   {
      if (extname) free(extname);
      return rtn;
   }

   if (mode == FH_FILE_RDONLY)
   {
      if (fh_extensions(hu))
      {
	 /* === Read in the extension headers now, or just leave the file locked?
	  * === For now, I'm opting to leave the whole file locked.
	  */
      }
      else
      {
	 if (fh_unlock_file(hu) == -1)
	    log_perror("failed to unlock file");
      }
   }

   /*
    * If an extension was given, try to locate that extension within
    * the MEF file.  What happens here is a little gross.
    */
   if (extname)
   {
      HeaderUnit ehu;
      fh_bool inherit;

      if ((ehu = fh_ehu_by_extname(hu, extname)) == 0)
      {
	 sprintf(err_buf, "extension `%.900s' not found", extname);
	 log_error(err_buf);
	 free(extname);
	 return FH_NOT_FOUND;
      }
      free(extname);
      /*
       * Extension was found, but the caller expects to find the keywords
       * in `hu' which they passed in, not in `ehu'.  So the next step is
       * either to replace or add to the cards already in `hu' to make it
       * look like the extension.  Replace or add depends on INHERIT:
       */
      if (fh_get_bool(ehu, "INHERIT", &inherit) != FH_SUCCESS ||
	  inherit == FH_FALSE)
      {
	 /*
	  * Do not inherit keywords from the parent.
	  */
	 while (list->len)
	    free(list->hdr[--list->len]);
      }
      /*
       * In any case, do not inherit EXTEND and NEXTEND keywords.
       */
      fh_remove(hu, "EXTEND");
      fh_remove(hu, "NEXTEND");
      invalidate_hu_cache(list);
      /*
       * Change `hu' into the EHU by copying the following:
       */
      list->off_hdrs = FH_HU(ehu)->off_hdrs;
      list->off_data = FH_HU(ehu)->off_data;
      list->fd_header_blocks = FH_HU(ehu)->fd_header_blocks;
      /*
       * Now merge all the cards from the EHU into `hu'.
       */
      fh_merge(hu, ehu);
      fh_destroy(list->ehu);
      list->ehu = 0;
   }

   return FH_SUCCESS;
}

int
fh_file_desc(HeaderUnit hu)
{
   return FH_HU(hu)->fd;
}

const char*
fh_next(HeaderUnit hu)
{
   HeaderUnitStruct* list = FH_HU(hu);

   if (!list || !list->len) return 0;
   if (list->pos < list->len)
      return list->hdr[list->pos++]->card;
   return 0;
}

const char*
fh_first(HeaderUnit hu)
{
   HeaderUnitStruct* list = FH_HU(hu);

   if (!list) return 0;
   list->pos = 0;
   return fh_next(hu);
}

double
fh_idx(HeaderUnit hu)
{
   HeaderUnitStruct* list = FH_HU(hu);

   if (!list || !list->len) return 0;
   if (list->pos > list->len || list->pos < 1) return 0;
   return list->hdr[list->pos-1]->idx;
}

void
fh_set_str(HeaderUnit hu, double idx,
	   const char* name, const char* value, const char* comment)
{
   HeaderUnitStruct* list = FH_HU(hu);
   char* s;
   int col = FH_NAME_SIZE;

   if (!list || !name || !value)
   {
      log_error("Invalid argument passed to fh_set_str()");
      if (list) list->err_invalid++;
      return;
   }

   /*
    * First set this value for all extensions too, if applicable.
    */
   if (list->set_all_units)
   {
      int ext;

      for (ext = 1; ext <= fh_extensions(list); ext++)
	 fh_set_str(fh_ehu(hu, ext), idx, name, value, comment);
   }

   s = get_card(list, idx, name, 0); /* Replace any other with same name */
   if (!s) return; /* malloc problem; will be caught later by fh_rewrite() */
   s[col++] = '=';
   s[col++] = ' ';
   s[col++] = '\'';
   while (col < 78 && *value)
   {
      if (*value < ' ' || *value >= 127)
      {
	 sprintf(err_buf, "illegal character %x in string FITS card", *value++);
	 log_warning(err_buf);
	 s[col++] = '_';
      }
      else if (*value == '\'')
      {
	 s[col++] = '\'';
	 s[col++] = '\'';
	 value++;
      }
      else
      {
	 s[col++] = *value++;
      }
   }
   /* col could be as high as 79 now, if the last char was a '' */

   while (col < 19)
      s[col++] = ' '; /* Old FITS standard wants this :-/ */

   s[col++] = '\'';

   add_comment(s, col, comment);
}

void
fh_set_com(HeaderUnit hu, double idx,
	   const char* name, const char* comment)
{
   HeaderUnitStruct* list = FH_HU(hu);
   int col = FH_NAME_SIZE;
   char* s;

   if (!list || !name || !comment)
   {
      log_error("invalid argument to fh_set_com()");
      if (list) list->err_invalid++;
      return;
   }

   /*
    * First set this value for all extensions too, if applicable.
    */
   if (list->set_all_units)
   {
      int ext;

      for (ext = 1; ext <= fh_extensions(list); ext++)
	 fh_set_com(fh_ehu(hu, ext), idx, name, comment);
   }

   s = get_card(list, idx, name, idx); /* Only _replace_ if idx matches */
   if (!s) return; /* malloc problem; will be caught later by fh_rewrite() */
   while (col < FH_CARD_SIZE && *comment)
   {
      if (*comment)
      {
	 if (*comment < ' ' || *comment >= 127)
	 {
	    sprintf(err_buf, "illegal character %x in comment FITS card", *comment++);
	    log_warning(err_buf);
	    s[col++] = '_';
	 }
	 else
	 {
	    s[col++] = *comment++;
	 }
      }
      else
	 s[col++] = ' ';
   }
}

void
fh_set_int(HeaderUnit hu, double idx,
	   const char* name, int value, const char* comment)
{
   HeaderUnitStruct* list = FH_HU(hu);
   int col = FH_NAME_SIZE;
   char* s;

   if (!list || !name)
   {
      log_error("invalid argument to fh_set_int()");
      if (list) list->err_invalid++;
      return;
   }

   invalidate_hu_cache(list); /* In case this changed BITPIX or NAXIS */

   /*
    * First set this value for all extensions too, if applicable.
    */
   if (list->set_all_units)
   {
      int ext;

      for (ext = 1; ext <= fh_extensions(list); ext++)
	 fh_set_int(fh_ehu(hu, ext), idx, name, value, comment);
   }

   s = get_card(list, idx, name, 0); /* Replace any other with same name */
   if (!s) return; /* malloc problem; will be caught later by fh_rewrite() */
   s[col++] = '=';
   s[col++] = ' ';

   col += sysv_sprintf(s + col, "%20d", value);
   s[col] = ' '; /* Remove the \0 that sprintf added (add_comment overwrites it too) */

   add_comment(s, col, comment);
}

void
fh_set_flt(HeaderUnit hu, double idx,
	   const char* name, double value,
	   int significant_digits, const char* comment)
{
   HeaderUnitStruct* list = FH_HU(hu);
   int col = FH_NAME_SIZE;
   char* s;

   if (!list || !name)
   {
      log_error("invalid argument to fh_set_flt()");
      if (list) list->err_invalid++;
      return;
   }

   /*
    * Sanity-check the significant_digits parameter and silently
    * limit it to the size of the FITS card to prevent buffer
    * overruns.  This is probably somewhat C-library dependent,
    * and certainly dependent on the range of exponents that can
    * occur in the floating point value being printed.  It assumes
    * the optional "E+<exponent>" which may get printed will be at
    * most 3 digits.  In that case, 62 significant digits and the
    * space (or minus sign) in front of the number plus the decimal
    * point will result in using all but the last column in the 80
    * character FITS card.
    */
   if (significant_digits < 1 || significant_digits > MAX_SIG_FIGS)
      significant_digits = MAX_SIG_FIGS;

   /*
    * First set this value for all extensions too, if applicable.
    */
   if (list->set_all_units)
   {
      int ext;

      for (ext = 1; ext <= fh_extensions(list); ext++)
	 fh_set_flt(fh_ehu(hu, ext), idx, name, value, significant_digits, comment);
   }

   s = get_card(list, idx, name, 0); /* Replace any other with same name */
   if (!s) return; /* malloc problem; will be caught later by fh_rewrite() */
   s[col++] = '=';
   s[col++] = ' ';
   col += sysv_sprintf(s + col, "%# 20.*G", significant_digits, value);
   s[col] = ' '; /* Remove the \0 that sprintf added for us */

   add_comment(s, col, comment);
}

void
fh_set_pfl(HeaderUnit hu, double idx,
	   const char* name, double value,
	   int decimal_places, const char* comment)
{
   HeaderUnitStruct* list = FH_HU(hu);
   int col = FH_NAME_SIZE;
   char* s;
   double i, hival = 0;

   if (!list || !name)
   {
      log_error("invalid argument to fh_set_flt()");
      if (list) list->err_invalid++;
      return;
   }

   /*
    * If the value is too big (or too negative) to be expressed
    * without exponent with the number of columns that a FITS card
    * allows, fall back to the other way of formatting floats.
    * Also do this if decimal_places is more than MAX_SIG_FIGS.
    * The alternate float-formatting routine fh_set_flt() will be
    * used at the limit of MAX_SIG_FIGS and may automatically
    * switch to E-notation, even if fh_set_pfl() was used.
    * This is necessary to prevent buffer overflows and potential
    * memory corruption when very large magnitude numbers get
    * passed to fh_set_pfl().
    *
    * For performance reasons, the first few cases are pre-calculated
    * in a switch statement.  For more than 9 decimal places, the
    * CPU will have to do a bunch of divide by 10's for each value
    * passed to fh_set_pfl() to figure out if it is going to fit or
    * not.  Note that we don't want to suck in a dependency on libm
    * and use log10().  We also didn't want to just switch to snprintf()
    * because we were not sure if all the systems we might want to use
    * libfh on all have that function.  (Plus, snprintf would probably
    * corrupt and truncate the number.  This solution does not.)
    */
   switch (decimal_places)
   {
      case 0: hival = 1.0E66; break;
      case 1: hival = 1.0E65; break;
      case 2: hival = 1.0E64; break;
      case 3: hival = 1.0E63; break;
      case 4: hival = 1.0E62; break;
      case 5: hival = 1.0E61; break;
      case 6: hival = 1.0E60; break;
      case 7: hival = 1.0E59; break;
      case 8: hival = 1.0E58; break;
      case 9: hival = 1.0E57; break;
      default:
      {
	 if (decimal_places < MAX_SIG_FIGS)
	 {
	    hival = 1.0E56;
	    for (i = 10; i < decimal_places; i++)
	       hival /= 10.;
	 }
      }
   }
   if (value >= hival || value <= -hival)
   {
      fh_set_flt(hu, idx, name, value, MAX_SIG_FIGS, comment);
      return;
   }

   /*
    * First set this value for all extensions too, if applicable.
    */
   if (list->set_all_units)
   {
      int ext;

      for (ext = 1; ext <= fh_extensions(list); ext++)
	 fh_set_pfl(fh_ehu(hu, ext), idx, name, value, decimal_places, comment);
   }

   s = get_card(list, idx, name, 0); /* Replace any other with same name */
   if (!s) return; /* malloc problem; will be caught later by fh_rewrite() */
   s[col++] = '=';
   s[col++] = ' ';
   col += sysv_sprintf(s + col, "%# 20.*f", decimal_places, value);
   s[col] = ' '; /* Remove the \0 that sprintf added for us */

   add_comment(s, col, comment);
}

void
fh_set_val(HeaderUnit hu, double idx,
	   const char* name, const char* value, const char* comment)
{
   HeaderUnitStruct* list = FH_HU(hu);
   int col = FH_NAME_SIZE;
   int len = strlen(value);
   char* s;
   char* endptr;

   if (!list || !name || !value)
   {
      log_error("invalid argument to fh_set_val()");
      if (list) list->err_invalid++;
      return;
   }

   /*
    * If the value is not boolean or numeric, save it as a string.
    */
   strtod(value, &endptr);
   if (strcmp(value, "T") &&
       strcmp(value, "F") &&
       (!endptr || *endptr!='\0'))
   {
      /*
       * Strip quotes if needed.
       */
      if (*value == '\'' && strlen(value) > 1 &&
	  value[strlen(value) - 1] == '\'')
      {
	 char* newval = strdup(value + 1);

	 newval[strlen(newval) - 1] = '\0';
	 fh_set_str(hu, idx, name, newval, comment);
	 free(newval);
	 return;
      }
      fh_set_str(hu, idx, name, value, comment);
      return;
   }

   /*
    * First set this value for all extensions too, if applicable.
    */
   if (list->set_all_units)
   {
      int ext;

      for (ext = 1; ext <= fh_extensions(list); ext++)
	 fh_set_val(fh_ehu(hu, ext), idx, name, value, comment);
   }

   if (len > 70)
   {
      sprintf(err_buf, "truncating value for [%.8s]", name);
      log_warning(err_buf);
   }

   s = get_card(list, idx, name, 0); /* Replace any other with same name */
   if (!s) return; /* malloc problem; will be caught later by fh_rewrite() */
   s[col++] = '=';
   s[col++] = ' ';
   while (len < 20) { s[col++] = ' '; len++; } /* Right justify */
   col += sysv_sprintf(s + col, "%.70s", value);
   if (col < FH_CARD_SIZE)
   {
      s[col] = ' '; /* Fix the \0 that sprintf added for us */
      add_comment(s, col, comment);
   }
}

void
fh_set_bool(HeaderUnit hu, double idx,
	    const char* name, fh_bool value, const char* comment)
{
   HeaderUnitStruct* list = FH_HU(hu);
   int col = FH_NAME_SIZE;
   char* s;

   if (!list || !name)
   {
      log_error("invalid argument to fh_set_bool()");
      if (list) list->err_invalid++;
      return;
   }

   /*
    * First set this value for all extensions too, if applicable.
    */
   if (list->set_all_units)
   {
      int ext;

      for (ext = 1; ext <= fh_extensions(list); ext++)
	 fh_set_bool(fh_ehu(hu, ext), idx, name, value, comment);
   }

   s = get_card(list, idx, name, 0); /* Replace any other with same name */
   if (!s) return; /* malloc problem; will be caught later by fh_rewrite() */
   s[col++] = '=';
   while (col < 29) s[col++] = ' ';
   s[col++] = value?'T':'F';

   add_comment(s, col, comment);
}

void
fh_set_card(HeaderUnit hu, double idx, const char* card)
{
   HeaderUnitStruct* list = FH_HU(hu);
   const char* name = 0;
   char* s;
   int len;

   if (!list || !card)
   {
      log_error("invalid argument to fh_set_card()");
      if (list) list->err_invalid++;
      return;
   }

   len = strlen(card);
   if (len > FH_CARD_SIZE)
   {
      log_error("card too long for fh_set_card()");
      if (list) list->err_invalid++;
      return;
   }

   /*
    * First set this value for all extensions too, if applicable.
    */
   if (list->set_all_units)
   {
      int ext;

      for (ext = 1; ext <= fh_extensions(list); ext++)
	 fh_set_card(fh_ehu(hu, ext), idx, card);
   }

   if (len > FH_NAME_SIZE && card[FH_NAME_SIZE] == '=') name = card;
   s = get_card(list, idx, name, 0); /* Replace any other with same name */
   if (!s) return; /* malloc problem; will be caught later by fh_rewrite() */
   memcpy(s, card, len);
}

fh_result
fh_get_bool(HeaderUnit hu, const char* name, fh_bool* value)
{
   HeaderUnitStruct* list = FH_HU(hu);
   const char* p;

   if (!list || !name || !value) return FH_INVALID; /* Bad arguments */
   if (!(p = get_value(list, name))) return FH_NOT_FOUND;
   if (p[0] == 'F' && p[1] == ' ') { *value=FH_FALSE; return FH_SUCCESS; }
   if (p[0] == 'T' && p[1] == ' ') { *value=FH_TRUE; return FH_SUCCESS; }
   return FH_BAD_VALUE; /* Card is not T or F? */
}

fh_result
fh_get_int(HeaderUnit hu, const char* name, int* value)
{
   HeaderUnitStruct* list = FH_HU(hu);
   char* endptr;
   const char* p;

   if (!list || !name || !value) return FH_INVALID; /* Bad arguments */
   if (!(p = get_value(list, name))) return FH_NOT_FOUND;
   *value = (int)strtol(p, &endptr, 0);
   if (endptr && *endptr==' ') return FH_SUCCESS;
   return FH_BAD_VALUE; /* Card is not an integer? */
}

fh_result
fh_get_flt(HeaderUnit hu, const char* name, double* value)
{
   HeaderUnitStruct* list = FH_HU(hu);
   char* endptr;
   const char* p;

   if (!list || !name || !value) return FH_INVALID; /* Bad arguments */
   if (!(p = get_value(list, name))) return FH_NOT_FOUND;
   *value = strtod(p, &endptr);
   if (endptr && *endptr==' ') return FH_SUCCESS;
   return FH_BAD_VALUE; /* Card is not an valid double? */
}

fh_result
fh_get_str(HeaderUnit hu, const char* name, char* value, int maxlen)
{
   HeaderUnitStruct* list = FH_HU(hu);
   fh_result rtn = FH_SUCCESS;
   const char* p;
   char* valp = value;
   int len = 0;
   int free_format = 0;

   if (!list || !name || !value || !maxlen) return FH_INVALID;
   if (!(p = get_value(list, name))) return FH_NOT_FOUND;
   if (*p++!='\'') return FH_BAD_VALUE; /* Not a string */
   while (1)
   {
      if (!*p) { rtn=FH_BAD_VALUE; break; } /* Unterminated string */
      if (*p == '\'')
      {
	 if (p[1] == '\'') p++;	/* Literal single quote character in string? */
	 else break;		/* Or actual end of string? */
      }
      if (++len > maxlen) { rtn=FH_BAD_VALUE; break; } /* value buf too small */
      *valp++ = *p++;
   }
   /*
    * Make sure there are no other characters (other than space) between
    * the end of string and the start of the comment field.
    */
   if (*p == '\'')
   {
      /*
       * Check for strings which are not in "fixed" format.
       */
      if (len < 8) free_format = 1;
      while (*(++p) == ' ');
      if (*p != '\0' && *p != '/') rtn=FH_BAD_VALUE;
   }
   while (len && *(valp - 1)==' ')
   { valp--; len--; } /* Trim white space off end, but not beginning */
   *valp++ = '\0';
   if (free_format && rtn == FH_SUCCESS)
   {
      static int warned = 0;
      int tmp;
      
      if (!warned)
      {
	 log_warning("converting strings to \"fixed\" format");
	 warned = 1;
      }
      tmp = list->set_all_units; /* === Hack to prevent EHU's from being affected. */
      list->set_all_units = 0;
      fh_set_str(hu, 0, name, value, 0);
      list->set_all_units = tmp;
   }
   return rtn;
}

fh_result
fh_search(HeaderUnit hu, const char* name, double* idx)
{
   HeaderUnitStruct* list = FH_HU(hu);
   FitsCardPtr hdr;

   if (!list || !name) return FH_INVALID;
   hdr = get_hdr(list, name, 0);
   if (!hdr) return FH_NOT_FOUND;
   if (idx) *idx = hdr->idx;
   return FH_SUCCESS;
}

double
fh_idx_after(HeaderUnit hu, const char* name)
{
   double idx;

   if (fh_search(hu, name, &idx) == FH_SUCCESS)
      return idx + 0.00001;
   else
      return 0.0;
}

double
fh_idx_before(HeaderUnit hu, const char* name)
{
   double idx;

   if (fh_search(hu, name, &idx) == FH_SUCCESS)
      return idx - 0.00001;
   else
      return 0.0;
}

fh_result
fh_remove(HeaderUnit hu, const char* name)
{
   HeaderUnitStruct* list = FH_HU(hu);
   double idx = 0.; /* For future use? */
   int i;

   if (!list || !name) return FH_INVALID;

   for (i = 0; i < list->len; i++)
   {
      if (fh_cmp(list->hdr[i]->card, name)>=MATCH_KEYWORD &&
	  (idx == 0 || list->hdr[i]->idx == idx))
      {
	 free(list->hdr[i]);
	 list->hdr[i] = 0;
	 /*
	  * Shift the rest of the list (this only has to copy pointers)
	  */
	 for (i++ ; i < list->len; i++)
	    list->hdr[i-1] = list->hdr[i];
	 list->len--;
	 return FH_SUCCESS;
      }
   }
   return FH_NOT_FOUND;
}

int
fh_extensions(HeaderUnit hu)
{
   HeaderUnitStruct* list = FH_HU(hu);
   fh_bool extend;
   int nextend;

   if (!list)
   {
      log_error("invalid argument to fh_extensions()");
      return 0;
   }
   if (list->extensions != -1)
      return list->extensions;
   if (fh_get_bool(list, "EXTEND", &extend)!=FH_SUCCESS ||
       fh_get_int(list, "NEXTEND", &nextend)!=FH_SUCCESS)
      nextend = 0;

   list->extensions = nextend;
   return nextend;
}

int
fh_header_blocks(HeaderUnit hu)
{
   HeaderUnitStruct* list = FH_HU(hu);
   int rtn;

   if (!list)
   {
      log_error("invalid argument to fh_header_blocks()");
      return 0;
   }

   /*
    * Calculate the minimum number of header blocks required to store
    * all of the keywords plus an END line.
    */
   rtn = list->len + 1; /* +1 is for the END line */
   if (list->reserve)
   {
      /*
       * If reserve keywords are requested, the header is written with
       * special COMMENT lines and the way END is written is also changed:
       * the END line is never placed in the very last slot.  Instead, it
       * always goes in the second-to-last slot (to avoid triggering bugs
       * in other FITS reading software that may have a problem with this
       * boundary condition.)
       */
      rtn +=
	 list->reserve + 1; /* +1 is for the extra blank line at the end */
   }
   rtn = (rtn * FH_CARD_SIZE + FH_BLOCK_SIZE - 1) / FH_BLOCK_SIZE;

   /*
    * Bump size up to previous size.  It is possible to remove enough
    * keywords that the header shrinks by a block, but we never actually
    * write back a smaller header (for one thing, fh_rewrite() wouldn't
    * work at all if we did.)
    */
   if (rtn < list->fd_header_blocks)
      rtn = list->fd_header_blocks;

   return rtn;
}

static int
fh_xtension(HeaderUnit hu)
{
   HeaderUnitStruct* list = FH_HU(hu);
   if (list->xtension == -1)
   {
      char tmp[80];
      if (fh_get_str(list, "XTENSION", tmp, sizeof(tmp)) != FH_SUCCESS)
	 list->xtension = FH_XTENSION_UNKNOWN;
      else if (!strcmp(tmp, "IMAGE"))
	 list->xtension = FH_XTENSION_IMAGE;
      else if (!strcmp(tmp, "TABLE"))
	 list->xtension = FH_XTENSION_TABLE;
      else
	 list->xtension = FH_XTENSION_UNKNOWN;
   }
   return list->xtension;
}

static int
fh_bitpix(HeaderUnit hu)
{
   HeaderUnitStruct* list = FH_HU(hu);

   if (list->bitpix == -1)
      if (fh_get_int(list, "BITPIX", &list->bitpix)!=FH_SUCCESS)
	 list->bitpix = -1;

   return list->bitpix;
}

static int
fh_bytepix(HeaderUnit hu)
{
   switch (fh_bitpix(hu))
   {
      case 8:
      case -8: return 1; break;
      case 16:
      case -16: return 2; break;
      case 32:
      case -32: return 4; break;
      case 64:
      case -64: return 8; break;
      default:
      {
	 log_error("invalid or missing BITPIX card");
	 return 0;
      }
   }
}

int
fh_image_bytes(HeaderUnit hu)
{
   HeaderUnitStruct* list = FH_HU(hu);
   int naxis = 0;
   int n_bytes = 1;
   int tmp;

   if (!list)
   {
      log_error("invalid argument to fh_image_bytes()");
      return 0;
   }
   if (list->image_bytes != -1)
      return list->image_bytes;

   if (fh_get_int(list, "NAXIS", &naxis)!=FH_SUCCESS || naxis<1 || naxis>999)
      return 0;

   while (naxis)
   {
      char naxis_hdr[9];

      sprintf(naxis_hdr, "NAXIS%d", naxis);
      if (fh_get_int(list, naxis_hdr, &tmp)!=FH_SUCCESS || tmp < 1)
      {
	 sprintf(err_buf, "missing %.900s card", naxis_hdr);
	 log_error(err_buf);
	 return 0;
      }
      n_bytes *= tmp;
      naxis--;
   }
   n_bytes *= fh_bytepix(hu);
   /*
    * The following allows BINTABLE extensions to be copied correctly,
    * even though libfh does not provide support for interpreting them.
    */
   if (fh_get_int(list, "PCOUNT", &tmp) == FH_SUCCESS)
      n_bytes += tmp;
   list->image_bytes = n_bytes;
   list->image_bytes_left = n_bytes;
   return n_bytes;
}

int
fh_image_blocks(HeaderUnit hu)
{
   return ((fh_image_bytes(hu) + FH_BLOCK_SIZE - 1) / FH_BLOCK_SIZE);
}

fh_result
fh_merge(HeaderUnit hu, const HeaderUnit source_)
{
   HeaderUnitStruct* list = FH_HU(hu);
   const HeaderUnitStruct* source = FH_HU(source_);
   int i = 0;
   char* s;

   if (!list || !source)
   {
      log_error("invalid argument passed to fh_merge()");
      return FH_INVALID;
   }

   while (i < source->len)
   {
      double idxmatch;
      char name[9] = "        ";

      /*
       * No `=' in the FITS card?  In that case, create a new entry
       * unless the idx values match exactly.  Otherwise, leave idxmatch
       * set to 0, which tells get_card() to re-use an old card if
       * it had the same name.
       */
      if (source->hdr[i]->card[8] != '=' ||
	  !memcmp(source->hdr[i]->card, "COMMENT ", 8) ||
	  !memcmp(source->hdr[i]->card, "HISTORY ", 8) ||
	  !memcmp(source->hdr[i]->card, "        ", 8))
	 idxmatch = source->hdr[i]->idx;
      else
	 idxmatch = 0.;
      memcpy(name, source->hdr[i]->card, 8);
      s = get_card(list, source->hdr[i]->idx, name, idxmatch);
      if (!s) return FH_NO_MEMORY;
      memcpy(s, source->hdr[i]->card, FH_CARD_SIZE);
      i++;
   }
   return FH_SUCCESS;
}

static void
pad_header(HeaderUnit hu, int n)
{
   int i;
   double idx = 900000000;

   for (i = 0; i < n; i++)
   {
      fh_set_card(hu, idx++, FH_RESERVE);
   }
}

fh_result
fh_copy(HeaderUnit hu, const HeaderUnit source_)
{
   fh_result result;
   int reserve;

   result = fh_merge(hu, source_);
   if (result != FH_SUCCESS) return result;
   reserve = fh_get_reserve(source_);
   if (reserve) pad_header(hu, reserve);
   return FH_SUCCESS;
}

fh_result
fh_read_keyword_buffer(HeaderUnit hu, const char* buffer, double idx, int fast)
{
   HeaderUnitStruct* list = FH_HU(hu);
   char* s;
   int cards = FH_BLOCK_SIZE/FH_CARD_SIZE;

   if (!list || !buffer)
   {
      log_error("invalid argument passed to fh_read_keyword_buffer()");
      return FH_INVALID;
   }

   while (cards)
   {
      s = get_card(list, idx, 0, 0); /* 0, 0 means no check for existing */
      if (!s) return FH_NO_MEMORY;
      idx = 0.0; /* Switch to auto-increment mode. */

      do
      {
	 memcpy(s, buffer, FH_CARD_SIZE);
	 buffer += FH_CARD_SIZE;
	 cards --;
	 if (!fast && !memcmp(s, FH_RESERVE, FH_RESERVE_LEN))
	 {
	    list->reserve_found++;
	    continue; /* Skip FH_RESERVE lines */
	 }
	 break;
      } while (1);

      if (!fast && fix_characters(s, /*repair=*/0) != FH_SUCCESS)
      {
	 sprintf(err_buf, "FITS format error in card [%.80s]", s);
	 log_error(err_buf);
	 return FH_BAD_VALUE;
      }
      if (!memcmp(s, "END     ", 8))
      {
	 double idx_auto;
	 int i;

	 free(list->hdr[--list->len]); /* Remove the END */
	 while (!fast && list->len &&
		strspn(list->hdr[list->len-1]->card, " ")==FH_CARD_SIZE)
	    free(list->hdr[--list->len]); /* Remove trailing blank lines
					   * and FH_RESERVE lines
					   */

	 /*
	  * fh_read() is the only case where get_card() is called without
	  * knowing what the keyword name is.  In order to enforce proper
	  * `idx' numbers for the reserved keywords, one pass through all
	  * the entries must be made here to correct the values.
	  */
	 if (!fast)
	 {
	    for (i = 0; i < list->len; i++)
	    {
	       idx_auto = auto_idx(list->hdr[i]->card);
	       if (idx_auto < 10.0) list->hdr[i]->idx = idx_auto;
	    }
	    /* === Also do padding check if !fast? */
	 }
	 return FH_END_OF_FILE; /* This block included END. */
      }
   }
   return FH_SUCCESS; /* More to read... */
}

fh_result
fh_read(HeaderUnit hu, int fd, double idx)
{
   HeaderUnitStruct* list = FH_HU(hu);
   int bytes_read = 0;
   char* s;
   fh_result rtn = FH_SUCCESS;

   if (!list)
   {
      log_error("invalid argument passed to fh_read()");
      return FH_INVALID;
   }

   list->fd = fd;
   list->off_hdrs = lseek(fd, 0, SEEK_CUR);
   list->off_data = (off_t)-1; /* Gets set before returning FH_SUCCESS */
   
   while (1)
   {
      s = get_card(list, idx, 0, 0); /* 0, 0 means no check for existing */
      if (!s) return FH_NO_MEMORY;
      idx = 0.0; /* Switch to auto-increment mode. */

      /* Get the next (non-reserve-space) FITS card */
      do
      {
	 switch (read_file(fd, s, FH_CARD_SIZE))
	 {
	    case -1:
	    {
	       log_perror("failed to read FITS card");
	       return FH_IN_ERRNO;
	       break;
	    }
	    case 0:
	    {
	       /*
		* For use in programs like fitspipe, it can be
		* expected to have end of file occur, but not
		* if some of the header was already found (i.e.,
		* an error message is only printed if the EOF
		* occurs in the middle of a header read, not at
		* the beginning.)  Either way, FH_END_OF_FILE is
		* returned so the caller can do with that as
		* they wish.
		*/
	       if (bytes_read != 0)
		  log_error("unexpected end of file");
	       return FH_END_OF_FILE;
	       break;
	    }
	    case FH_CARD_SIZE: break; /* success */
	    default:
	    {
	       log_error("short read on FITS card");
	       return FH_BAD_VALUE;
	    }
	 }
	 bytes_read += FH_CARD_SIZE;
	 if (!memcmp(s, FH_RESERVE, FH_RESERVE_LEN))
	 {
	    list->reserve_found++;
	    continue; /* Skip FH_RESERVE lines */
	 }
	 break;
      } while (1);

      if (fix_characters(s, /*repair=*/0) != FH_SUCCESS)
      {
	 sprintf(err_buf, "FITS format error in card [%.80s]", s);
	 log_error(err_buf);
	 rtn = FH_BAD_VALUE;
      }
      if (!memcmp(s, "END     ", 8))
      {
	 int first_padding = 1;
	 double idx_auto;
	 int i;

	 free(list->hdr[--list->len]); /* Remove the END */
	 while (list->len &&
		strspn(list->hdr[list->len-1]->card, " ")==FH_CARD_SIZE)
	    free(list->hdr[--list->len]); /* Remove trailing blank lines
					   * and FH_RESERVE lines
					   */

	 /*
	  * fh_read() is the only case where get_card() is called without
	  * knowing what the keyword name is.  In order to enforce proper
	  * `idx' numbers for the reserved keywords, one pass through all
	  * the entries must be made here to correct the values.
	  */
	 for (i = 0; i < list->len; i++)
	 {
	    idx_auto = auto_idx(list->hdr[i]->card);
	    if (idx_auto < 10.0) list->hdr[i]->idx = idx_auto;
	 }

	 /*
	  * Verify the padding.
	  */
	 while ((bytes_read % FH_BLOCK_SIZE)!=0)
	 {
	    char tmp[FH_CARD_SIZE+1];
	    int rd;
	    
	    rd = read_file(fd, tmp, FH_CARD_SIZE);
	    if (rd == -1)
	    {
	       log_perror("FITS cards not properly padded");
	       return FH_IN_ERRNO;
	    }
	    tmp[FH_CARD_SIZE] = '\0';
	    if (rd == 0 && first_padding)
	    {
	       /*
		* If there is no padding at all, this is OK (might be useful
		* for generating templates which this library needs to read.)
		* However, don't set fd_header_blocks... just return here.
		* That way, the library will refuse to *update* one of these
		* truncated header files, but can read in the headers without
		* complaints.
		*/
	       return rtn;
	    }
	    if (rd != FH_CARD_SIZE)
	    {
	       log_error("FITS cards not properly padded");
	       return FH_BAD_PADDING;
	    }
	    bytes_read += FH_CARD_SIZE;
	    first_padding = 0;

	    if (strspn(tmp, " ") != FH_CARD_SIZE)
	    {
	       log_warning("header padding contains characters other than ' '");
	       return FH_BAD_PADDING;
	    }
	 }
	 list->off_data = lseek(fd, 0, SEEK_CUR);
	 list->fd_header_blocks = bytes_read / FH_BLOCK_SIZE;
	 return rtn;
      }
   }
   return FH_INVALID; /* Not reached */
}


fh_result
fh_reserve(HeaderUnit hu, int n)
{
   HeaderUnitStruct* list = FH_HU(hu);

   if (!list)
   {
      log_error("invalid argument to fh_reserve()");
      return FH_INVALID;
   }

   list->reserve = n;
   return FH_SUCCESS;
}

int
fh_get_reserve(HeaderUnit hu)
{
   HeaderUnitStruct* list = FH_HU(hu);

   if (!list)
   {
      log_error("invalid argument to fh_get_reserve()");
      return -1;
   }

   if (list->reserve)
      return list->reserve;
   else
      return list->reserve_found;
}

fh_result
fh_rewrite(HeaderUnit hu)
{
   HeaderUnitStruct* list = FH_HU(hu);

   fh_result rtn;

   if (!list)
   {
      log_error("invalid argument to fh_rewrite()");
      return FH_INVALID;
   }

   if (list->counting_cards)
   {
      show_card_count(hu);
      return FH_SUCCESS;
   }

   if (!(list->fd_header_blocks) ||
       (list->fd == -1))
   {
      log_error("fh_rewrite only valid on headers created from fh_read()");
      return FH_INVALID;
   }

   /*
    * First rewrite all extensions too, if applicable.
    */
   if (list->set_all_units)
   {
      fh_result tmp;
      int ext;

      for (ext = 1; ext <= fh_extensions(list); ext++)
         if ((tmp = fh_rewrite(fh_ehu(hu, ext))) != FH_SUCCESS)
	    return tmp;
   }

   /*
    * If the number of blocks that will be written isn't still
    * equal to the number of blocks found in the original file,
    * it won't fit.  Return FH_NO_SPACE error.
    */
   if (fh_header_blocks(list) != list->fd_header_blocks)
   {
      sprintf(err_buf,
	      "need %d blocks for new header, have %d; "
	      "increase upstream fh_reserve()",
	      fh_header_blocks(list), list->fd_header_blocks);
      log_error(err_buf);
      return FH_NO_SPACE;
   }

   /*
    * Seek to the start of the old headers.
    */
   if (seek_file(list->fd, list->off_hdrs) != 0)
      return FH_IN_ERRNO; /* seek_file() prints its own error message */
   /* === IN_ERRNO is not always correct here ! */

   /*
    * Write the new header block.
    */
   rtn = fh_write(list, list->fd); /* fh_write prints its own error message */

   /*
    * Just a quick sanity check... make sure that the file is now back at the start
    * of the data.  This should always be the case, unless there is a bug in this library.
    */
   if (rtn == FH_SUCCESS)
   {
      if (lseek(list->fd, 0, SEEK_CUR) != list->off_data)
      {
	 log_error("not at start of data after updating headers");
	 return FH_INTERNAL_ERROR;
      }
   }

   if (fh_unlock_file(hu) == -1)
      log_perror("failed to unlock file");

   return rtn;
}

static fh_result
check_latent_errors(HeaderUnitStruct* list)
{
   /*
    * The fh_set() functions do not return an errors.  However,
    * they do set a flag for the rare case that memory allocation
    * should fail, or if they were called with invalid arguments
    * (NULL pointers, etc.)  fh_write*() flushes out these errors.
    */
   if (list->err_memory)
   {
      list->err_memory = list->err_invalid = 0;
      return FH_NO_MEMORY;
   }
   if (list->err_invalid)
   {
      list->err_memory = list->err_invalid = 0;
      return FH_INVALID;
   }
   return FH_SUCCESS;
}

fh_result
fh_write_keyword_buffer(HeaderUnit hu, char* buffer, int* fits_blocks_inout)
{
   HeaderUnitStruct* list = FH_HU(hu);
   char* writebuf = buffer;
   int i = 0, writelen;
   int blocks_left;
   fh_result result;

   /*
    * Check for valid arguments.
    */
   if (!list || !buffer ||
       !fits_blocks_inout || *fits_blocks_inout < 1)
   {
      log_error("invalid argument to fh_write_keyword_buffer()");
      return FH_INVALID;
   }

   result = check_latent_errors(list);
   if (result != FH_SUCCESS) return result;

   fh_sort(list);
   blocks_left = fh_header_blocks(list);

   /*
    * Truncate the header if it will not fit.
    */
   if (blocks_left > *fits_blocks_inout)
   {
      blocks_left = *fits_blocks_inout;
      result = FH_NO_SPACE; /* At the end, return this error/warning */
   }
   else
   {
      *fits_blocks_inout = blocks_left;
   }

   while (blocks_left--)
   {
      /*
       * Initialize the header block with spaces (' ' character.)
       */
      memset(writebuf, ' ', FH_BLOCK_SIZE);

      /*
       * If there are headers remaining in the list (i < list->len)
       * then copy them in until the entire block is filled.  Obviously
       * FH_BLOCK_SIZE better be divisible by FH_CARD_SIZE (it is.)
       */
      for (writelen = 0;
	   writelen < FH_BLOCK_SIZE;
	   writelen += FH_CARD_SIZE)
      {
	 if (i < list->len)
	    memcpy(writebuf + writelen, list->hdr[i++]->card, FH_CARD_SIZE);
      }
      /*
       * Last block? Add END plus a blank line in the last two slots.
       * This relies on the calculation of fh_header_blocks() being
       * correct, and having left at least two empty slots at the
       * end of the final blocks.
       */
      if (!blocks_left)
      {
	 memset(writebuf + FH_BLOCK_SIZE - 2 * FH_CARD_SIZE, ' ', 2 * FH_CARD_SIZE);
	 memcpy(writebuf + FH_BLOCK_SIZE - 2 * FH_CARD_SIZE, "END", 3);
      }
      writebuf += FH_BLOCK_SIZE;
   }
   return result;
}

fh_result
fh_write(HeaderUnit hu, int fd)
{
   HeaderUnitStruct* list = FH_HU(hu);
   char writebuf[FH_BLOCK_SIZE];
   int i = 0;
   unsigned int writelen;
   int blocks_left;
   fh_result result;

   if (!list)
   {
      log_error("invalid argument to fh_write()");
      return FH_INVALID;
   }

   if (list->counting_cards)
   {
      show_card_count(hu);
      return FH_SUCCESS;
   }

   result = check_latent_errors(list);
   if (result != FH_SUCCESS) return result;

   list->off_hdrs = lseek(fd, 0, SEEK_CUR);

   fh_sort(list);

   blocks_left = fh_header_blocks(list);

   while (blocks_left--)
   {
      /*
       * Initialize the header block with spaces (' ' character.)
       */
      memset(writebuf, ' ', sizeof(writebuf));

      /*
       * If there are headers remaining in the list (i < list->len)
       * then copy them in until the entire block is filled.  Obviously
       * FH_BLOCK_SIZE better be divisible by FH_CARD_SIZE (it is.)
       */
      for (writelen = 0;
	   writelen < sizeof(writebuf);
	   writelen += FH_CARD_SIZE)
      {
	 if (i < list->len)
	    memcpy(writebuf + writelen, list->hdr[i++]->card, FH_CARD_SIZE);
	 /*
	  * Reserve COMMENT lines get inserted in three cases:
	  * 1) They were requested with fh_reserve().
	  * 2) They were found in the existing FITS header.
	  * 3) A header without any is being re-written with LESS blocks
	  *    such that the END would land in the wrong block.  (Which
	  *    only happens if the program deleted a bunch of keywords.)
	  */	  
	 else if (list->reserve || list->reserve_found ||
		  ((i == list->len) && (blocks_left > 0)))
	    memcpy(writebuf + writelen, FH_RESERVE, FH_RESERVE_LEN);
	 else if (i == list->len)
	 {
	    memcpy(writebuf + writelen, "END", 3);
	    i++;
	 }
      }
      /*
       * Last block? Add END plus a blank line in the last two slots.
       * This relies on the calculation of fh_header_blocks() being
       * correct, and having left at least two empty slots at the
       * end of the final blocks.
       *
       * This only happens for the case with the special "reserve" comments.
       * If list->reserve == 0, then the END was already included, immediately
       * after the last keyword.
       */
      if (!blocks_left && (list->reserve || list->reserve_found))
      {
	 memset(writebuf + sizeof(writebuf) - 2 * FH_CARD_SIZE, ' ', 2 * FH_CARD_SIZE);
	 memcpy(writebuf + sizeof(writebuf) - 2 * FH_CARD_SIZE, "END", 3);
      }
      if (write_file(fd, writebuf, sizeof(writebuf)) != sizeof(writebuf))
      {
	 log_perror("write failed");
	 return FH_IN_ERRNO;
      }
   }
   /*
    * Remember the data offset.
    */
   list->off_data = lseek(fd, 0, SEEK_CUR);

   /*
    * Remember the number of blocks written for potential fh_rewrite() later.
    */
   list->fd_header_blocks = fh_header_blocks(list);

   /*
    * Remember the file descriptor for potential mmap later.
    */
   if (list->fd == -1)
      list->fd = fd;
   return FH_SUCCESS;
}


fh_result
fh_read_padding(HeaderUnit hu, int fd)
{
   HeaderUnitStruct* list = FH_HU(hu);
   int padding_bytes = fh_image_blocks(hu) * FH_BLOCK_SIZE - fh_image_bytes(hu);
   int bytes_read;
   char buf[FH_BLOCK_SIZE];
   char* p = buf;
   char padding = '\0';

   if (!list)
   {
      log_error("invalid HeaderUnit passed to fh_read_padding");
      return FH_INVALID;
   }

   if (list->image_bytes_left)
   {
      log_error("fh_read_padding attempted with pixels left to read");
      return FH_BAD_SIZE;
   }

   if (padding_bytes)
   {
      bytes_read = read_file(fd, buf, padding_bytes);
      if (bytes_read < 0)
      {
	 log_perror("failed to read padding");
	 return FH_IN_ERRNO;
      }
      if (bytes_read != padding_bytes)
      {
	 log_error("not enough padding");
	 return FH_END_OF_FILE;
      }
   }
   if (fh_xtension(hu) == FH_XTENSION_TABLE) padding = ' ';
   while (padding_bytes--)
      if (*p++ != padding)
      {
	 if (fh_xtension(hu) == FH_XTENSION_TABLE)
	    log_error("padding contains characters other than ' '");
	 else
	    log_error("padding contains characters other than \\0");
	 return FH_BAD_PADDING;
      }

   return FH_SUCCESS;
}

/* Simple run-time test for the host byte-order, and swapping routines. */
static int _endian_test = 1;
#define is_little_endian() (*(char*)&_endian_test)
static void fh_chsign(unsigned short* buffer, int elements)
{
#if 0
   if (is_little_endian()) /* %%% Or is it always 0x0080 ? */
      while (elements--)
	 *buffer++ ^= 0x8000;
   else
#else
   while (elements--)
      *buffer++ ^= 0x0080;
#endif
}
static void fh_bswap2(unsigned short* buffer, int elements)
{
   register unsigned short temp;

   while (elements--)
   {
      temp = *buffer;
      *buffer    = (temp & 0xff00) >> 8;
      *buffer++ |= (temp & 0x00ff) << 8;
   }
}
static void fh_bswap2chsign(unsigned short* buffer, int elements)
{
   register unsigned short temp;

   while (elements--)
   {
      temp = *buffer;
      *buffer    = ((temp^0x8000) & 0xff00) >> 8;
      *buffer++ |= (temp & 0x00ff) << 8;
   }
}
static void fh_bswap4(unsigned long* buffer, int elements)
{
   register unsigned long temp;

   while (elements--)
   {
      temp = *buffer;
      *buffer    =  (temp & 0xff000000) >> 24;
      *buffer   |= (temp & 0x00ff0000) >> 8;
      *buffer   |= (temp & 0x0000ff00) << 8;
      *buffer++ |= (temp & 0x000000ff) << 24;
   }
}
static void fh_bswap8(unsigned long* buffer, int elements)
{
   register unsigned temp1, temp2;

   while (elements--)
   {
      temp1 = *buffer;
      temp2 = *(buffer + 1);
      *buffer    =  (temp2 & 0xff000000) >> 24;
      *buffer   |= (temp2 & 0x00ff0000) >> 8;
      *buffer   |= (temp2 & 0x0000ff00) << 8;
      *buffer++ |= (temp2 & 0x000000ff) << 24;
      *buffer    =  (temp1 & 0xff000000) >> 24;
      *buffer   |= (temp1 & 0x00ff0000) >> 8;
      *buffer   |= (temp1 & 0x0000ff00) << 8;
      *buffer++ |= (temp1 & 0x000000ff) << 24;
   }
}

fh_result
fh_munmap_raw_image(HeaderUnit hu)
{
   HeaderUnitStruct* list = FH_HU(hu);
   HeaderUnitStruct* primary = FH_HU(list->phu);

   if (!primary) primary = list;
   if (!list)
   {
      log_error("invalid argument to fh_munmap_raw_image()");
      return FH_INVALID;
   }

   if (list->mmap_count == 0 || primary->mmap_count == 0)
      return FH_SUCCESS; /* Don't complain if nothing to munmap? */

   list->mmap_count--;
   primary->mmap_count--;

   if (primary->mmap_count == 0)
   {
      if (munmap(primary->mmap_addr, primary->file_size) != 0)
      {
	 log_error("munmap failed");
	 return FH_IN_ERRNO;
      }
   }
   return FH_SUCCESS;
}

fh_result
fh_map_raw_image(HeaderUnit hu, void** data, int size)
{
   HeaderUnitStruct* list = FH_HU(hu);
   HeaderUnitStruct* primary = FH_HU(list->phu);

   if (!primary) primary = list;
   if (!list || !data || !size)
   {
      log_error("invalid argument to fh_map_raw_image()");
      return FH_INVALID;
   }

   if (list->fd == -1)
   {
      log_error("fh_map_raw_image attempted before fh_read");
      return FH_INVALID;
   }

   if (fh_image_bytes(hu) == 0)
   {
      log_error("image size must be set by NAXIS/BITPIX keywords before mmaping");
      return FH_BAD_SIZE;
   }

   fh_munmap_raw_image(hu); /* Remove any previous mapping. */

   /*
    * Calculate file_size now, if this is a newly created file.
    * Upon a read, file_size is determined by a stat() call.
    * When writing a new file, it is determined with the seek,
    * below:
    *
    * %%% It doesn't seem to work.  Why?
    */
   if (list->file_size == 0)
   {
      list->file_size = lseek(list->fd, 0, SEEK_END);
      if (list->file_size == (off_t)-1)
      {
	 log_perror("seek to end of file failed");
	 return -1;
      }
   }

   if (size != list->image_bytes)
   {
      sprintf(err_buf,
	      "fh_map_raw_image size (%d) does not match image_bytes (%d)",
	      size, list->image_bytes);
      log_error(err_buf);
      return FH_BAD_SIZE;
   }
   if (primary->mmap_count == 0)
   {
      primary->mmap_addr = mmap(0, primary->file_size,
				PROT_READ|PROT_WRITE, MAP_SHARED, /* %%% ### The PROT_WRITE is only temporary */
				list->fd, 0);
      if (primary->mmap_addr == (void*)-1 ||
	  primary->mmap_addr == (void*)0)
      {
	 log_perror("error: mmap failed");
	 return FH_IN_ERRNO;
      }
   }

   list->mmap_count++;
   primary->mmap_count++;

   *data = (char*)primary->mmap_addr + list->off_data;
   return FH_SUCCESS;
}

fh_result
fh_read_image(HeaderUnit hu, int fd, void* data, int size, int typesize)
{
   HeaderUnitStruct* list = FH_HU(hu);
   int need_swap = 0;
   int bytes_to_read, bytes_left;

   if (!list || !data || fd == -1)
   {
      log_error("invalid argument to fh_read_image()");
      return FH_INVALID;
   }

   if (is_little_endian()) need_swap = 1;		/* Auto */
   if (typesize < 0) { need_swap = 1; typesize *= -1; }	/* Forced on */
   else if (typesize < 2) need_swap = 0;		/* Forced off or N/A */

   if (typesize && fh_bitpix(hu) != -1 && typesize != fh_bytepix(hu))
   {
      log_error("fh_read_image typesize/BITPIX mismatch");
      return FH_BAD_SIZE;
   }

   if (fh_image_bytes(hu) == 0) /* Calculate image size if needed. */
   {
      log_error("image size must be set by NAXIS/BITPIX keywords before reading");
      return FH_BAD_SIZE;
   }

   if (size > list->image_bytes_left)
   {
      sprintf(err_buf,
	      "fh_read_image size (%d) is more than image_bytes_left (%d)",
	      size, list->image_bytes_left);
      log_error(err_buf);
      return FH_BAD_SIZE;
   }
   bytes_left = size;
   list->image_bytes_left -= size;

   while (bytes_left)
   {
      bytes_to_read = bytes_left;
      /* Do smaller reads to pipeline byte-swapping if need_swap is true */
      if (need_swap && bytes_to_read > 32768) bytes_to_read = 32768;
      if (read_file(fd, data, bytes_to_read) != bytes_to_read)
      {
	 log_perror("failed to read image data");
	 return FH_IN_ERRNO;
      }
      if (need_swap) switch (typesize)
      {
	 case 2: fh_bswap2(data, bytes_to_read / 2); break;
	 case 4: fh_bswap4(data, bytes_to_read / 4); break;
	 case 8: fh_bswap8(data, bytes_to_read / 8); break;
	 default:
	 log_error("byte-swapping only defined for 2,4,8-byte data types");
	 return FH_INVALID;
      }
      bytes_left -= bytes_to_read;
      data = (char*)data + bytes_to_read;
   }
   return FH_SUCCESS;
}

fh_result
fh_read_padded_image(HeaderUnit hu, int fd, void* data, int size, int typesize)
{
   fh_result rtn;

   if (size &&
       (rtn = fh_read_image(hu, fd, data, size, typesize)) != FH_SUCCESS)
      return rtn;

   return fh_read_padding(hu, fd);
}

fh_result
fh_write_padding(HeaderUnit hu, int fd)
{
   HeaderUnitStruct* list = FH_HU(hu);
   HeaderUnitStruct* primary = list?(FH_HU(list->phu)):0;
   int padding_bytes;

   if (!list || fd == -1)
   {
      log_error("invalid argument to fh_write_padding()");
      return FH_INVALID;
   }

   padding_bytes = fh_image_blocks(hu) * FH_BLOCK_SIZE - fh_image_bytes(hu);

   /*
    * The library will only write the padding for two cases:
    *
    * 1) The library itself has been used to write all the data,
    *    all at once or in segments, so image_bytes_left is 0.
    * 2) The library has written none of the data (image_bytes_left
    *    is equal to image_bytes) in which case it is assumed
    *    that the data was written to the file by the user.
    *
    * The main point of this check is to catch alignment problems
    * where not enough data is written to the FITS file before
    * the padding was added.
    */
   if (list->image_bytes_left != 0 &&
       list->image_bytes_left != fh_image_bytes(hu))
   {
      sprintf(err_buf,
	      "fh_write_padding attempted but %d pixels left to write",
	      list->image_bytes_left);
      log_error(err_buf);
      return FH_BAD_SIZE;
   }

   if (padding_bytes)
   {
      if (write_file(fd, padding_block(fh_xtension(hu)), padding_bytes)
	  != padding_bytes)
      {
	 log_perror("failed to write padding");
	 return FH_IN_ERRNO;
      }
   }
   /* %%% ... */
   if (list->file_size < list->off_data + fh_image_bytes(hu) + padding_bytes)
      list->file_size = list->off_data + fh_image_bytes(hu) + padding_bytes;
   if (primary && primary->file_size < list->file_size)
      primary->file_size = list->file_size; /* ... %%% */

   return FH_SUCCESS;
}

fh_result
fh_write_image(HeaderUnit hu, int fd, void* data, int size, int typesize)
{
   HeaderUnitStruct* list = FH_HU(hu);
   int need_swap = 0;
   int need_chsign = 0;
   int bytes_to_write, bytes_left;

   if (is_little_endian()) need_swap = 1;		/* Auto */
   if (typesize < 0) { need_swap = 1; typesize *= -1; }	/* Forced on */
   else if (typesize < 2 ||				/* Forced off or N/A */
	    typesize == FH_TYPESIZE_16URAW) need_swap = 0;
   if (typesize == FH_TYPESIZE_16U ||			/* 16-bit unsigned */
       typesize == FH_TYPESIZE_16URAW) { typesize = 2; need_chsign = 1; }

   if (!list || !data || fd == -1)
   {
      log_error("invalid argument to fh_write_image()");
      return FH_INVALID;
   }
   if (typesize && fh_bitpix(hu) != -1 && typesize != fh_bytepix(hu))
   {
      log_error("fh_write_padded_image typesize/BITPIX mismatch");
      return FH_BAD_SIZE;
   }
   if (fh_image_bytes(hu) == 0) /* Calculate image size if needed. */
   {
      log_error("image size must be set by NAXIS/BITPIX keywords before writing");
      return FH_BAD_SIZE;
   }
   if (size > list->image_bytes_left)
   {
      sprintf(err_buf,
	      "fh_write_image size (%d) is more than image_bytes_left (%d)",
	      size, list->image_bytes_left);
      log_error(err_buf);
      return FH_BAD_SIZE;
   }
   bytes_left = size;
   list->image_bytes_left -= size;
   if (!need_swap && !need_chsign)
   {
      if (write_file(fd, data, bytes_left) != bytes_left)
      {
	 log_perror("failed to write image data");
	 return FH_IN_ERRNO;
      }
   }
   else /* byte-swapping case requires a temporary buffer */
   {
      unsigned char* outbuf[32768];

      while (bytes_left)
      {
	 bytes_to_write = bytes_left;
	 if (bytes_to_write > sizeof(outbuf)) bytes_to_write = sizeof(outbuf);
	 memcpy(outbuf, data, bytes_to_write);
	 switch (typesize + need_chsign)
	 {
	    case 2: fh_bswap2((void*)outbuf, bytes_to_write / 2); break;
	    case 3: /* Special case that also converts unsign->sign (16-bit) */
	    if (need_swap)
	       fh_bswap2chsign((void*)outbuf, bytes_to_write / 2);
	    else
	       fh_chsign((void*)outbuf, bytes_to_write / 2);
	    break;
	    case 4: fh_bswap4((void*)outbuf, bytes_to_write / 4); break;
	    case 8: fh_bswap8((void*)outbuf, bytes_to_write / 8); break;
	    default:
	    log_error("byte-swapping only defined for 2,4,8-byte data types");
	    return FH_INVALID;
	 }
	 if (write_file(fd, outbuf, bytes_to_write) != bytes_to_write)
	 {
	    log_perror("failed to write image data");
	    return FH_IN_ERRNO;
	 }
	 bytes_left -= bytes_to_write;
	 data = (char*)data + bytes_to_write;
      }
   }
   return FH_SUCCESS;
}

fh_result
fh_reserve_padded_image(HeaderUnit hu, int fd)
{
   HeaderUnitStruct* list = FH_HU(hu);

   if (!list || fd == -1)
   {
      log_error("invalid argument to fh_reserve_padded_image()");
      return FH_INVALID;
   }
   if (list->off_data == 0 || list->off_data == (off_t)-1)
   {
      log_error("no header for fh_reserve_padded_image()");
      return FH_INVALID;
   }
   if (fh_image_bytes(hu) == 0) /* Calculate image size if needed. */
   {
      log_error("image size must be set by NAXIS/BITPIX keywords before reserving image area");
      return FH_BAD_SIZE;
   }
   if (seek_file(fd, list->off_data + list->image_bytes_left - 1) != 0)
      return FH_IN_ERRNO; /* seek_file() prints its own error message */
   if (write_file(fd, "\0", 1) != 1)
   {
      log_perror("reserve image failed");
      return FH_IN_ERRNO; /* seek_file() prints its own error message */
   }
   list->image_bytes_left = 0;
   return fh_write_padding(hu, fd);
}

fh_result
fh_write_padded_image(HeaderUnit hu, int fd, void* data, int size, int typesize)
{
   fh_result rtn;

   if (size &&
       (rtn = fh_write_image(hu, fd, data, size, typesize)) != FH_SUCCESS)
      return rtn;
   
   return fh_write_padding(hu, fd);
}

static fh_result
fh_copy_padded_image_internal(HeaderUnit hu, int fd_out, int fd_in, int verify)
{
   HeaderUnitStruct* list = FH_HU(hu);
   static char* buf = 0;
   int copy_size;
   int image_bytes;
   fh_result rtn0 = FH_SUCCESS, rtn1, rtn2;
   int zero_bytes;
   char* p;


   if (!list || fd_out == -1 || fd_in == -1)
   {
      log_error("invalid argument to fh_copy_padded_image()");
      return FH_INVALID;
   }
   if (!buf &&
       !(buf = (char*)malloc(FH_BUFFER_SIZE)))
   {
      log_error("out of memory in fh_copy_padded_image()");
      return FH_NO_MEMORY;
   }

   image_bytes = fh_image_bytes(hu);

   while (image_bytes)
   {
      int bytes;

      copy_size = image_bytes;
      if (copy_size > FH_BUFFER_SIZE) copy_size = FH_BUFFER_SIZE;
      bytes = read_file(fd_in, buf, copy_size);
      if (bytes == 0)
      {
	 log_error("fh_copy_padded_image unexpected EOF");
	 return FH_END_OF_FILE;
      }
      if (bytes == -1)
      {
	 log_perror("fh_copy_padded_image read failed");
	 return FH_IN_ERRNO;
      }
      if (bytes != copy_size)
      {
	 log_error("fh_copy_padded_image short read");
	 return FH_BAD_VALUE;
      }
      if (verify) for (p = buf, zero_bytes = 0; p < buf + copy_size; p++)
      {
	 if (*p != '\0')
	 {
	    zero_bytes = 0;
	    p = (char*)((unsigned int)p|511); /* next disk sector */
	 }
	 else if (++zero_bytes == 512)
	 {
	    /* %%% log_error? */
	    fprintf(stderr, "warning: fhtool: file contains zero-block(s)\n");
	    rtn0 = FH_BAD_VALUE;
	    break;
	 }
      }
      bytes = write_file(fd_out, buf, copy_size);
      if (bytes != copy_size)
      {
	 log_perror("fh_copy_padded_image write failed");
	 return FH_IN_ERRNO;
      }
      list->image_bytes_left -= copy_size;
      image_bytes -= copy_size;
   }

   rtn1 = fh_read_padding(hu, fd_in);
   rtn2 = fh_write_padding(hu, fd_out);
   if (rtn2 != FH_SUCCESS) return rtn2;
   if (rtn0 != FH_SUCCESS) return rtn0;
   return rtn1;
}

fh_result
fh_copy_padded_image(HeaderUnit hu, int fd_out, int fd_in)
{
   return fh_copy_padded_image_internal(hu, fd_out, fd_in, /*verify=*/0);
}

fh_result
fh_copy_and_verify_padded_image(HeaderUnit hu, int fd_out, int fd_in)
{
   return fh_copy_padded_image_internal(hu, fd_out, fd_in, /*verify=*/1);
}

void
fh_link_ehu_to_phu(HeaderUnit ehu, HeaderUnit phu)
{
   HeaderUnitStruct* ehu_list = FH_HU(ehu);
   HeaderUnitStruct* phu_list = FH_HU(phu);

   ehu_list->next = phu_list->ehu;
   phu_list->ehu = ehu_list;
   ehu_list->phu = phu_list;
}

static HeaderUnit
ehu_internal(HeaderUnitStruct* phu, const char* extname, int imageid)
{
   int i = 0, found = 0;
   off_t next_off;
   HeaderUnit* next_ehu = &(phu->ehu);

   if (phu == 0)
   {
      log_error("invalid argument to ehu function");
      return 0;
   }

   if (phu->fd == -1)
   {
      log_error("no file associated with PHU");
      return 0;
   }

   next_off = phu->off_data;

   if (fh_extensions(phu)==0)
   {
      log_error("no extensions found in file");
      return 0;
   }

   /*
    * Search all extensions for `extname' or `imageid'
    */
   while (i++ < fh_extensions(phu))
   {
      if (!*next_ehu)
      {
	 /*
	  * If this is the first pass, read the EHU into the cache.
	  */
	 *next_ehu = fh_create();
	 FH_HU(*next_ehu)->phu = phu; /* Link to phu for mmap re-use */
	 if (seek_file(phu->fd, next_off) != 0) return 0;
	 if (fh_read(*next_ehu, phu->fd, 0) != FH_SUCCESS)
	 {
	    log_error("problem with MEF structure");
	    return 0; /* File error? */
	 }
      }
      /*
       * Now look for either a matching EXTNAME or IMAGEID card.
       */
      if (extname)
      {
	 char n[FH_MAX_STRLEN + 1];

	 if (fh_get_str(*next_ehu, "EXTNAME", n, sizeof(n)) == FH_SUCCESS &&
	     fh_cmp(n, extname)>=MATCH_FULL)
	    found = 1;
      }
      else if (imageid >= 0)
      {
	 int id;

	 if (fh_get_int(*next_ehu, "IMAGEID", &id) == FH_SUCCESS &&
	     id == imageid)
	    found = 1;
      }
      else
      {
	 if (i == -imageid)
	    found = 1;
      }

      if (found)
      {
	 if (seek_file(phu->fd, FH_HU(*next_ehu)->off_data) != 0)
	    return 0;
	 return *next_ehu; /* Success! */
      }

      /*
       * Set the offset to the beginning of the next extension (but don't
       * actually seek there unless the EHU hasn't been cached yet.)
       */
      next_off = FH_HU(*next_ehu)->off_data +
	 fh_image_blocks(*next_ehu) * FH_BLOCK_SIZE;
      next_ehu = &(FH_HU(*next_ehu)->next);
   }
   return 0; /* Not found */
}

HeaderUnit
fh_ehu(HeaderUnit phu, int number)
{
   HeaderUnitStruct* list = FH_HU(phu);
   
   if (number == 0 && list)
   {
      if (seek_file(list->fd, list->off_data) != 0)
	 return 0;
      list->image_bytes_left = fh_image_bytes(phu);
      return (HeaderUnit)list;
   }
   return ehu_internal(list, 0, -number);
}

HeaderUnit
fh_ehu_by_imageid(HeaderUnit phu, int imageid)
{
   return ehu_internal(FH_HU(phu), 0, imageid);
}

HeaderUnit
fh_ehu_by_extname(HeaderUnit phu, const char* extname)
{
   return ehu_internal(FH_HU(phu), extname, 0);
}

#if 0
/* %%% Decide whether to keep this or not.*/
typedef fh_result (*fh_handler)(double idx, const char* card);

fh_result
fh_foreach(HeaderUnit hu, fh_handler handler)
{
   HeaderUnitStruct* list = FH_HU(hu);
   fh_result rtn = FH_SUCCESS;
   int i;

   for (i = 0; i < list->len ; i++)
      if (handler(list->hdr[i]->idx, handler) != FH_SUCCESS)


   return rtn;
}
#endif

fh_result
fh_show(HeaderUnit hu)
{
   HeaderUnitStruct* list = FH_HU(hu);
   int i;

   for (i = 0; i < list->len ; i++)
      printf("%.79s\n", list->hdr[i]->card);

   return FH_SUCCESS;
}

/* -------------------------------------------------------------------------
 *             Implementation of internal utility functions:
 * -------------------------------------------------------------------------
 */
static const char*
padding_block(int type)
{
   static int s_initialized = 0;
   static char s[FH_BLOCK_SIZE];
   static int z_initialized = 0;
   static char z[FH_BLOCK_SIZE];

   if (type == FH_XTENSION_TABLE) /* FITS ASCII TABLES are padded with ' ' */
   {
      if (!s_initialized)
      {
	 memset(s, ' ', sizeof(s));
	 s_initialized = 1;
      }
      return s;
   }

   if (!z_initialized) /* FITS data and IMAGE extensions are padded with \0 */
   {
      memset(z, 0, sizeof(z));
      z_initialized = 1;
   }
   return z;
}

static int
write_file(int fd, const void* buf, int len)
{
   int rtn;
   int count = 0;

   while (len)
   {
      rtn = write(fd, (const char*)buf, len);

      switch (rtn)
      {
	 case -1: if (errno == EINTR || errno == EAGAIN)
	    continue; /* retry */
	    return -1; /* permanent failure */
	 case 0: return count;
	 default:
	 {
	    len -= rtn;
	    count += rtn;
	    buf = (const char*)buf + rtn;
	 }
      }
   }
   return count;
}

static int
read_file(int fd, void* buf, int len)
{
   int rtn;
   int count = 0;

   while (len)
   {
      rtn = read(fd, (char*)buf, len);

      switch (rtn)
      {
	 case -1: if (errno == EINTR || errno == EAGAIN)
	    continue; /* retry */
	    return -1; /* permanent failure */
	 case 0: return count;
	 default:
	 {
	    len -= rtn;
	    count += rtn;
	    buf = (char*)buf + rtn;
	 }
      }
   }
   return count;
}

static int
seek_file(int fd, off_t off)
{
   if (off == (off_t)-1)
   {
      log_error("cannot seek in input");
      return -1;
   }

   if (lseek(fd, off, SEEK_SET) == (off_t)-1)
   {
      log_perror("seek failed");
      return -1;
   }
   return 0;
}

static int
fh_unlock_file(HeaderUnit hu)
{
   HeaderUnitStruct* list = FH_HU(hu);
   int rtn;

   if (list->fl.l_type != F_UNLCK && list->fd_locked != -1)
   {
      list->fl.l_type = F_UNLCK;
      do
      {
         rtn = fcntl(list->fd_locked, F_SETLKW, &list->fl);
      } while (rtn == -1 && errno == EINTR);
      return rtn;
   }
   return 0;
}

static int
fh_lock_file(HeaderUnit hu, int fd, fh_mode mode)
{
   HeaderUnitStruct* list = FH_HU(hu);
   int rtn;

   fh_unlock_file(hu); /* Remove any old lock, if needed */
   list->fd_locked = fd;
   list->fl.l_type = (mode == FH_FILE_RDONLY)? F_RDLCK : F_WRLCK;
   list->fl.l_whence = SEEK_SET;
   list->fl.l_start = 0;
   list->fl.l_len = 0;
   do
   {
      rtn = fcntl(fd, F_SETLKW, &list->fl);
   } while (rtn == -1 && errno == EINTR);
   return rtn;
}


static fh_result
fix_characters(char* s, int repair)
{
   int col;
   int bad_value = 0;

   for (col=0; col<FH_NAME_SIZE; col++)
   {
      if (s[col] == ' ')
      {
	 while (++col < FH_NAME_SIZE)
	    if (s[col] != ' ') { if (repair) s[col] = ' '; bad_value++; }
	 break;
      }
      if (s[col] != NAME_CHR(s[col]))
      { if (repair) s[col] = NAME_CHR(s[col]); bad_value++; }
   }
   
   for (col=FH_NAME_SIZE; col<FH_CARD_SIZE; col++)
      if (s[col] < ' ' || s[col] >= 127) { if (repair) s[col] = '_'; bad_value++; }

   if (bad_value) return FH_BAD_VALUE;
   return FH_SUCCESS;
}

/*
 * See if "name1" is the same as "name2".
 *
 * Only the first 8 characters are compared, or up to a '\0'.
 * If one name contains a '\0', the other must contain only
 * ' ' characters (or also a a '\0') for the match to be
 * successful.
 */
static match_result
fh_cmp(const char* name1, const char* name2)
{
   int i;

   if (!name1 || !name2) return MATCH_FAILED;
   for (i = 0; i < FH_CARD_SIZE; i++)
   {
      if (NAME_CHR(*name1) != NAME_CHR(*name2))
      {
         /* mismatch found */
         if (i >= FH_NAME_SIZE) return MATCH_KEYWORD;
         if (!*name2) return MATCH_SUBSTR;
         return MATCH_FAILED;
      }
      if (*name1) name1++;
      if (*name2) name2++;
   }
   return MATCH_FULL;
}

static int
fh_compare(const void* a, const void* b)
{
   double diff = (*((FitsCard* const*)a))->idx - (*((FitsCard* const*)b))->idx;

   if (diff < 0) return -1;
   else if (diff > 0) return 1;
   else return 0;
}

static void
fh_sort(HeaderUnit hu)
{
   HeaderUnitStruct* list = FH_HU(hu);

   if (list->len <= 0) return; /* No keywords to sort! */

   qsort(list->hdr, list->len, sizeof(FitsCardPtr), fh_compare);
}

/*
 * Passing a 0 for `idx' means match only by `name'
 */
static FitsCardPtr
get_hdr(HeaderUnitStruct* list, const char* name, double idx)
{
   FitsCardPtr hdr;
   int i;

   for (i = 0; i < list->len; i++)
   {
      hdr = list->hdr[i];
      if (fh_cmp(hdr->card, name) >= MATCH_KEYWORD &&
	  (idx == 0 || hdr->idx == idx))
	 return hdr;
   }
   return 0;
}

static const char*
get_value(HeaderUnitStruct* list, const char* name)
{
   FitsCardPtr hdr = get_hdr(list, name, 0);
   const char* p;
   fh_bool inherit;

   if (!hdr && name &&
       strcmp(name, "EXTEND") &&
       strcmp(name, "NEXTEND") &&
       strcmp(name, "INHERIT"))
   {
      if (list->phu && name &&
	  fh_get_bool(list, "INHERIT", &inherit)==FH_SUCCESS &&
	  inherit==FH_TRUE)
      {
	 /* %%% Only do this if INHERIT=T */
	 hdr = get_hdr(list->phu, name, 0);
      }
   }
   if (!hdr) return 0; /* Not found. */

   p = hdr->card + FH_NAME_SIZE;
   if (*p++ != '=') return 0; /* Something wrong with FITS file? */
   while (*p == ' ') p++;
   return p;
}

static double
auto_idx(const char* name)
{
   if (!name) return 10.0;
   if (fh_cmp(name, "SIMPLE")  >=MATCH_KEYWORD)	return 0.0;
   if (fh_cmp(name, "XTENSION")>=MATCH_KEYWORD)	return 0.0;
   if (fh_cmp(name, "BITPIX")  >=MATCH_KEYWORD)	return 1.0;
   if (fh_cmp(name, "NAXIS")   >=MATCH_SUBSTR) 	return 2.0 + 0.1 * atoi(name + 5);
   if (fh_cmp(name, "EXTEND")  >=MATCH_KEYWORD)	return 3.0;
   if (fh_cmp(name, "NEXTEND") >=MATCH_KEYWORD)	return 3.1;
   if (fh_cmp(name, "GROUPS")  >=MATCH_KEYWORD)	return 4.0;
   if (fh_cmp(name, "PCOUNT")  >=MATCH_KEYWORD)	return 5.0;
   if (fh_cmp(name, "GCOUNT")  >=MATCH_KEYWORD)	return 6.0;
   if (fh_cmp(name, "TFIELDS") >=MATCH_KEYWORD)	return 7.000;
   if (fh_cmp(name, "TFORM")   >=MATCH_SUBSTR)  return 7.0001 + 0.001*atoi(name+5);
   if (fh_cmp(name, "TBCOL")   >=MATCH_SUBSTR)  return 7.0002 + 0.001*atoi(name+5);
   if (fh_cmp(name, "INHERIT") >=MATCH_KEYWORD) return 9.0;
   if (fh_cmp(name, "END")     >=MATCH_SUBSTR)	return DBL_MAX;
   return 10.0;
}

/*
 * Find/Create a new card in the list.
 */
static char*
get_card(HeaderUnitStruct* list, double idx, const char* name, double idxmatch)
{
   FitsCardPtr rtn;
   int i;
   double idx_auto = auto_idx(name);

   if (idx == DBL_MAX)
   {
      log_warning("END is added automatically by libfh");
      return 0;
   }

   /*
    * If idx_auto < 10, the library should assign the idx value itself
    * in such a way that the output will conform to the FITS standard.
    */
   if (idx_auto < 10.0) idx = idx_auto;
   else
      /*
       * Otherwise, if the user's idx < 10.0, auto-increment the last idx.
       */
      if (idx < 10.0) idx = (list->idx_highest += IDX_AUTO_INCR);
      else if (idx > list->idx_highest) list->idx_highest = idx;

   /*
    * Check if a card by the same name (or same name _and_ idxmatch,
    * if idxmatch is non-zero) already existed.
    */
   if (name && (rtn = get_hdr(list, name, idxmatch))) return rtn->card;

   /*
    * Allocate more memory (just double the size) if table is full.
    */
   if (++list->len > list->size)
   {
      FitsCardPtr* newhdr;
      if (!(newhdr = (FitsCardPtr*)realloc(list->hdr,
					   sizeof(FitsCardPtr) *
					   list->size * 2)))
      {
	 log_error("out of memory for FITS card slots");
	 list->err_memory++;
	 return 0; /* Save for delayed error from fh_rewrite() */
	 /* Old list->hdr is still valid if realloc fails. */
      }
      list->size *= 2;
      list->hdr = newhdr;
   }

   /*
    * Allocate a new FITS card.
    */
   if (!(rtn = list->hdr[list->len-1] = (FitsCard*)malloc(sizeof(FitsCard))))
   {
      log_error("out of memory for new FITS card");
      list->err_memory++; /* Save for delayed error from fh_rewrite() */
      return 0;
   }

   /*
    * Blank the entry and fill in the name.
    */
   rtn->idx = idx;
   memset(rtn->card, ' ', FH_CARD_SIZE);
   rtn->card[FH_CARD_SIZE] = '\0';

   /*
    * Check, and fill in the name if needed.
    */
   if (name)
   {
      for (i=0; i<FH_NAME_SIZE && *name; i++)
	 rtn->card[i] = NAME_CHR(*name++);

      /*
       * The '=' case is OK.  It happens since fh_set_card passes
       * the whole card in for name also.
       */
      if (*name!='\0' && *name!='=')
	 log_warning("long (>8 byte) keyword name truncated");
   }

   return rtn->card;
}


static void
add_comment(char* s, int col, const char* comment)
{
   /*
    * Comments must not begin before column 30.
    */
   while (col < 30) s[col++] = ' ';

   /*
    * If no comment given, leave the old comment in place,
    * if possible.  It could be that a longer string
    * has overwritten the / that starts the comment, in which
    * case this will make sure that the first non-space character
    * after the value is still a '/'.
    */
   if (!comment || !*comment)
   {
      if (col < FH_CARD_SIZE) s[col++] = ' ';
      while (col < FH_CARD_SIZE && s[col] == ' ') col++;
      if (col < FH_CARD_SIZE) s[col++] = '/';
      return;
   }

   if (col < 77 && comment && *comment)
   {
      s[col++] = ' ';
      s[col++] = '/';
      s[col++] = ' ';
      while (col < FH_CARD_SIZE && *comment)
      {
	 if (*comment < ' ' || *comment >= 127)
	 {
	    s[col++] = '_';
	    sprintf(err_buf, "illegal character %x in FITS card comment.",
		    *comment++); /* Don't forget to increment comment */
	    log_warning(err_buf);
	 }
	 else
	 {
	    s[col++] = *comment++;
	 }
      }
   }
   while (col < FH_CARD_SIZE) s[col++] = ' '; /* Blank out rest of old value, if any */
}

/* End of fh.c */
