/* -*- c-file-style: "Ellemtel" -*-
 *
 * persist_fits.c - read and write persistence info to FITS tables.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "fh/fh.h"

#include "burntool.h"
#include "burnparams.h"
#include "persist_fits.h"

/* Default names for burn info table extensions. We'll use keyword information
 * when reading to determine the names of the extensions, but we'll always 
 * write them with these names. */
#define DEFAULT_EXTNAME_AREA_TABLE "burntool_areas"
#define DEFAULT_EXTNAME_FIT_TABLE  "burntool_fits"

/* Index of all the columns of the area table. */
typedef enum
{
   AREA_TABLE_COL_CELL,
   AREA_TABLE_COL_TIME,
   AREA_TABLE_COL_CX,
   AREA_TABLE_COL_CY,
   AREA_TABLE_COL_MAX,
   AREA_TABLE_COL_Y0,
   AREA_TABLE_COL_SX,
   AREA_TABLE_COL_SY,
   AREA_TABLE_COL_EX,
   AREA_TABLE_COL_EY,
   AREA_TABLE_COL_Y0M,
   AREA_TABLE_COL_Y0P,
   AREA_TABLE_COL_Y1M,
   AREA_TABLE_COL_Y1P,
   AREA_TABLE_COL_X0M,
   AREA_TABLE_COL_X0P,
   AREA_TABLE_COL_X1M,
   AREA_TABLE_COL_X1P,
   AREA_TABLE_COL_FUNC,
   AREA_TABLE_COL_UP,
   AREA_TABLE_COL_SLOPE,
   AREA_TABLE_COL_NFIT,
   AREA_TABLE_COL_SXFIT,
   AREA_TABLE_COL_EXFIT,
   AREA_TABLE_COL_FITERR,
   
   /* Add new columns above this line. */
   AREA_TABLE_NUM_COLS
} AREA_TABLE_COL_T;

/* All of the columns of the area table. */
static fhTableCol 
area_table_cols[] = 
{
/*   Name     Description                              Units      Format                  Width  Dec places */
   { "cell",  "Cell that correction area is in",       "",        FH_TABLE_FORMAT_INT,    2,     0 },
   { "time",  "PON time when area created",            "secs",    FH_TABLE_FORMAT_INT,    10,    0 },
   { "cx",    "Center x (position of max)",            "pixels",  FH_TABLE_FORMAT_INT,    3,     0 },
   { "cy",    "Center y (position of max)",            "pixels",  FH_TABLE_FORMAT_INT,    3,     0 },
   { "max",   "Max data value (above sky)",            "ADU",     FH_TABLE_FORMAT_INT,    5,     0 },
   { "y0",    "y origin for fit (stamp sy for stars)", "pixels",  FH_TABLE_FORMAT_INT,    3,     0 },
   { "sx",    "Left corner of area",                   "pixels",  FH_TABLE_FORMAT_INT,    3,     0 },
   { "sy",    "Bottom corner of area",                 "pixels",  FH_TABLE_FORMAT_INT,    3,     0 },
   { "ex",    "Right corner of area",                  "pixels",  FH_TABLE_FORMAT_INT,    3,     0 },
   { "ey",    "Top corner of area",                    "pixels",  FH_TABLE_FORMAT_INT,    3,     0 },
   { "y0m",   "Min y value at sx",                     "pixels",  FH_TABLE_FORMAT_INT,    5,     0 },
   { "y0p",   "Max y value at sx",                     "pixels",  FH_TABLE_FORMAT_INT,    5,     0 },
   { "y1m",   "Min y value at ex",                     "pixels",  FH_TABLE_FORMAT_INT,    5,     0 },
   { "y1p",   "Max y value at ex",                     "pixels",  FH_TABLE_FORMAT_INT,    5,     0 },
   { "x0m",   "Min x value at sy",                     "pixels",  FH_TABLE_FORMAT_INT,    5,     0 },
   { "x0p",   "Max x value at sy",                     "pixels",  FH_TABLE_FORMAT_INT,    5,     0 },
   { "x1m",   "Min x value at ey",                     "pixels",  FH_TABLE_FORMAT_INT,    5,     0 },
   { "x1p",   "Max x value at ey",                     "pixels",  FH_TABLE_FORMAT_INT,    5,     0 },
   { "func",  "What are we going to do about it?",     "",        FH_TABLE_FORMAT_INT,    3,     0 },
   { "up",    "Trails up or down",                     "1=up 0=down", FH_TABLE_FORMAT_INT,1,     0 },
   { "slope", "Slope of fit",                          "",        FH_TABLE_FORMAT_DOUBLE, 9,     6 },
   { "nfit",  "Number of columns corrected",           "",        FH_TABLE_FORMAT_INT,    3,     0 },
   { "sxfit", "Starting column for fit",              "pixels",  FH_TABLE_FORMAT_INT,    3,     0 },
   { "exfit", "Ending column for fit",                "pixels",  FH_TABLE_FORMAT_INT,    3,     0 },
   { "fiterr", "Error code of fit",                "",  FH_TABLE_FORMAT_INT,    3,     0 },
};

/* Index of all of the columns in the fit table. */
typedef enum
{
   FIT_TABLE_COL_CELL,
   FIT_TABLE_COL_CX,
   FIT_TABLE_COL_CY,
   FIT_TABLE_COL_XFIT,
   FIT_TABLE_COL_YFIT,
   FIT_TABLE_COL_ZERO,
   
   /* Add new columns above this line. */
   FIT_TABLE_NUM_COLS
} FIT_TABLE_COL_T;

/* All of the columns in the fit table. */
static fhTableCol
fit_table_cols[] = 
{
/*   Name     Description                              Units      Format                  Width  Dec places */
   { "cell",  "Cell that correction area is in",       "",        FH_TABLE_FORMAT_INT,    2,     0 },
   { "cx",    "Center x (position of max)",            "pixels",  FH_TABLE_FORMAT_INT,    3,     0 },
   { "cy",    "Center y (position of max)",            "pixels",  FH_TABLE_FORMAT_INT,    3,     0 },
   { "xfit",  "x of each value of the start of correction", "pixels", FH_TABLE_FORMAT_INT,3,     0 },
   { "yfit",  "x of each value of the start of correction", "pixels", FH_TABLE_FORMAT_INT,3,     0 },
   { "zero",  "Zero of this fit",                      "",        FH_TABLE_FORMAT_DOUBLE, 8,     4 },
};

/* Structure describing area table. Stuff like the number of
 * rows, the overall size, etc. are calculated at runtime. */
static fhTable
area_table = 
{
   .extname = DEFAULT_EXTNAME_AREA_TABLE,
   .num_cols = AREA_TABLE_NUM_COLS,
   .num_rows = 0, /* Don't know yet - need to rip through the data to figure this out. */
   .cols = area_table_cols,
};

