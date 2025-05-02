/* -*- c-file-style: "Ellemtel" -*-
 *
 * fh_table - manage ASCII FITS tables as part of libfh.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fh.h"

/* The FITS standard explicitly states the maximum number of 
 * columns in a table. */
#define MAX_COLS 999

#define FH_RESERVE \
"COMMENT  Reserved space.  This line can be used to add a new FITS card.         "
/*
 * Always place the END on the second line of a new 2880 block, so that
 * there are enough blank lines after the END for expansion, but we
 * don't require or put any blank lines before because some FITS
 * implementations (libfh is one of them, and cfitsio too) will
 * remove them.  Use a COMMENT instead, repeated until one occupies
 * the first line of a new block.  Then the END will go after that.
 * Anything which compresses and decompresses does not have to worry
 * about keeping track of blank lines, since they are after the end.
 * It will be forced to put back just as many as there were to begin
 * with in order to pad the header again.
 */
static void
pad_header(HeaderUnit hu)
{
   int blocks_orig = fh_header_blocks(hu);
   double idx = 900000000;

   do
   {
      fh_set_card(hu, idx++, FH_RESERVE);
   } while (fh_header_blocks(hu) == blocks_orig);
   fh_set_card(hu, idx++, FH_RESERVE);
}

/*
 * Sets up table info.
 *
 * Inputs:
 *    table - FITS table.
 */
fh_result
fh_table_init(fhTable * table)
{
   int i;
   int row_width = 0;

   /* Already set up? */
   if(table->strbuf) return FH_SUCCESS;

   if((table->num_cols <= 0) || (table->num_cols > MAX_COLS))
   {
      fprintf(stderr, 
              "error: %d-column table is invalid. Tables may only have 1..%d columns.\n",
              table->num_cols, MAX_COLS);
      return FH_INVALID;
   }

   table->widest_col = 0;
   for(i = 0 ; i < table->num_cols; i++)
   {
      /* Widest column in table? */
      if(table->cols[i].width > table->widest_col)
      {
         table->widest_col = table->cols[i].width;
      }

      /* Character positioning in a row in FITS starts from 1, not 0. */
      table->cols[i].first_char_pos = row_width + 1;
      row_width += table->cols[i].width;

      /* Generate a format string that will be used later to 
       * sprintf() values to the table. */
      switch(table->cols[i].format)
      {
         case FH_TABLE_FORMAT_CHAR:
            snprintf(table->cols[i].format_string, FH_TABLE_FORMAT_STR_LEN, 
                     "%%%ds", table->cols[i].width);
            break;
            
         case FH_TABLE_FORMAT_INT:
            snprintf(table->cols[i].format_string, FH_TABLE_FORMAT_STR_LEN, 
                     "%%%dd", table->cols[i].width);
            break;
            
         case FH_TABLE_FORMAT_FLOAT:
         case FH_TABLE_FORMAT_DOUBLE: /* Intentional fall-through. */
            snprintf(table->cols[i].format_string, FH_TABLE_FORMAT_STR_LEN, 
                     "%%%d.%df", table->cols[i].width, table->cols[i].dec_places);
            break;
            
         default:
            free(table);
            fprintf(stderr, "error: column %d has unknown format specifier '%c'\n",
                    i, table->cols[i].format);
            return FH_BAD_VALUE;
            break;
      }
   }

   table->row_width = row_width;
   table->table_size = row_width * table->num_rows;

   /* Make a space guaranteed to be large enough to hold any
    * of the fields, plus a null character on the end. This is 
    * done so that we can use the string library to manipulate
    * the contents of each field. */
   if(!(table->strbuf = malloc(table->widest_col + 1))) 
   {
      fprintf(stderr, 
              "error: Unable to malloc %d bytes for table field buffer.\n",
              table->widest_col + 1);
      return FH_NO_MEMORY;
   }

   return FH_SUCCESS;
}

