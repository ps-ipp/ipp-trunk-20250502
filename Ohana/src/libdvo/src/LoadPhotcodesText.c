# include <dvo.h>

static int PhotcodeNumberOrName (char *word) {

  int value;
  char *endpt;

  if (!strcmp (word, "-")) return (0);

  value = strtol (word, &endpt, 10);
  if (endpt != word + strlen(word)) {
    value = GetPhotcodeCodebyName (word);
  }
  return (value);
}

# define MAX_LINE_LENGTH 1024
# define MAX_WORD_LENGTH 64

/* load the text photcode table */
int LoadPhotcodesText (char *filename) {
  
  /* XXX fix these */
  PhotCodeData *table = NULL;
  PhotCode *photcode;

  FILE *f;
  int i, Nsecfilt, Nsec, Ncode, NPHOTCODE, Nfield;
  int code;
  char *c;
  char line[MAX_LINE_LENGTH], **c1_names, **c2_names, **eq_names;
  char name[MAX_WORD_LENGTH], type[MAX_WORD_LENGTH], Zero[MAX_WORD_LENGTH], Airmass[MAX_WORD_LENGTH], Offset[MAX_WORD_LENGTH];
  char C1[MAX_WORD_LENGTH], C2[MAX_WORD_LENGTH], Slope[MAX_WORD_LENGTH], Color[MAX_WORD_LENGTH], Primary[MAX_WORD_LENGTH];
  char astromErrSys[MAX_WORD_LENGTH], astromErrScale[MAX_WORD_LENGTH], astromErrMagScale[MAX_WORD_LENGTH], photomErrSys[MAX_WORD_LENGTH];
  char astromPoorMask[MAX_WORD_LENGTH], astromBadMask[MAX_WORD_LENGTH], photomPoorMask[MAX_WORD_LENGTH], photomBadMask[MAX_WORD_LENGTH];

  table = GetPhotcodeTable ();
  if (table[0].code != NULL) free (table[0].code);
  /* we are using a 16-bit int for the photcodes, so these indexes can be fixed-length */
  /* XXX if we need to go with a larger photcode, we'll need to use a sequenced index and a
     binary search to get to a given value (0x100000000 ints would take quite a few
     bytes...) */
  for (i = 0; i < 0x10000; i++) {
    table[0].hashcode[i] = -1;
    table[0].hashNsec[i] = -1;
    table[0].codeNsec[i] = -1;
  }

  f = fopen (filename, "r");
  if (f == (FILE *) NULL) {
    table[0].Ncode    = 0;
    table[0].Nsecfilt = 0;
    table[0].code     = (PhotCode *) NULL;
    return (FALSE);
  }

  Ncode = 0;
  NPHOTCODE = 10;
  ALLOCATE (photcode, PhotCode, NPHOTCODE);
  ALLOCATE (eq_names, char *,   NPHOTCODE);
  ALLOCATE (c1_names, char *,   NPHOTCODE);
  ALLOCATE (c2_names, char *,   NPHOTCODE);

  while (scan_line_maxlen (f, line, MAX_LINE_LENGTH) != EOF) {
    for (c = line; isspace (*c); c++);
    if (*c == '#') continue;
    Nfield = sscanf (c, "%d %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s", 
		     &code, name, type, Zero, Airmass, Offset, C1, C2, Slope, Color, Primary, astromErrSys, astromErrScale, astromErrMagScale, photomErrSys, astromPoorMask, astromBadMask, photomPoorMask, photomBadMask);

    switch (Nfield) {
      case 11: 
	// minimum number of fields : original elixir layout
	strcpy (astromErrSys,      "0.0");
	strcpy (astromErrScale,    "0.0");
	strcpy (astromErrMagScale, "0.0");
	strcpy (photomErrSys,      "0.0");
	strcpy (astromPoorMask,    "0");
	strcpy (astromBadMask,     "0");
	strcpy (photomPoorMask,    "0");
	strcpy (photomBadMask,     "0");
	break;
      case 14: 
	// allow only astrom elements
	strcpy (photomErrSys,      "0.0");
	strcpy (astromPoorMask,    "0");
	strcpy (astromBadMask,     "0");
	strcpy (photomPoorMask,    "0");
	strcpy (photomBadMask,     "0");
	break;
      case 15: 
	strcpy (astromPoorMask,    "0");
	strcpy (astromBadMask,     "0");
	strcpy (photomPoorMask,    "0");
	strcpy (photomBadMask,     "0");
	break;
      case 17: 
	strcpy (photomPoorMask,    "0");
	strcpy (photomBadMask,     "0");
	break;
      case 19: 
	// all fields defined
	break;
      default:
	// skip unknown layouts
	continue;
    }

    if (!code) {
	fprintf (stderr, "photcode values may not be 0: fix %s\n", name);
	return (FALSE);
    }

    if (!strcasecmp(name, "MAG")) {
      fprintf (stderr, "MAG is not an allowed photcode name (reserved to DVO internals)\n");
      return FALSE;
    }

    photcode[Ncode].type = 0;
    memset(photcode[Ncode].dummy, 0, sizeof(photcode[Ncode].dummy));
    photcode[Ncode].code = code;
    memset (photcode[Ncode].name, 0, 32);
    strcpy (photcode[Ncode].name, name);

    if (!strncasecmp (type, "pri", 3)) {
      photcode[Ncode].type  = PHOT_SEC;
    }
    if (!strncasecmp (type, "sec", 3)) {
      photcode[Ncode].type  = PHOT_SEC;
    }
    if (!strncasecmp (type, "dep", 3)) {
      photcode[Ncode].type  = PHOT_DEP;
    }
    if (!strncasecmp (type, "ref", 3)) {
      photcode[Ncode].type  = PHOT_REF;
    }
    if (!strncasecmp (type, "alt", 3)) {
      /* alt photcodes are a little different: they have the SAME photcode as an existing
	 pri/sec photcode, but define an alternate transformation for that code */
      photcode[Ncode].type  = PHOT_ALT;
    }

    photcode[Ncode].astromErrSys      = atof (astromErrSys);
    photcode[Ncode].astromErrScale    = atof (astromErrScale);
    photcode[Ncode].astromErrMagScale = atof (astromErrMagScale);
    photcode[Ncode].photomErrSys      = atof (photomErrSys);

    photcode[Ncode].astromPoorMask    = strtoll (astromPoorMask, NULL, 0);
    photcode[Ncode].astromBadMask     = strtoll (astromBadMask, NULL, 0);
    photcode[Ncode].photomPoorMask    = strtoll (photomPoorMask, NULL, 0);
    photcode[Ncode].photomBadMask     = strtoll (photomBadMask, NULL, 0);

    switch (photcode[Ncode].type) {
      case PHOT_SEC:
      case PHOT_ALT:
      case PHOT_DEP:
	photcode[Ncode].C     = 1000*atof (Zero);
	photcode[Ncode].K     = atof (Airmass);
	photcode[Ncode].dC    = 1000*atof (Offset);
	photcode[Ncode].dX    = 1000*atof (Color);
	c1_names[Ncode]       = strcreate (C1);
	c2_names[Ncode]       = strcreate (C2);
	eq_names[Ncode]       = strcreate (Primary);
	ParseColorTerms (Slope, photcode[Ncode].X, &photcode[Ncode].Nc);
	break;

      case PHOT_REF:
	photcode[Ncode].C     = 0;
	photcode[Ncode].K     = 0;
	photcode[Ncode].dC    = 0;
	photcode[Ncode].dX    = 0;
	c1_names[Ncode]       = strcreate ("0");
	c2_names[Ncode]       = strcreate ("0");
	eq_names[Ncode]       = strcreate (Primary);
	photcode[Ncode].X[0]  = 0;
	photcode[Ncode].Nc    = 0;
	break;

      default:
	fprintf (stderr, "error: invalid photcode type\n");
	exit (2);
    }      

    if (!photcode[Ncode].type) {
      fprintf (stderr, "error in Photfile: unknown type %s\n", type);
    }

    Ncode++;
    if (Ncode == NPHOTCODE) {
      NPHOTCODE += 10;
      REALLOCATE (photcode, PhotCode, NPHOTCODE);
      REALLOCATE (eq_names, char *,   NPHOTCODE);
      REALLOCATE (c1_names, char *,   NPHOTCODE);
      REALLOCATE (c2_names, char *,   NPHOTCODE);
    }
  }
  fclose (f);
  
  // convert the named references (c1, c2, equiv) to code values
  for (i = 0; i < Ncode; i++) {
    photcode[i].c1    = PhotcodeNumberOrName (c1_names[i]);
    photcode[i].c2    = PhotcodeNumberOrName (c2_names[i]);
    photcode[i].equiv = PhotcodeNumberOrName (eq_names[i]);
    free (c1_names[i]);
    free (c2_names[i]);
    free (eq_names[i]);
  }
  free (c1_names);
  free (c2_names);
  free (eq_names);

  /* set up photcode indexes (see dvo_photcode_ops.c) */
  Nsecfilt = 0;
  for (i = 0; i < Ncode; i++) {
    if (photcode[i].type == PHOT_ALT) continue; /* no hashcode for ALT codes */
    if (table[0].hashcode[photcode[i].code] != -1) {
      fprintf (stderr, "duplicate photcodes in file\n");
      code = table[0].hashcode[photcode[i].code];
      fprintf (stderr, "conflict between %s (%d) and %s (%d)\n",
	       photcode[i].name, photcode[i].code, photcode[code].name, photcode[code].code);
      free (photcode);
      return (FALSE);
    }
    table[0].hashcode[photcode[i].code] = i;
    if (photcode[i].type == PHOT_SEC) {
      table[0].hashNsec[photcode[i].code] = Nsecfilt;
      table[0].codeNsec[Nsecfilt] = photcode[i].code;
      Nsecfilt ++;
    }
  }

  // validity check for references
  // photcode.equiv of 0 means "undefined"
  for (i = 0; i < Ncode; i++) {
    if (photcode[i].type == PHOT_DEP) {
      if (photcode[i].equiv == 0) continue;
      Nsec = table[0].hashcode[photcode[i].equiv];
      if ((Nsec >= Ncode) || (Nsec < 0)) {
	fprintf (stderr, "reference for dependent photcode is not a valid photcode\n");
	free (photcode);
	return (FALSE);
      }
      if (photcode[Nsec].type != PHOT_SEC) {
	fprintf (stderr, "reference for dependent photcode is not an average photcode\n");
	free (photcode);
	return (FALSE);
      }
    }
    if (photcode[i].type == PHOT_ALT) {
      Nsec = table[0].hashcode[photcode[i].code];
      if ((Nsec >= Ncode) || (Nsec < 0)) {
	fprintf (stderr, "reference for alternate photcode is not in photcodes\n");
	free (photcode);
	return (FALSE);
      }
      if (photcode[Nsec].type != PHOT_SEC) {
	fprintf (stderr, "reference for alternate photcode is not an average photcode\n");
	free (photcode);
	return (FALSE);
      }
    }
  }
  table[0].code     = photcode;
  table[0].Ncode    = Ncode;
  table[0].Nsecfilt = Nsecfilt;

  return (TRUE);
}

# define NCTERMS 4
void ParseColorTerms (char *terms, float *X, int *N) {

  int i;
  char *p;

  p = terms;

  for (i = 0; (p != NULL) && (i < NCTERMS); i++) {
    X[i] = atof (p);
    p = strchr (p, ',');
    if (p == (char *) NULL) continue;
    p ++;
  }
  *N = i;
}