/* This is the fit table, in all its glory. */
static fhTable
fit_table = 
{
   .extname = DEFAULT_EXTNAME_FIT_TABLE,
   .num_cols = FIT_TABLE_NUM_COLS,
   .num_rows = 0, /* Don't know yet - need to rip through the data to figure this out. */
   .cols = fit_table_cols,
};

/*
 * Finds existing fit and area tables in a FITS file.
 *
 * Given the primary header of a FITS file, this function
 * tries to find the area and fit table extensions. It does this
 * by first looking at the header to see if the extension names
 * of the tables are listed, and if so, looks for those by name.
 * If that information is not available for some reason, the default
 * names are used instead.
 *
 * Inputs:
 *    phu - primary header of a FITS file.
 *
 * Outputs:
 *      area_hu - header of area table stored here if found, NULL otherwise.
 *    area_name - if non-NULL, this is a buffer where the name of the 
 *                area table extension will be copied.
 *       fit_hu - header of fit table stored here if found, NULL otherwise.
 *     fit_name - if non-NULL, this is a buffer where the name of the 
 *                fit table extension will be copied.
 *
 * This function returns nothing.
 */
static void
find_existing_tables(HeaderUnit phu, 
                     HeaderUnit * area_hu, char * area_name, 
                     HeaderUnit * fit_hu, char * fit_name)
{
   char area_extname[FH_MAX_STRLEN+1];
   char fit_extname[FH_MAX_STRLEN+1];

   /* Does the FITS file provide the names of the burn tables?
    * If so, use those, otherwise revert to the defaults. */
   if(fh_get_str(phu, PHU_NAME_BURN_AREA, area_extname, sizeof(area_extname)) != FH_SUCCESS)
   {
      snprintf(area_extname, FH_MAX_STRLEN+1, DEFAULT_EXTNAME_AREA_TABLE);
   }
   if(fh_get_str(phu, PHU_NAME_BURN_FIT, fit_extname, sizeof(fit_extname)) != FH_SUCCESS)
   {
      snprintf(fit_extname, FH_MAX_STRLEN+1, DEFAULT_EXTNAME_FIT_TABLE);
   }

   *area_hu = fh_ehu_by_extname(phu, area_extname);
   if(area_name)
   {
      if(*area_hu) strcpy(area_name, area_extname);
      else area_extname[0] = '\0';
   }
   *fit_hu = fh_ehu_by_extname(phu, fit_extname);
   if(fit_name)
   {
      if(*fit_hu) strcpy(fit_name, fit_extname);
      else fit_name[0] = '\0';
   }
}

/*
 * Determines how many rows should be in the area and fit tables.
 *
 * Given all of the burn information for the whole OTA, this function
 * figures out how many rows are needed in the area and fit tables to
 * accomodate all of that info.
 *
 * Inputs:
 *    cell - array of 64 CELL structures, one for each cell in the OTA.
 *
 * Outputs:
 *    num_areas_found - the number of burn areas for the OTA stored here.
 *     num_fits_found - the number of fits across all of the burn
 *                      areas for the OTA stored here.
 *
 * This function returns FH_SUCCESS if the row was successfully 
 * written.
 */
static void
calculate_rows(CELL *cell, int * num_areas_found, int * num_fits_found)
{
   int j, k;
   int num_areas = 0;
   int num_fits = 0;

   for(j=0; j<MAXCELL; j++) 
   {
      /* First: patched up persists */
      for(k=0; k<cell[j].npersist; k++) 
      {
/* Retire old burns */
	 if(cell[j].time - cell[j].persist[k].time > EXPIRE_TRAIL_TIME)
	    continue;

	 if(PERSIST_RETAIN) {
/* Keep fits which have a dubious slope */
	    if(cell[j].persist[k].fiterr != FIT_SLOPE_ERROR) {
	       if(cell[j].persist[k].fiterr) continue;
	       if(cell[j].persist[k].nfit <= 0) continue;
	    }
	 } else {
	    if(cell[j].persist[k].fiterr) continue;
	    if(cell[j].persist[k].nfit <= 0) continue;
	 }
         num_areas++;
         num_fits += cell[j].persist[k].nfit;
      }

      /* Second: new burns */
      for(k=0; k<cell[j].nburn; k++) 
      {
	 if(!cell[j].burn[k].burned) continue;
	 if(PERSIST_RETAIN) {
/* Keep fits which have a dubious slope */
	    if(cell[j].burn[k].fiterr != FIT_SLOPE_ERROR) {
	       if(cell[j].burn[k].fiterr && 
		  cell[j].burn[k].fiterr != FIT_TOP_ERROR) continue;
	       if(cell[j].burn[k].nfit <= 0) continue;
	    }
	 } else {
	    if(cell[j].burn[k].fiterr && 
	       cell[j].burn[k].fiterr != FIT_TOP_ERROR) continue;
	    if(cell[j].burn[k].nfit <= 0) continue;
	 }
         num_areas++;
         num_fits += cell[j].burn[k].nfit;
      }
   }

   *num_areas_found = num_areas;
   *num_fits_found = num_fits;
}

/*
 * Writes one row of the area table.
 *
 * Given all of the data for a row of the area table, this function writes
 * the row in the table body.
 *
 * Inputs:
 *       hu - header of fit table extension.
 *     data - points to buffer for table body where data will be written.
 *    table - describes structure of the table.
 *      row - row of table to be written.
 *     area - all properties of the area.
 *
 * Outputs:
 *    None.
 *
 * This function returns FH_SUCCESS if the row was successfully 
 * written.
 */