/*
 * Writes essential FITS header information.
 *
 * Given a table and a FITS header, this funciton will write the necessary 
 * keywords to make the FITS header describe an ASCII table.
 *
 * Inputs:
 *       table - FITS table.
 *          hu - the FITS header to be written.
 *
 * Outputs:
 *    none.
 *
 * This function returns a PASS/FAIL indication that the header was 
 * successfully populated.
 *
 * NOTE: this function currently assumes that the FITS table is being
 * written to an extension header, not a primary. 
 *
 * NOTE: this function does not set keywords for the extension name
 * or other keywords that are not explicitly required for an ASCII 
 * table extension. It is the caller's responsibility to fill out
 * these keywords as required.
 */
fh_result fh_table_populate_header(HeaderUnit hu, fhTable * table)
{
   char keyword[FH_NAME_SIZE + 1];
   char value[FH_MAX_STRLEN + 1];
   int i;
   double idx;

   /* Do some sanity checks first. */
   if(!table || !table->cols || !table->num_cols || !table->num_rows)
   {
      fprintf(stderr, 
              "error: Incomplete table info passed to fh_table_populate_header()\n");
      return FH_INVALID;
   }

   if(fh_table_init(table) != FH_SUCCESS) return FH_BAD_VALUE;

   fh_set_str(hu, 0.0, "XTENSION", "TABLE", "");
   fh_set_int(hu, 1.0, "BITPIX", 8, "8-bit ASCII");
   fh_set_int(hu, 2.0, "NAXIS", 2, "2D table of values");
   fh_set_int(hu, 2.1, "NAXIS1", table->row_width, 
              "Characters in each table row");
   fh_set_int(hu, 2.2, "NAXIS2", table->num_rows, 
              "Number of rows in table");
   fh_set_int(hu, 3.0, "PCOUNT", 0, 
              "Random parameters before each array in a group");
   fh_set_int(hu, 3.2, "GCOUNT", 1, 
              "Number of random groups");
   fh_set_int(hu, 4.0, "TFIELDS", table->num_cols, 
              "Number of columns in table");

   /* Write the format specifiers for each column. */
   for (i=0; i < table->num_cols; i++)
   {
      int n;

      idx = 5.0 + i/10.;
      n = i + 1; /* FITS numbering scheme starts from 1. */
      snprintf(keyword, FH_NAME_SIZE + 1, "TFORM%d", n);
      if((table->cols[i].format == FH_TABLE_FORMAT_FLOAT) ||
         (table->cols[i].format == FH_TABLE_FORMAT_DOUBLE))
      {
         snprintf(value, FH_MAX_STRLEN + 1, "%c%d.%d", 
                  table->cols[i].format, table->cols[i].width,
                  table->cols[i].dec_places);
      }
      else
      {
         snprintf(value, FH_MAX_STRLEN + 1, "%c%d", 
                  table->cols[i].format, table->cols[i].width);
      }
      fh_set_str(hu, idx, keyword, value, 
                 "FITS table field format");
      idx += 0.01;

      snprintf(keyword, FH_NAME_SIZE + 1, "TBCOL%d", n);
      fh_set_int(hu, idx, keyword, table->cols[i].first_char_pos, 
                 "First character of column");
   }

   /* Set the name of this extension. */
   fh_set_str(hu, 6.0, "EXTNAME", table->extname, "Extension name");

   /* The FITS standard suggests that other descriptive information 
    * that describes each column must follow after all of the format
    * information. Presumably this is because this information, while
    * nice to have from a self-documenting perspective, isn't strictly
    * necessary to interpret the actual table values. */
   for (i=0; i < table->num_cols; i++)
   {
      int n;

      idx = 7.0 + i/10.;
      n = i + 1; /* FITS numbering scheme starts from 1. */
      snprintf(keyword, FH_NAME_SIZE + 1, "TTYPE%d", n);
      fh_set_str(hu, idx, keyword, table->cols[i].name, 
                 "Column name");
      
      snprintf(keyword, FH_NAME_SIZE + 1, "TUNIT%d", n);
      idx += 0.01;
      fh_set_str(hu, idx, keyword, table->cols[i].units, 
                 "Column data units");
      
      snprintf(keyword, FH_NAME_SIZE + 1, "TBCOM%d", n);
      idx += 0.01;
      fh_set_str(hu, idx, keyword, table->cols[i].comment, 
                 "Column description");

   }
   pad_header(hu);
   return FH_SUCCESS;
}

