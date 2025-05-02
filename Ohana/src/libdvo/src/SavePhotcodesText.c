# include <dvo.h>
# define SCALE 0.001

static char *PHOT_SEC_NAME = "sec";
static char *PHOT_ALT_NAME = "alt";
static char *PHOT_REF_NAME = "ref";
static char *PHOT_DEP_NAME = "dep";

/* this function saves the FITS photcode file into the internal photcode table */
/* locking is used to avoid collisions with programs trying to update the photcodes values */
/* XXX better distinction between NOT FOUND and FAILURE */
int SavePhotcodesText (char *filename) {

  PhotCodeData *table = NULL;
  struct stat filestat;
  char *type;
  int i, j, status;
  FILE *f;

  table = GetPhotcodeTable ();
  if (table == NULL) {
    fprintf (stderr, "ERROR: no internal photcode table is defined\n");
    return FALSE;
  }

  /* check if file exists */
  status = stat (filename, &filestat);
  if (status == -1) {
    if (errno != ENOENT) {
      fprintf (stderr, "ERROR: problem accessing output path for%s\n", filename);
      return FALSE;
    }
  } else {
    make_backup (filename);
  } 

  f = fopen (filename, "w");
  if (f == NULL) {
    fprintf (stderr, "ERROR: problem creating photcode file %s\n", filename);
    return FALSE;
  }

  fprintf (f, "#                                           airmass      color                         astrometry  mag    photom  astrom mask    photom mask\n");
  fprintf (f, "# code  name                type    zero  slope offset c1    c2   slope   zero  equiv  sys scale   scale  sys     poor   bad     poor   bad\n");
  //          "  1     g                    sec   0.000  0.000 0.000  1003  1004 0.0160     0  1051   0.000 0.000 0.000  0.000   0xffff 0xffff  0xffff 0xffff

  for (i = 0; i < table[0].Ncode; i++) {
    switch (table[0].code[i].type) {
      case PHOT_SEC:
	type = PHOT_SEC_NAME;
	break;
      case PHOT_ALT:
	type = PHOT_ALT_NAME;
	break;
      case PHOT_REF:
	type = PHOT_REF_NAME;
	break;
      case PHOT_DEP:
	type = PHOT_DEP_NAME;
	break;
      default:
	fprintf (stderr, "ERROR: problem with photcode type for %s\n", GetPhotcodeNamebyCode(table[0].code[i].code));
	return FALSE;
    }

    fprintf (f, "  %-5d %-18s  %4s  %6.3f %6.3f %5.3f ",
	     table[0].code[i].code,
	     GetPhotcodeNamebyCode (table[0].code[i].code),
	     type,
	     table[0].code[i].C*SCALE, 
	     table[0].code[i].K, 
	     table[0].code[i].dC*SCALE);

    PrintPhotcodeNamebyCode (f, "%5d ", table[0].code[i].c1);
    PrintPhotcodeNamebyCode (f, "%5d ", table[0].code[i].c2);

    for (j = 0; j < table[0].code[i].Nc - 1; j++) {
      fprintf (f, " %6.4f,", table[0].code[i].X[j]);
    }
    fprintf (f, "%6.4f %5d ", table[0].code[i].X[j], table[0].code[i].dX);
    PrintPhotcodeNamebyCode (f, "%5d ", table[0].code[i].equiv);

    fprintf (f, "  %5.3f %5.3f %5.3f  %5.3f", 
	     table[0].code[i].astromErrSys, 
	     table[0].code[i].astromErrScale, 
	     table[0].code[i].astromErrMagScale, 
	     table[0].code[i].photomErrSys); 

    fprintf (f, "   0x%04x 0x%04x  0x%04x 0x%04x", 
	     table[0].code[i].astromPoorMask, 
	     table[0].code[i].astromBadMask, 
	     table[0].code[i].photomPoorMask, 
	     table[0].code[i].photomBadMask); 

    fprintf (f, "\n");
  }
  fclose (f);
  return TRUE;
}

void PrintPhotcodeNamebyCode (FILE *f, char *format, int code) {

  char *name;
  
  name = GetPhotcodeNamebyCode (code);
  if (name == NULL) {
    fprintf (f, "    - ");
  } else {
    fprintf (f, format, code);
  }
}