static fh_result
write_area_row(HeaderUnit hu, void * data, fhTable * table, int row, OBJBOX * area)
{
   if((fh_table_write_value(table, data, row, AREA_TABLE_COL_CELL,  &(area->cell))  != FH_SUCCESS) ||
      (fh_table_write_value(table, data, row, AREA_TABLE_COL_TIME,  &(area->time))  != FH_SUCCESS) ||
      (fh_table_write_value(table, data, row, AREA_TABLE_COL_CX,    &(area->cx))    != FH_SUCCESS) ||
      (fh_table_write_value(table, data, row, AREA_TABLE_COL_CY ,   &(area->cy))    != FH_SUCCESS) ||
      (fh_table_write_value(table, data, row, AREA_TABLE_COL_MAX,   &(area->max))   != FH_SUCCESS) ||
      (fh_table_write_value(table, data, row, AREA_TABLE_COL_Y0,    &(area->y0))    != FH_SUCCESS) ||
      (fh_table_write_value(table, data, row, AREA_TABLE_COL_SX,    &(area->sx))    != FH_SUCCESS) ||
      (fh_table_write_value(table, data, row, AREA_TABLE_COL_SY,    &(area->sy))    != FH_SUCCESS) ||
      (fh_table_write_value(table, data, row, AREA_TABLE_COL_EX,    &(area->ex))    != FH_SUCCESS) ||
      (fh_table_write_value(table, data, row, AREA_TABLE_COL_EY,    &(area->ey))    != FH_SUCCESS) ||
      (fh_table_write_value(table, data, row, AREA_TABLE_COL_Y0M,   &(area->y0m))   != FH_SUCCESS) ||
      (fh_table_write_value(table, data, row, AREA_TABLE_COL_Y0P,   &(area->y0p))   != FH_SUCCESS) ||
      (fh_table_write_value(table, data, row, AREA_TABLE_COL_Y1M,   &(area->y1m))   != FH_SUCCESS) ||
      (fh_table_write_value(table, data, row, AREA_TABLE_COL_Y1P,   &(area->y1p))   != FH_SUCCESS) ||
      (fh_table_write_value(table, data, row, AREA_TABLE_COL_X0M,   &(area->x0m))   != FH_SUCCESS) ||
      (fh_table_write_value(table, data, row, AREA_TABLE_COL_X0P,   &(area->x0p))   != FH_SUCCESS) ||
      (fh_table_write_value(table, data, row, AREA_TABLE_COL_X1M,   &(area->x1m))   != FH_SUCCESS) ||
      (fh_table_write_value(table, data, row, AREA_TABLE_COL_X1P,   &(area->x1p))   != FH_SUCCESS) ||
      (fh_table_write_value(table, data, row, AREA_TABLE_COL_FUNC,  &(area->func))  != FH_SUCCESS) ||
      (fh_table_write_value(table, data, row, AREA_TABLE_COL_UP,    &(area->up))    != FH_SUCCESS) ||
      (fh_table_write_value(table, data, row, AREA_TABLE_COL_SLOPE, &(area->slope)) != FH_SUCCESS) ||
      (fh_table_write_value(table, data, row, AREA_TABLE_COL_NFIT,  &(area->nfit))  != FH_SUCCESS) ||
      (fh_table_write_value(table, data, row, AREA_TABLE_COL_SXFIT, &(area->sxfit)) != FH_SUCCESS) ||
      (fh_table_write_value(table, data, row, AREA_TABLE_COL_EXFIT, &(area->exfit)) != FH_SUCCESS) ||
      (fh_table_write_value(table, data, row, AREA_TABLE_COL_FITERR, &(area->fiterr)) != FH_SUCCESS))
   {
      fprintf(stderr, "\rerror: Error writing data to row %d of area table.\n", row);
      return FH_BAD_VALUE;
   }

   return FH_SUCCESS;
}

/*
 * Writes area data to a FITS file.
 *
 * This function iterates through all of the burn areas in all cells 
 * and populates the body of the area table in the provided buffer. 
 *
 * Inputs:
 *       hu - header of fit table extension.
 *     data - points to buffer for table body where data will be written.
 *    table - describes structure of the table.
 *     cell - array of 64 CELL structures for all cells of an OTA. Burn
 *            data from these structures will be written to the table.
 *
 * Outputs:
 *    None.
 *
 * This function returns FH_SUCCESS if the table was successfully 
 * written.
 */
static fh_result
write_area_table(HeaderUnit hu, void * data, fhTable * table, CELL * cell)
{
   int j, k;
   int row=0;
   fh_result result;

   for(j=0; j<MAXCELL; j++) 
   {
      /* First: patched up persists */
      for(k=0; k<cell[j].npersist; k++) 
      {
/* Retire old burns */
	 if(cell[j].time - cell[j].persist[k].time > EXPIRE_TRAIL_TIME)
	    continue;

	 if(PERSIST_RETAIN) {
/* Keep fits which have a dubious slope */
	    if(cell[j].persist[k].fiterr != FIT_SLOPE_ERROR) {
	       if(cell[j].persist[k].fiterr) continue;
	       if(cell[j].persist[k].nfit <= 0) continue;     
	    }
	 } else {
	    if(cell[j].persist[k].fiterr) continue;
	    if(cell[j].persist[k].nfit <= 0) continue;     
	 }

         result = write_area_row(hu, data, table, row++, &(cell[j].persist[k]));
         if(result != FH_SUCCESS) return result;
      }

      /* Second: new burns */
      for(k=0; k<cell[j].nburn; k++) 
      {
	 if(!cell[j].burn[k].burned) continue;
	 if(PERSIST_RETAIN) {
/* Keep fits which have a dubious slope */
	    if(cell[j].burn[k].fiterr != FIT_SLOPE_ERROR) {
	       if(cell[j].burn[k].fiterr && 
		  cell[j].burn[k].fiterr != FIT_TOP_ERROR) continue;
	       if(cell[j].burn[k].nfit <= 0) continue;
	    }
	 } else {
	    if(cell[j].burn[k].fiterr && 
	       cell[j].burn[k].fiterr != FIT_TOP_ERROR) continue;
	    if(cell[j].burn[k].nfit <= 0) continue;
	 }

         result = write_area_row(hu, data, table, row++, &(cell[j].burn[k]));
         if(result != FH_SUCCESS) return result;
      }
   }

   return FH_SUCCESS;
}


/*
 * Writes one row of the fit table.
 *
 * Given all of the data for a row of the fit table, this function writes
 * the row in the table body.
 *
 * Inputs:
 *       hu - header of fit table extension.
 *     data - points to buffer for table body where data will be written.
 *    table - describes structure of the table.
 *      row - row of table to be written.
 *     cell - cell number for fit.
 *       cx - center x of fit.
 *       cy - center y of fit.
 *     xfit - x of each value of correction start.
 *     yfit - y of each value of correction start.
 *     zero - zero of this fit.
 *
 * Outputs:
 *    None.
 *
 * This function returns FH_SUCCESS if the row was successfully 
 * written.
 */
static fh_result
write_fit_row(HeaderUnit hu, void * data, fhTable * table, int row, 
              int cell, int cx, int cy, int xfit, int yfit, double zero)
{
   if((fh_table_write_value(table, data, row, FIT_TABLE_COL_CELL, &cell) != FH_SUCCESS) ||
      (fh_table_write_value(table, data, row, FIT_TABLE_COL_CX,   &cx)   != FH_SUCCESS) ||
      (fh_table_write_value(table, data, row, FIT_TABLE_COL_CY,   &cy)   != FH_SUCCESS) ||
      (fh_table_write_value(table, data, row, FIT_TABLE_COL_XFIT, &xfit) != FH_SUCCESS) ||
      (fh_table_write_value(table, data, row, FIT_TABLE_COL_YFIT, &yfit) != FH_SUCCESS) ||
      (fh_table_write_value(table, data, row, FIT_TABLE_COL_ZERO, &zero) != FH_SUCCESS))
   {
      fprintf(stderr, "\rerror: Error writing data to row %d of fit table.\n", row);
      return FH_BAD_VALUE;
   }

   return FH_SUCCESS;
}

