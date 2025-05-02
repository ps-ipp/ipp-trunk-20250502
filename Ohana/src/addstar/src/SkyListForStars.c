# include "addstar.h"

void sort_stars_ra (Catalog *catalog) {

# define SWAPFUNC(A,B){ Average tmp; tmp = catalog->average[A]; catalog->average[A] = catalog->average[B]; catalog->average[B] = tmp; }
# define COMPARE(A,B)(catalog->average[A].R < catalog->average[B].R)

  OHANA_SORT (catalog->Naverage, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE

}

SkyList *SkyListForStars (SkyTable *table, int depth, Catalog *catalog) {
  
  int i, j, Nr, NR;
  SkyList *here;
  SkyList *list;
  
  Nr = 0;
  NR = 10;
  ALLOCATE (list, SkyList, 1);
  ALLOCATE (list[0].regions,  SkyRegion *, NR);
  ALLOCATE (list[0].filename, char *, NR);
  list[0].Nregions = Nr;
  list[0].ownElements = FALSE; // free these elements when freeing the list

  sort_stars_ra (catalog); /* sort by RA */

  ALLOCATE (catalog->found_t, off_t, catalog->Naverage);
  for (i = 0; i < catalog->Naverage; i++) {
    catalog->found_t[i] = 0;
  }

  for (i = 0; i < catalog->Naverage; i++) {
    if (catalog->found_t[i] == -2) continue;
    here = SkyRegionByPoint (table, depth, catalog->average[i].R, catalog->average[i].D);
    catalog->found_t[i] = -2;
    /* search forward for all contained stars */
    for (j = i; j < catalog->Naverage; j++) {
      if (catalog->average[j].R >= here[0].regions[0][0].Rmax) break;
      if (catalog->average[j].R <  here[0].regions[0][0].Rmin) break;
      if (catalog->average[j].D <  here[0].regions[0][0].Dmin) continue;
      if (catalog->average[j].D >= here[0].regions[0][0].Dmax) continue;
      catalog->found_t[j] = -2;
    }
    list[0].regions[Nr] = here[0].regions[0];
    list[0].filename[Nr] = here[0].filename[0];
    SkyListFree (here); 
    Nr ++;
    if (Nr >= NR) {
	NR += 32;
	REALLOCATE (list[0].regions, SkyRegion *, NR);
	REALLOCATE (list[0].filename, char *, NR);
    }
    list[0].Nregions = Nr;
  }

  /* reset to -1 for all stars: required start for find_match_refstars */
  free (catalog->found_t);
  catalog->found_t = NULL;
  return (list);
}

/* given a list of stars, find all region files which contain them 

   - sort by ra
   - loop over stars
     - find region file which contains star
     - go forwards until ra > Rmax
       - mark all stars in this region file
     
   - use DEC band information?
   - would like to minimize the number of disk reads

*/
