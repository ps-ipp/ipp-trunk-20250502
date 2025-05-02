# include "gcompare.h"

void output (data1, data2, matches, Nmatches, match, deltas, nomatch1, nomatch2)
data_type  data1;
data_type  data2;
match_type matches[];
int        Nmatches;
int        match;
int        deltas; 
int        nomatch1;
int        nomatch2;
{

  int i, Nmatch1, Nmatch2;

  fprintf (stderr, "This is a Gcompare output list\n");
  fprintf (stderr, "There are %d matches\n\n", Nmatches);
  Nmatch1 = Nmatch2 = 0.0;

  if (match || deltas) {
    for (i = 0; i < Nmatches; i++) {
      if (deltas)
	fprintf (stdout, "%15.9f  %15.9f  ", matches[i].dX, matches[i].dY);
      if (match)
	fprintf (stdout, "%s  %s", matches[i].line1, matches[i].line2);
      fprintf (stdout, "\n");
    }
  }

  if (match && nomatch1) 
    fprintf (stdout, "\n non-matces from file 1:\n");
  
  if (nomatch1) {
    for (i = 0; i < data1.Nvalues; i++) {
      if (!data1.values[i].match) {
	fprintf (stdout, "%s\n", data1.values[i].line);
	Nmatch1 ++;
      }
    }
    fprintf (stderr, "no matches in file 1: %d\n", Nmatch1);
  }

  if ((match || nomatch1) && nomatch2)
    fprintf (stdout, "\n non-matces from file 2:\n");

  if (nomatch2) {
    for (i = 0; i < data2.Nvalues; i++) {
      if (!data2.values[i].match) {
	fprintf (stdout, "%s\n", data2.values[i].line);
	Nmatch2++;
      }
    }
    fprintf (stderr, "no matches in file 2: %d\n", Nmatch2);
  }


}