/*
 * Writes fit data to a FITS file.
 *
 * This function iterates through all of the burn areas in all cells 
 * and populates the body of the fit table in the provided buffer. 
 *
 * Inputs:
 *       hu - header of fit table extension.
 *     data - points to buffer for table body where data will be written.
 *    table - describes structure of the table.
 *     cell - array of 64 CELL structures for all cells of an OTA. Burn
 *            data from these structures will be written to the table.
 *
 * Outputs:
 *    None.
 *
 * This function returns FH_SUCCESS if the table was successfully 
 * written.
 */
static fh_result
write_fit_table(HeaderUnit hu, void * data, fhTable * table, CELL * cell)
{
   int i, j, k;
   int row=0;
   fh_result result;

   for(j=0; j<MAXCELL; j++) 
   {
      /* First: patched up persists */
      for(k=0; k<cell[j].npersist; k++) 
      {
/* Retire old burns */
	 if(cell[j].time - cell[j].persist[k].time > EXPIRE_TRAIL_TIME)
	    continue;

	 if(PERSIST_RETAIN) {
/* Keep fits which have a dubious slope */
	    if(cell[j].persist[k].fiterr != FIT_SLOPE_ERROR) {
	       if(cell[j].persist[k].fiterr) continue;
	    }
	 } else {
	    if(cell[j].persist[k].fiterr) continue;
	 }
	 for(i=0; i<cell[j].persist[k].nfit; i++) 
         {
            result = write_fit_row(hu, data, table, row++,
                                   cell[j].cell, cell[j].persist[k].cx, cell[j].persist[k].cy,
                                   cell[j].persist[k].xfit[i], cell[j].persist[k].yfit[i], 
                                   cell[j].persist[k].zero[i]);
            if(result != FH_SUCCESS) return result;
	 }
      }

      /* Second: new burns */
      for(k=0; k<cell[j].nburn; k++) 
      {
	 if(!cell[j].burn[k].burned) continue;
	 if(PERSIST_RETAIN) {
/* Keep fits which have a dubious slope */
	    if(cell[j].burn[k].fiterr != FIT_SLOPE_ERROR) {
	       if(cell[j].burn[k].fiterr && 
		  cell[j].burn[k].fiterr != FIT_TOP_ERROR) continue;
	    }
	 } else {
	    if(cell[j].burn[k].fiterr && 
	       cell[j].burn[k].fiterr != FIT_TOP_ERROR) continue;
	 }

	 for(i=0; i<cell[j].burn[k].nfit; i++) 
         {
            result = write_fit_row(hu, data, table, row++,
                                   cell[j].cell, cell[j].burn[k].cx, cell[j].burn[k].cy,
                                   cell[j].burn[k].xfit[i], cell[j].burn[k].yfit[i], 
                                   cell[j].burn[k].zero[i]);
            if(result != FH_SUCCESS) return result;
	 }
      }
   }

   return FH_SUCCESS;
}

/*
 * Reads one row of the area table from a FITS file.
 *
 * Given information about an area table and a row, this function
 * reads a single row of data from a buffer containing the table 
 * body. The data is stored in boxbuf[row].
 *
 * Inputs:
 *    table - describes structure of the table.
 *     data - points to data from table.
 *      row - the row to be read from the table.
 *
 * Outputs:
 *    None.
 *
 * This function returns FH_SUCCESS if the row was successfully 
 * read.
 */
static fh_result
read_area(fhTable * table, void * data, int row, int apply)
{
   int cell_num;

   /* Get the cell number first for some sanity checking. */
   if(fh_table_read_value(table, data, row, AREA_TABLE_COL_CELL, &cell_num) != FH_SUCCESS)
   {
      fprintf(stderr,
              "\rerror: Unable to get cell number from row %d of burn area table\n",
              row);
      return FH_BAD_VALUE;
   }

   /* Don't want to deal with gibberish. */
   if((cell_num < 0) || (cell_num > MAXCELL))
   {
      fprintf(stderr, "\rerror: illegal cell %d in area table row %d\n",
              cell_num, row);
      boxbuf[row].cell = -1;
      boxbuf[row].nfit = 0;
      return FH_BAD_VALUE;
   }

   boxbuf[row].cell = cell_num;

   if((fh_table_read_value(table, data, row, AREA_TABLE_COL_TIME,  &(boxbuf[row].time)) != FH_SUCCESS) ||
      (fh_table_read_value(table, data, row, AREA_TABLE_COL_CX,    &(boxbuf[row].cx)) != FH_SUCCESS) ||
      (fh_table_read_value(table, data, row, AREA_TABLE_COL_CY,    &(boxbuf[row].cy)) != FH_SUCCESS) ||
      (fh_table_read_value(table, data, row, AREA_TABLE_COL_MAX,   &(boxbuf[row].max)) != FH_SUCCESS) ||
      (fh_table_read_value(table, data, row, AREA_TABLE_COL_Y0,    &(boxbuf[row].y0)) != FH_SUCCESS) ||
      (fh_table_read_value(table, data, row, AREA_TABLE_COL_SX,    &(boxbuf[row].sx)) != FH_SUCCESS) ||
      (fh_table_read_value(table, data, row, AREA_TABLE_COL_SY,    &(boxbuf[row].sy)) != FH_SUCCESS) ||
      (fh_table_read_value(table, data, row, AREA_TABLE_COL_EX,    &(boxbuf[row].ex)) != FH_SUCCESS) ||
      (fh_table_read_value(table, data, row, AREA_TABLE_COL_EY,    &(boxbuf[row].ey)) != FH_SUCCESS) ||
      (fh_table_read_value(table, data, row, AREA_TABLE_COL_Y0M,   &(boxbuf[row].y0m)) != FH_SUCCESS) ||
      (fh_table_read_value(table, data, row, AREA_TABLE_COL_Y0P,   &(boxbuf[row].y0p)) != FH_SUCCESS) ||
      (fh_table_read_value(table, data, row, AREA_TABLE_COL_Y1M,   &(boxbuf[row].y1m)) != FH_SUCCESS) ||
      (fh_table_read_value(table, data, row, AREA_TABLE_COL_Y1P,   &(boxbuf[row].y1p)) != FH_SUCCESS) ||
      (fh_table_read_value(table, data, row, AREA_TABLE_COL_X0M,   &(boxbuf[row].x0m)) != FH_SUCCESS) ||
      (fh_table_read_value(table, data, row, AREA_TABLE_COL_X0P,   &(boxbuf[row].x0p)) != FH_SUCCESS) ||
      (fh_table_read_value(table, data, row, AREA_TABLE_COL_X1M,   &(boxbuf[row].x1m)) != FH_SUCCESS) ||
      (fh_table_read_value(table, data, row, AREA_TABLE_COL_X1P,   &(boxbuf[row].x1p)) != FH_SUCCESS) ||
      (fh_table_read_value(table, data, row, AREA_TABLE_COL_FUNC,  &(boxbuf[row].func)) != FH_SUCCESS) ||
      (fh_table_read_value(table, data, row, AREA_TABLE_COL_UP,    &(boxbuf[row].up)) != FH_SUCCESS) ||
      (fh_table_read_value(table, data, row, AREA_TABLE_COL_SLOPE, &(boxbuf[row].slope)) != FH_SUCCESS) ||
      (fh_table_read_value(table, data, row, AREA_TABLE_COL_NFIT,  &(boxbuf[row].nfit)) != FH_SUCCESS) ||
      (fh_table_read_value(table, data, row, AREA_TABLE_COL_SXFIT, &(boxbuf[row].sxfit)) != FH_SUCCESS) ||
      (fh_table_read_value(table, data, row, AREA_TABLE_COL_EXFIT, &(boxbuf[row].exfit)) != FH_SUCCESS) ||
      (fh_table_read_value(table, data, row, AREA_TABLE_COL_FITERR, &(boxbuf[row].fiterr)) != FH_SUCCESS))
   {
      fprintf(stderr,
              "\rerror: Error reading values from row %d of burn area table\n",
              row);
      boxbuf[row].nfit = 0;
      return FH_BAD_VALUE;
   }
   
   /* Make space for fits (to be read later). */
   if(boxbuf[row].nfit > 0)
   {
      /* This shouldn't happen, but be nice and don't leak. */
      if(boxbuf[row].zero) free(boxbuf[row].zero);
      if(boxbuf[row].xfit) free(boxbuf[row].xfit);
      if(boxbuf[row].yfit) free(boxbuf[row].yfit);

      boxbuf[row].zero = (double *)calloc(boxbuf[row].nfit, sizeof(double));
      boxbuf[row].xfit = (int *)calloc(boxbuf[row].nfit, sizeof(int));
      boxbuf[row].yfit = (int *)calloc(boxbuf[row].nfit, sizeof(int));

      if(boxbuf[row].zero == NULL ||
	 boxbuf[row].xfit == NULL ||
	 boxbuf[row].yfit == NULL) {
	 fprintf(stderr, "\rerror: failed to alloc boxbuf\n");
	 exit(-671);
      }
// 100203 JT: fiterr now saved and read, refit if not just an "apply"
      if(!apply) boxbuf[row].fiterr = 0;
   }

   return FH_SUCCESS;
}