/*
 * Writes a field in a table.
 *
 * This function is used to write a single field to a table. Given
 * a table, information about its columns, the row and column within 
 * the table to be written and a value, the value will be written to 
 * the table according to the formatting instructions for that column.
 *
 * The value is passed as a void* so that this one function can be used
 * for any type of data that can be put into the table. The function
 * uses the column information to decide how to handle the pointer. It is
 * the caller's responsibility to keep their casts and pointers straight.
 *
 * Inputs:
 *    table - FITS table.
 *     data - buffer where FITS table is to be written.
 *      row - the row in the table where the datum should be written.
 *      col - the column in the table where the datum should be written.
 *    value - points to the value to be written.
 *
 * Outputs:
 *    none.
 *
 * This function returns a FH_SUCCESS if the value was successfully written.
 */
fh_result
fh_table_write_value(fhTable * table, void * data, 
                     int row, int col, void * value)
{
   char * ptr;

   /* Do some sanity checks first. */
   if(!table)
   {
      fprintf(stderr, 
              "error: no FITS table passed to fh_table_write_value().\n");
      return FH_INVALID;
   }
   if(!table->strbuf)
   {
      fprintf(stderr, 
              "error: no field buffer in this table. Was fh_table_populate_header() called already?\n");
      return FH_INVALID;
   }
   if((row >= table->num_rows) || (col >= table->num_cols))
   {
      fprintf(stderr, 
              "error: attempted to write value beyond edge of FITS table (row %d col %d, table is %dx%d).\n",
              row, col, table->num_rows, table->num_cols);
      return FH_INVALID;
   }
   if(!value)
   {
      fprintf(stderr, "error: no value given for row %d, col %d of FITS table.\n", row, col);
      return FH_INVALID;            
   }
   
   /* Figure out where this data is going to go. */
   ptr = (char *)data +
      (row * table->row_width) +
      table->cols[col].first_char_pos - 1;

   switch(table->cols[col].format)
   {
      case FH_TABLE_FORMAT_CHAR:
         snprintf(table->strbuf, table->cols[col].width + 1, 
                  table->cols[col].format_string, (char *)value);
         break;
         
      case FH_TABLE_FORMAT_INT:
         snprintf(table->strbuf, table->cols[col].width + 1,
                  table->cols[col].format_string, *((int *)value));
         break;
         
      case FH_TABLE_FORMAT_FLOAT:
         snprintf(table->strbuf, table->cols[col].width + 1, 
                  table->cols[col].format_string, *((float *)value));
         break;
      case FH_TABLE_FORMAT_DOUBLE:
         snprintf(table->strbuf, table->cols[col].width + 1, 
                  table->cols[col].format_string, *((double *)value));
         break;
         
      default:
         fprintf(stderr, 
                 "error: column %d has unknown format specifier '%c'\n",
                 col, table->cols[col].format);
         return FH_BAD_VALUE;
         break;
   }

   /* Copy the field over to the table, minus the null character. */
   memcpy(ptr, table->strbuf, table->cols[col].width);

   return FH_SUCCESS;
}

