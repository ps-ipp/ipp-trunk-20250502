# include "imregister.h"
# include "detrend.h"

Criteria *ExpandRecipe (Criteria *base, int *Ncrit) {

  int i, N, Ns, NS;
  Criteria *crit;
  int Nbase;
  
  Nbase = *Ncrit;
  NS = Ns = 0;
  ALLOCATE (crit, Criteria, 1);

  for (N = 0; N < Nbase; N++) {

    RecipeType = LoadRecipe (filterhash[base[N].Filter], &Nrecipe);

    NS += Nrecipe;
    REALLOCATE (crit, Criteria, NS);
    
    for (i = 0; i < Nrecipe; i++) {
      crit[Ns] = base[N];
      crit[Ns].TypeSelect = TRUE;
      crit[Ns].Type = get_image_type (RecipeType[i]);
      if (crit[Ns].Type == T_UNDEF) { 
	fprintf (stderr, "ERROR: invalid image type %s\n", RecipeType[i]);
	exit (1);
      }
      if (!strcasecmp (RecipeType[i], "bias") ||
	  !strcasecmp (RecipeType[i], "dark") ||
	  !strcasecmp (RecipeType[i], "mask")) {
	crit[Ns].FilterSelect = FALSE;
      }
      if (!strcasecmp (RecipeType[i], "flat") ||
	  !strcasecmp (RecipeType[i], "scatter") ||
	  !strcasecmp (RecipeType[i], "fringe") ||
	  !strcasecmp (RecipeType[i], "frpts") ||
	  !strcasecmp (RecipeType[i], "modes")) {
	crit[Ns].FilterSelect = TRUE;
      }
      if (!strcasecmp (RecipeType[i], "modes")) {
	crit[Ns].CCDSelect = FALSE;
      }
      /*      
      if (!strcasecmp (RecipeType[i], "dark")) {
      crit[Ns].ExptimeSelect = TRUE;
      } 
      */
      Ns ++;
    }
  }

  *Ncrit = Ns;
  return (crit);
}

char **LoadRecipe (char *filter, int *nrecipe) {
  
  char **RecipeType;
  int Nrecipe, NRECIPE;

  int Nfield, found;
  char *c, *p1, line[256], list[256], Filter[64];
  FILE *f;
  
  /* open filter list file */
  f = fopen (RecipeFile, "r");
  if (f == (FILE *) NULL) {
    fprintf (stderr, "error reading recipe %s\n", RecipeFile);
    exit (1);
  }

  /* allocate dataspace needed */
  NRECIPE = 10;
  Nrecipe = 0;
  ALLOCATE (RecipeType, char *, NRECIPE);

  /* load data from file */
  found = FALSE;
  while (!found && (scan_line (f, line) != EOF)) {
    for (c = line; isspace (*c); c++);
    if (*c == '#') continue;
    Nfield = sscanf (c, "%s %s", Filter, list);
    if (Nfield != 2) { continue; }

    if (!strcasecmp (filter, Filter)) found = TRUE;
  }
  fclose (f);  

  if (!found) {
    fprintf (stderr, "can't find filter %s\n", filter);
    exit (1);
  }

  /* list contains word,word,word - parse these words to RecipeType */
  p1 = list;
  while ((c = strchr (p1, ',')) != (char *) NULL) {
    *c = 0;
    RecipeType[Nrecipe] = strcreate (p1);  
    p1 = c + 1;
    Nrecipe ++;
    if (Nrecipe == NRECIPE) {
      NRECIPE += 10;
      REALLOCATE (RecipeType, char *, NRECIPE);
    }
  }
  if (*p1) {
    RecipeType[Nrecipe] = strcreate (p1);  
    Nrecipe ++;
  }    
  *nrecipe = Nrecipe;
  return (RecipeType);
}