/*
 * Reads the area table from a FITS file.
 *
 * This function fills out boxbuf[] with all of the areas in the
 * given table. 
 *
 * Inputs:
 *       hu - header of the table extension for the area table.
 *    table - describes structure of the table.
 *
 * Outputs:
 *    None.
 *
 * This function returns FH_SUCCESS if the tables were successfully 
 * appended to the FITS file.
 */
static fh_result
read_area_table(HeaderUnit hu, fhTable * table, int apply)
{
   fh_result result = FH_INVALID;
   int i;
   int row_chars, num_cols, num_rows;
   void * data;

   if((fh_get_int(hu, "NAXIS1",  &row_chars) != FH_SUCCESS) ||
      (fh_get_int(hu, "NAXIS2",  &num_rows) != FH_SUCCESS) ||
      (fh_get_int(hu, "TFIELDS", &num_cols) != FH_SUCCESS))
   {
      fprintf(stderr, "\rerror: Unable to find required keywords for area table dimensions\n");
      return FH_NOT_FOUND;
   }

   /* Sanity check. */
   if(num_cols != table->num_cols) 
   {
      fprintf(stderr, "\rerror: %d-column area table found, expected %d cols.\n",
              num_cols, table->num_cols);
      return FH_BAD_VALUE;
   }

   if(num_rows > MAXBURN)
   {
      fprintf(stderr, 
              "\rerror: too many boxes in area table. Max is %d, got %d\n",
              MAXBURN, num_rows);
      return FH_BAD_VALUE;
   }

   table->num_rows = num_rows;
   table->table_size = num_rows * row_chars;


   if(!(data = malloc(table->table_size)))
   {
      fprintf(stderr, 
              "\rerror: Unable to allocate %d bytes for area table.\n",
              table->table_size);
      return FH_NO_MEMORY;
   }
      
   if(fh_read_image(hu, fh_file_desc(hu), data, 
                    table->table_size, FH_TYPESIZE_8) != FH_SUCCESS)
   {
      fprintf(stderr, "\rerror: Unable to map area table body for reading.\n");
      free(data);
      return FH_INVALID;
   }

   for(i = 0; i < num_rows; i++)
   {
      if((result = read_area(table, data, i, apply)) != FH_SUCCESS) break;
   }

   free(data);
   return result;
}

/*
 * Reads one row of the fit table from a FITS file.
 *
 * Given information about a fit table and a row, this function
 * reads a single row of data from a buffer containing the table 
 * body.
 *
 * Inputs:
 *       hu - header of the table extension for the fit table.
 *    table - describes structure of the table.
 *     data - points to buffer of data from fit table.
 *      row - the row to be read.
 *
 * Outputs:
 *    cell - cell number stored here.
 *      cx - center x from table row stored here.
 *      cy - center y from table row stored here.
 *    xfit - x of each value of correction start stored here.
 *    yfit - y of each value of correction start stored here.
 *    zero - zero of this fit stored here.
 *
 * This function returns FH_SUCCESS if the table row was read 
 * successfully.
 *
 * Note: this function must not be called until after read_area_table()
 *       has read out the burn areas across all cells and prepared
 *       boxbuf[]. This function assumes that each area's fit
 *       information has been filled out in boxbuf[], including 
 *       allocating space for the fits. 
 */