/*
 * Reads a field in a table.
 *
 * This function is used to retrieve a single field in a table. Given
 * a table, information about its columns, the row and column within 
 * the table to be written and a value, the value will be written to 
 * the table according to the formatting instructions for that column.
 *
 * The value is passed as a void* so that this one function can be used
 * for any type of data that can be put into the table. The function
 * uses the column information to decide how to handle the pointer. It is
 * the caller's responsibility to ensure that there is enough space on
 * the end of the value pointer to store the value, and that the value,
 * once written, will be interpreted properly (e.g. don't go passing
 * a pointer to a double for an int field).
 *
 * Inputs:
 *    table - FITS table.
 *     data - buffer where FITS table is to be written.
 *      row - the row in the table where the datum should be written.
 *      col - the column in the table where the datum should be written.
 *
 * Outputs:
 *    value - value in field will be written here.
 *
 * This function returns a FH_SUCCESS if the value was successfull read.
 */
fh_result
fh_table_read_value(fhTable * table, void * data, 
                    int row, int col, void * value)
{
   char * ptr;
   char * endchar;

   /* Do some sanity checks first. */
   if(!table)
   {
      fprintf(stderr, 
              "error: no FITS table passed to fh_table_read_value().\n");
      return FH_INVALID;
   }
   if(!table->strbuf)
   {
      fprintf(stderr, 
              "error: no field buffer in this table. Was fh_table_populate_header() called already?\n");
      return FH_INVALID;
   }
   if((row >= table->num_rows) || (col >= table->num_cols))
   {
      fprintf(stderr, 
              "error: attempted to read value beyond edge of FITS table (row %d col %d, table is %dx%d).\n",
              row, col, table->num_rows, table->num_cols);
      return FH_INVALID;
   }
   if(!value)
   {
      fprintf(stderr, "error: no value given for row %d, col %d of FITS table.\n", row, col);
      return FH_INVALID;            
   }
   
   /* Figure out where this value is going to come from. */
   ptr = (char *)data +
      (row * table->row_width) +
      table->cols[col].first_char_pos - 1;
   
   /* Grab the value, and shove a null on the end so the string
    * library can have its way with it. */
   memcpy(table->strbuf, ptr, table->cols[col].width);
   table->strbuf[table->cols[col].width] = '\0';

   /* Interpret the data based on the column's reported type. */
   switch(table->cols[col].format)
   {
      case FH_TABLE_FORMAT_CHAR:
         strcpy((char *)value, table->strbuf);
         break;
         
      case FH_TABLE_FORMAT_INT:
         *((int *)value) = (int)strtol(table->strbuf, &endchar, 10);
         if(*endchar != '\0')
         {
            fprintf(stderr,
                    "error: Invalid value in table at row %d, col %d. "
                    "Expected an int, got '%s'\n",
                    row, col, table->strbuf);
         }
         break;
         
      case FH_TABLE_FORMAT_FLOAT:
         *((float *)value) = strtod(table->strbuf, &endchar);
         if(*endchar != '\0')
         {
            fprintf(stderr,
                    "error: Invalid value in table at row %d, col %d. "
                    "Expected a float, got '%s'\n",
                    row, col, table->strbuf);
         }
         break;
      case FH_TABLE_FORMAT_DOUBLE:
         *((double *)value) = strtod(table->strbuf, &endchar);
         if(*endchar != '\0')
         {
            fprintf(stderr,
                    "error: Invalid value in table at row %d, col %d. "
                    "Expected a double, got '%s'\n",
                    row, col, table->strbuf);
         }
         break;
         
      default:
         fprintf(stderr, 
                 "error: column %d has unknown format specifier '%c'\n",
                 col, table->cols[col].format);
         return FH_BAD_VALUE;
         break;
   }

   return FH_SUCCESS;
}


/* Not sure if we need to do anything more with these right now. At 
 * the moment, these are purely cosmetic wrappers around their 
 * respective image functions. */
fh_result
fh_reserve_padded_table(HeaderUnit hu, int fd)
{
   return fh_reserve_padded_image(hu, fd);
}

fh_result
fh_map_table(HeaderUnit hu, void** data, int size)
{
   return fh_map_raw_image(hu, data, size);
}

fh_result
fh_munmap_table(HeaderUnit hu)
{
   return fh_munmap_raw_image(hu);
}
