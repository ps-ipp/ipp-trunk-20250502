# include "gcompare.h"

table_type *find_matches (table, values, Nvalues, catalog, Ncatalog, noauto)
table_type   table[];
value_type   values[];
int          Nvalues;
catalog_type catalog[];
int          Ncatalog;
int          noauto;
{

  int i, j, start, done;
  double delta_Dec, dRA, dDec, radius;

  delta_Dec = (catalog[Ncatalog - 1].Dec - catalog[0].Dec);
  for (i = 0; i < Nvalues; i++) {
    if ((values[i].Dec > catalog[Ncatalog - 1].Dec) || (values[i].Dec < catalog[0].Dec))
      continue;
    start = Ncatalog * (values[i].Dec - catalog[0].Dec) / delta_Dec;
    done = FALSE;
    while (!done) {
      if ((catalog[start].Dec > values[i].Dec - values[i].radius) && (start != 0)) {
	start -= 100;
	if (start < 0)
	  start = 0;
      }
      else
	done = TRUE;
    }
    for (j = start; ((j < Ncatalog) && 
		     (catalog[j].Dec < values[i].Dec + values[i].radius)); j++) {
      dRA = (values[i].RA - catalog[j].RA) / cos (DEG_RAD*values[i].Dec);
      dDec = values[i].Dec - catalog[j].Dec;
      
      radius = hypot (dRA, dDec);
      if ((radius <= values[i].radius) && (!noauto || (radius > 0))) {
	ALLOCATE (table[i].match[table[i].Nmatches], char, NBYTES_LINE + 1);
	bzero (table[i].match[table[i].Nmatches], NBYTES_LINE + 1);
	strcpy (table[i].match[table[i].Nmatches], catalog[j].line);
	table[i].Nmatches ++;
	if (table[i].Nmatches > 500)
	  fprintf (stderr, "too many objects! \n");
      }
    }
  }

  return (table);
}