static fh_result
read_fit(HeaderUnit hu, fhTable * table, void * data, int row, 
         int * cell, int * cx, int * cy, int * xfit, int * yfit, double * zero)
{
   if(fh_table_read_value(table, data, row, FIT_TABLE_COL_CELL, cell) != FH_SUCCESS)
   {
      fprintf(stderr,
              "\rerror: Unable to get cell number from row %d of fit table\n",
              row);
      return FH_BAD_VALUE;
   }

   if((*cell < 0) || (*cell > MAXCELL))
   {
      fprintf(stderr, "\rerror: illegal cell %d in fit table row %d\n",
              *cell, row);
      
      return FH_BAD_VALUE;
   }

   if((fh_table_read_value(table, data, row, FIT_TABLE_COL_CX,   cx) != FH_SUCCESS) ||
      (fh_table_read_value(table, data, row, FIT_TABLE_COL_CY,   cy) != FH_SUCCESS) ||
      (fh_table_read_value(table, data, row, FIT_TABLE_COL_XFIT, xfit) != FH_SUCCESS) ||
      (fh_table_read_value(table, data, row, FIT_TABLE_COL_YFIT, yfit) != FH_SUCCESS) ||
      (fh_table_read_value(table, data, row, FIT_TABLE_COL_ZERO, zero) != FH_SUCCESS))
   {
      fprintf(stderr,
              "\rerror: Error reading values from row %d of burn area table\n",
              row);
      return FH_BAD_VALUE;
   }

   return FH_SUCCESS;
}

/*
 * Reads the fit table from a FITS file.
 *
 * This function copies the data from a given fit table to boxbuf[].
 * Once complete, the contents of boxbuf[] should be equivalent to
 * if persist_read() had read from a text file.
 *
 * Inputs:
 *       hu - header of the table extension for the fit table.
 *    table - describes structure of the table.
 *    num_areas - the number of areas that were previously read from
 *                the area table and written to boxbuf.
 *
 * Outputs:
 *    None.
 *
 * This function returns FH_SUCCESS if the table was read 
 * successfully.
 *
 * Note: this function must not be called until after read_area_table()
 *       has read out the burn areas across all cells and prepared
 *       boxbuf[]. This function assumes that each area's fit
 *       information has been filled out in boxbuf[], including 
 *       allocating space for the fits. 
 */
static fh_result
read_fit_table(HeaderUnit hu, fhTable * table, int num_areas)
{
   fh_result result;
   int area, i;
   int row_chars, num_cols, num_rows;
   void * data;
   int fit_table_row;

   int cell, cx, cy, xfit, yfit;
   double zero;

   if((fh_get_int(hu, "NAXIS1",  &row_chars) != FH_SUCCESS) ||
      (fh_get_int(hu, "NAXIS2",  &num_rows) != FH_SUCCESS) ||
      (fh_get_int(hu, "TFIELDS", &num_cols) != FH_SUCCESS))
   {
      fprintf(stderr, "\rerror: Unable to find required keywords for fit table dimensions\n");
      return FH_NOT_FOUND;
   }

   /* Sanity check. */
   if(num_cols != table->num_cols) 
   {
      fprintf(stderr, "\rerror: %d-column fit table found, expected %d cols.\n",
              num_cols, table->num_cols);
      return FH_BAD_VALUE;
   }

   table->num_rows = num_rows;
   table->table_size = num_rows * row_chars;
   
   /* Make space for the body of the table and copy it.
    *
    * %%% It would be preferable to just memory map the body of the
    * table in the file itself, but I don't seem to be able to get 
    * that to work for reads. */
   if(!(data = malloc(table->table_size)))
   {
      fprintf(stderr, 
              "\rerror: Unable to allocate %d bytes for area table.\n",
              table->table_size);
      return FH_NO_MEMORY;
   }
   if(fh_read_image(hu, fh_file_desc(hu), data, table->table_size, FH_TYPESIZE_8) != FH_SUCCESS)
   {
      fprintf(stderr, "\rerror: Unable to read fit table body.\n");
      free(data);
      return FH_INVALID;
   }

   /* For all areas, grab all of the fits for that area. This should 
    * end up consuming the whole table. */
   fit_table_row = 0;
   for(area = 0; area < num_areas; area++)
   {
      for(i = 0; i < boxbuf[area].nfit; i++)
      {
         if((result = read_fit(hu, table, data, fit_table_row, 
                               &cell, &cx, &cy, &xfit, &yfit, &zero)) != FH_SUCCESS)
         {
            free(data);
            return result;
         }
         
         /* This shouldn't happen, but just in case... */
         if((cell != boxbuf[area].cell) || 
            (cx != boxbuf[area].cx) ||
            (cy != boxbuf[area].cy))
         {
            fprintf(stderr, 
                    "\rerror: Fit in table row %d does not match current area "
                    "(table cell=%d cx=%d cy=%d vs. area cell=%d cx=%d cy=%d)\n",
                    fit_table_row,
                    cell, cx, cy, boxbuf[area].cell, boxbuf[area].cx, boxbuf[area].cy);
            free(data);
            return FH_BAD_VALUE;
         }

         boxbuf[area].xfit[i] = xfit;
         boxbuf[area].yfit[i] = yfit;   
         boxbuf[area].zero[i] = zero;      

         /* Move on to the next row of the table. */
         fit_table_row++;
      }
   }

   free(data);
   return FH_SUCCESS;
}

/*
 * Retrieves burn information from a FITS file.
 *
 * This function reads the burn tables from an existing FITS file
 * and builds up burntool's internal table of areas and fits from
 * the information within. This function should be equivalent to
 * persist_read(), except that it operates on a FITS file instead
 * of text.
 *
 * Inputs:
 *        cell - array of 64 CELL structures that describe all of the
 *               burn info for the whole OTA. Burn info will be written
 *               here.
 *    filename - filename of FITS file to read from.
 *
 * Outputs:
 *    None.
 *
 * This function returns FH_SUCCESS if the tables were successfully 
 * appended to the FITS file.
 *
 * Note: this function assumes that the passed file is a MEF and that
 * any required sanity checks on the file have already been performed.
 */
