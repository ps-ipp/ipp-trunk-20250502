# include "dvomerge.h"

# define STRFAIL { fprintf (stderr, "failure on image %s\n", image[i].name); continue; }

int dvorepairFixStackIDs (int argc, char **argv) {

  FITS_DB db;  // database handle pointing to input image table

  off_t Nimage;
  
  Image *image;

  if (argc != 2) {
    fprintf (stderr, "USAGE: dvorepair -fix-stack-ids (catdir.list)\n");
    fprintf (stderr, "  catdir.list : list of databases of interest\n");
    fprintf (stderr, "  image.externID is set based on the image name:\n");
    fprintf (stderr, "  e.g., for: RINGS.V3.skycell.0798.031.stk.3923362.skycal.3788239.cmf[SkyChip.hdr]\n");
    fprintf (stderr, "  externID (stackID) follows stk: 3923362\n");
    exit (2);
  }

  char *catdir_list  = argv[1];

  char name[DVO_MAX_PATH];
  char catdir[DVO_MAX_PATH];
  char imageFilenameOld[DVO_MAX_PATH];
  char imageFilenameNew[DVO_MAX_PATH];

  // read the list of catdirs and fix image tables for each 

  FILE *f = fopen (catdir_list, "r");
  myAssert (f, "failed to open catdir.list %s\n", catdir_list);

  while (fscanf (f, "%s", catdir) != EOF) {
    snprintf_nowarn (imageFilenameOld, DVO_MAX_PATH, "%s/Images.dat", catdir);
    snprintf_nowarn (imageFilenameNew, DVO_MAX_PATH, "%s/Images.dat.fixed", catdir);
    
    if ((image = LoadImages (&db, imageFilenameOld, &Nimage)) == NULL) {
      fprintf (stderr, "error loading images\n");
      exit (1);
    }

    int i;
    for (i = 0; i < Nimage; i++) {
      if (image[i].externID) continue;
      
      strcpy (name, image[i].name);
    
      char *p0 = strchr (name  , '.'); if (!p0) STRFAIL;
      char *p1 = strchr (p0 + 1, '.'); if (!p1) STRFAIL;
      char *p2 = strchr (p1 + 1, '.'); if (!p2) STRFAIL;
      char *p3 = strchr (p2 + 1, '.'); if (!p3) STRFAIL;
      char *p4 = strchr (p3 + 1, '.'); if (!p4) STRFAIL;
      char *p5 = strchr (p4 + 1, '.'); if (!p5) STRFAIL;
      char *p6 = strchr (p5 + 1, '.'); if (!p6) STRFAIL;
    
      *p6 = 0;
      int myStackID = atoi (p5 + 1);

      image[i].externID = myStackID;
      image[i].sourceID = 35; // hard-wired for gpc1 stacks
    }
    SaveImages(&db, imageFilenameNew, image, Nimage);
    gfits_db_free (&db);
  }

  fclose (f);
  ohana_memdump (TRUE);

  exit (0);
}