fh_result
persist_fits_read(CELL *cell, const char * filename, int apply)
{
   HeaderUnit phu;
   HeaderUnit area_hu, fit_hu;
   fh_result result;

   if(!(phu = fh_create()))
   {
      fprintf(stderr, 
              "\rerror: unable to create header structure for FITS file \"%s\"\n",
              filename);
      return FH_NO_MEMORY;
   }

   /* Try to open the FITS file. */
   if ((result = fh_file(phu, filename, FH_FILE_RDONLY)) != FH_SUCCESS)
   {
      fprintf(stderr, "\rerror: unable to open FITS file \"%s\"\n",
              filename);
      fh_destroy(phu);
      return result;
   }

   
   /* Don't even try if this FITS file doesn't have any burn info in it. */
   find_existing_tables(phu, &area_hu, NULL, &fit_hu, NULL);
   if(!area_hu || !fit_hu) 
   {
      fprintf(stderr, "\rerror: Unable to find persistence info in FITS file.\n");
      fh_destroy(phu);
      return FH_NOT_FOUND;
   }

   fh_table_init(&area_table);
   fh_table_init(&fit_table);

   fh_ehu_by_extname(phu, DEFAULT_EXTNAME_AREA_TABLE);
   if((result = read_area_table(area_hu, &area_table, apply)) != FH_SUCCESS)
   {
      free(area_table.strbuf);
      free(fit_table.strbuf);
      fh_destroy(phu);
      fprintf(stderr, "\rerror: Unable to read area table from FITS file.\n");
      return result;
   }

   fh_ehu_by_extname(phu, DEFAULT_EXTNAME_FIT_TABLE);
   if((result = read_fit_table(fit_hu, &fit_table, area_table.num_rows)) != FH_SUCCESS)
   {
      free(area_table.strbuf);
      free(fit_table.strbuf);
      fh_destroy(phu);
      fprintf(stderr, "\rerror: Unable to read fit table from FITS file.\n");
      return result;
   }

   /* Now copy over the boxes that were read to our array of cells. 
      This section is ripped off from persist_read(). */
   {
      int i, k;
      int nbox;

/* Initialize the counts */
      for(k=0; k<MAXCELL; k++) {
         cell[k].npersist = 0;
         cell[k].persist = NULL;
      }

/* Augment counts */
      for(nbox = 0; nbox < area_table.num_rows; nbox++)
      {
         k = boxbuf[nbox].cell;
         cell[k].npersist += 1;
      }

      
/* Allocate some space for them */
      for(k=0; k<MAXCELL; k++) {
         if( (i=cell[k].npersist) > 0) {
            if( (cell[k].persist = (OBJBOX *)calloc(i, sizeof(OBJBOX))) == NULL) {
	       fprintf(stderr, "\rerror: failed to alloc cell persist buffer\n");
	       exit(-672);
	    }
            cell[k].npersist = 0;
         }
      }
/* Copy the results to the cells */
      for(i=0; i<nbox; i++) {
         k = boxbuf[i].cell;
         memcpy(cell[k].persist+cell[k].npersist, boxbuf+i, sizeof(OBJBOX));
         cell[k].npersist += 1;
      }
   }

   fh_destroy(phu);
   return FH_SUCCESS;
}

/*
 * Writes burn information to a FITS file.
 *
 * This function writes all of the the burn information passed as 
 * a pair of FITS table extensions - one table listing the various 
 * areas across all of the cells in the OTA, and another indicating
 * the various fits in those areas.
 *
 * Inputs:
 *    cell - array of 64 CELL structures that describe all of the
 *           burn info for the whole OTA.
 *     phu - primary header for existing FITS file to be written to.
 *
 * Outputs:
 *    None.
 *
 * This function returns FH_SUCCESS if the tables were successfully 
 * appended to the FITS file.
 *
 * Note: this function assumes that the passed file is a MEF and that
 * any required sanity checks on the file have already been performed.
 *
 * Note: this function assumes that the burn information has been
 * applied and sets the keyword defined by PHU_NAME_BURN_APPLIED
 * to TRUE. 
 */
fh_result
persist_fits_write(CELL *cell, HeaderUnit phu)
{
   int fd;
   HeaderUnit area_hu, fit_hu;
   void * data;
   int nextend;

   /* Don't even try if this FITS file already has burn info in it.
    * It's a fair bit of work to try to strip an extension out of
    * a FITS file. */
   find_existing_tables(phu, &area_hu, NULL, &fit_hu, NULL);
   if(area_hu || fit_hu) 
   {
      fprintf(stderr, 
              "\rerror: Unable to write correction info to FITS file. Correction FITS tables already exist.\n");
      return FH_NO_SPACE;
   }

   /* Is there somehow no file associated with the primary header? */
   if ((fd = fh_file_desc(phu)) == -1)
   {
      fprintf(stderr, "\rerror: header passed has no associated file.\n");
      return FH_INVALID;
   }

   /* We're going to append our tables to the end of the file, so go there now. */
   if(lseek(fd, 0, SEEK_END) == (off_t)-1)
   {
      fprintf(stderr,
              "\rerror: Unable to seek to end of file.\n");
      return FH_IN_ERRNO;
   }

   area_hu = fh_create();
   fit_hu = fh_create();
   if(!area_hu || !fit_hu) 
   {
      fprintf(stderr, 
              "\rerror: Unable to create headers for correction FITS tables.\n");
      if(area_hu) fh_destroy(area_hu);
      if(fit_hu) fh_destroy(fit_hu);
      return FH_NO_MEMORY;
   }

   /* Find out how many rows are going to be in each table. */
   calculate_rows(cell, &(area_table.num_rows), &(fit_table.num_rows));

   /* Start with the area table:
    * - Build up an extension header and write it to the file.
    * - Make space in the FITS file for the table body.
    * - Memory-map the area in the FITS file for writing.
    * - Write the data to the table.
    * - Unmap the file.
    */
   fh_link_ehu_to_phu(area_hu, phu);
   if((fh_table_populate_header(area_hu, &area_table) != FH_SUCCESS) ||
      (fh_write(area_hu, fd) != FH_SUCCESS) ||
      (fh_reserve_padded_table(area_hu, fd) != FH_SUCCESS) ||
      (fh_map_table(area_hu, &data, area_table.table_size) != FH_SUCCESS) ||
      (write_area_table(area_hu, data, &area_table, cell) != FH_SUCCESS) ||
      (fh_munmap_table(area_hu) != FH_SUCCESS))
   {
      fprintf(stderr, "\rerror: Error encountered writing area table to FITS file.\n");
      fh_destroy(fit_hu);
      free(area_table.strbuf);
      free(fit_table.strbuf);
      return FH_INVALID;
   }

   /* Now the fit table:
    * - Build up an extension header and write it to the file.
    * - Make space in the FITS file for the table body.
    * - Memory-map the area in the FITS file for writing.
    * - Write the data to the table.
    */
   fh_link_ehu_to_phu(fit_hu, phu);
   if((fh_table_populate_header(fit_hu, &fit_table) != FH_SUCCESS) ||
      (fh_write(fit_hu, fd) != FH_SUCCESS) ||
      (fh_reserve_padded_table(fit_hu, fd) != FH_SUCCESS) ||
      (fh_map_table(fit_hu, &data, fit_table.table_size) != FH_SUCCESS) ||
      (write_fit_table(fit_hu, data, &fit_table, cell) != FH_SUCCESS) ||
      (fh_munmap_table(fit_hu) != FH_SUCCESS))
   {
      fprintf(stderr, "\rerror: Error encountered writing area table to FITS file.\n");
      free(area_table.strbuf);
      free(fit_table.strbuf);
      return FH_INVALID;
   }

   /* Finally, correct the primary header to account for the 
    * new extensions. */
   fh_get_int(phu, "NEXTEND", &nextend);
   nextend += 2;
   fh_set_int(phu, FH_AUTO, "NEXTEND", nextend, "Number of extensions");

   /* Add info to the primary header indicating where to find the burn info. */
   fh_set_str(phu, FH_AUTO, PHU_NAME_BURN_AREA, DEFAULT_EXTNAME_AREA_TABLE, PHU_COMMENT_BURN_AREA);
   fh_set_str(phu, FH_AUTO, PHU_NAME_BURN_FIT, DEFAULT_EXTNAME_FIT_TABLE, PHU_COMMENT_BURN_FIT);
   fh_set_bool(phu, FH_AUTO, PHU_NAME_BURN_APPLIED, FH_TRUE, PHU_COMMENT_BURN_APPLIED);  

   fh_rewrite(phu);

   /* Get shot of the temporary space associated with the table structures. */
   free(area_table.strbuf);
   free(fit_table.strbuf);

   return FH_SUCCESS;
}

/*
 * Removes the burn tables from a FITS file.
 *
 * This function strips out the burn tables (if present) from a FITS
 * file, writing a new file with a couple less extensions in it.
 *
 * Inputs:
 *     phu_in - primary header for existing FITS file to be copied.
 *    fileout - filename of new FITS file.
 *
 * Outputs:
 *    None.
 *
 * This function returns FH_SUCCESS if the new file was successfully
 * written.
 */
fh_result
persist_fits_remove_tables(HeaderUnit phu_in, const char * fileout)
{
   HeaderUnit phu_out;

   HeaderUnit ehu_area;
   HeaderUnit ehu_fit;

   int num_extensions;
   int num_extensions_copied = 0;

   int fd_out;
   int i;
   fh_bool burn_applied = FH_FALSE;

   char area_extname[FH_MAX_STRLEN+1];
   char fit_extname[FH_MAX_STRLEN+1];

   if(fh_get_int(phu_in, "NEXTEND", &num_extensions) != FH_SUCCESS)
   {
      fprintf(stderr, "\rerror: Unable to get NEXTEND from input primary FITS header.\n");
      return FH_INVALID;
   }

   find_existing_tables(phu_in, &ehu_area, area_extname, &ehu_fit, fit_extname);
   if(!ehu_area || !ehu_fit) 
   {
      fprintf(stderr, 
              "\rerror: Unable to find persistence info in FITS file.\n");
      return FH_INVALID;
   }

   /* Print a warning if the input FITS file appears to have burn 
    * areas applied and that the output fiel is about to have that
    * info removed. */
   fh_get_bool(phu_in, PHU_NAME_BURN_APPLIED, &burn_applied);
   if(burn_applied == FH_TRUE)
   {
      fprintf(stderr, 
              "warning: input FITS file has burns applied to it. "
              "%s still has those burns applied and has no built-in "
              "knowledge of how to undo them.\n", fileout);
   }

   if((fd_out = open(fileout, O_CREAT | O_RDWR, 0644)) < 0)
   {
      fprintf(stderr, "\rerror: Failed to open \"%s\" for output.\n",
              fileout);
      exit(EXIT_FAILURE);
   }

   /* Copy the primary header. Note that fh_merge() doesn't
    * preserve any reserved space in the header (in our case, a
    * big block of identical COMMENT cards). For now, make sure
    * there's still some free space in the header by reserving 
    * an entire block. The downside to this is obviously that the
    * header will look a little different than the one we're copying,
    * but I can't figure any way around this right now. */
   if(!(phu_out = fh_create()) ||
      (fh_merge(phu_out, phu_in) != FH_SUCCESS) ||
      (fh_reserve(phu_out, (2880/80)) != FH_SUCCESS) ||
      (fh_write(phu_out, fd_out) != FH_SUCCESS))
   {
      fprintf(stderr, "\rerror: Unable to copy primary header to \"%s\"\n",
              fileout);
      fh_destroy(phu_out);
      close(fd_out);
      return FH_INVALID;
   }

   /* Walk through all of the extensions in the input FITS. Copy everything
    * but the tables of burn info. */
   for (i = 1; i <= num_extensions; i++)
   {
      HeaderUnit ehu_in;
      HeaderUnit ehu_out;
      char extname[FH_MAX_STRLEN+1];

      if(!(ehu_in = fh_ehu(phu_in, i)))
      {
         fprintf(stderr, "\rerror: Unable to read extension %d from input FITS file\n", i);
         fh_destroy(phu_out);
         close(fd_out);
         return FH_INVALID;
      }
      
      if (fh_get_str(ehu_in, "EXTNAME", extname, sizeof(extname)) != FH_SUCCESS)
      {
         fprintf(stderr, "\rerror: Unable to get EXTNAME from extension %d in input FITS file.\n", i); 
         fh_destroy(phu_out);
         close(fd_out);
         return FH_INVALID;
      }

      /* No match? Copy this extension in its entirety. */
      if(strcasecmp(extname, area_extname) && strcasecmp(extname, fit_extname))
      {
         ehu_out = fh_create();

         /* Copy the extension's header. The same thing about
          * reserved space with fh_merge() applies here. */
         if((fh_merge(ehu_out, ehu_in) != FH_SUCCESS) ||
            (fh_reserve(ehu_out, (2880/80)) != FH_SUCCESS))
         {
            fprintf(stderr, "\rerror: Unable to copy extension %d to %s.\n", i, fileout); 
            fh_destroy(phu_out);
            close(fd_out);
            return FH_INVALID;           
         }
         
         /* Apparently this can never fail. It would be handy if it just 
          * returned FH_SUCCESS so it could be included in a big block of
          * checks. */
         fh_link_ehu_to_phu(ehu_out, phu_out);

         /* WRite the copy of our header and all of the data that follows. */
         if((fh_write(ehu_out, fd_out) != FH_SUCCESS) ||
            (fh_copy_padded_image(ehu_out, fd_out, fh_file_desc(ehu_in)) != FH_SUCCESS))
         {
            fprintf(stderr, "\rerror: Unable to copy extension %d to %s.\n", i, fileout); 
            fh_destroy(phu_out);
            close(fd_out);
            return FH_INVALID;           
         }

         num_extensions_copied++;
      }
   }

   /* Tweak the primary header on the new file to reflect the fact 
    * that we just removed some extensions. */
   fh_set_int(phu_out, FH_AUTO, "NEXTEND", 
                 num_extensions_copied, "Number of extensions");
   fh_remove(phu_out, PHU_NAME_BURN_AREA);
   fh_remove(phu_out, PHU_NAME_BURN_FIT);
   fh_rewrite(phu_out);

   fh_destroy(phu_in);
   fh_destroy(phu_out);
   close(fd_out);

   return FH_SUCCESS;
}
