# include <ohana.h>

main (argc, argv)
     int argc;
     char **argv;
{
  
  int status, REL, N;
  Header header, refhead;
  char  filename[1024], reffile[1024], *p;
  double X_O, X_X, X_Y, Y_O, Y_X, Y_Y;
  double RA_O, RA_X, RA_Y, DEC_O, DEC_X, DEC_Y;
  double ra_o, ra_x, ra_y, dec_o, dec_x, dec_y;
  double dRA, dDEC, dX, dY, dra, ddec;
  
  if (N = get_argument (argc, argv, "-rel")) {
    remove_argument (N, &argc, argv);
    REL = TRUE;
  }
  else {
    REL = FALSE;
  }
  
  while (fscanf (stdin, "%s", filename) != EOF) {
    
    status = gfits_read_header (filename, &header);
    if (!status) {
      fprintf (stderr, "error opening file %s\n", filename);
      continue;
    }
    status = gfits_scan (&header, "rREF", "%s", 1, reffile);
    status &= gfits_scan (&header, "X_O", "%lf", 1, &X_O);
    status &= gfits_scan (&header, "X_X", "%lf", 1, &X_X);
    status &= gfits_scan (&header, "X_Y", "%lf", 1, &X_Y);
    status &= gfits_scan (&header, "Y_O", "%lf", 1, &Y_O);
    status &= gfits_scan (&header, "Y_X", "%lf", 1, &Y_X);
    status &= gfits_scan (&header, "Y_Y", "%lf", 1, &Y_Y);
    status &= gfits_scan (&header, "dX", "%lf", 1, &dX);
    status &= gfits_scan (&header, "dY", "%lf", 1, &dY);
    if (!status) {
      fprintf (stderr, "file missing rastro info: %s\n", filename);
      gfits_free_header (&header);
      continue;
    }

    while ((p = strstr (reffile, "'")) != NULL) {
      *p = ' ';
    }
    stripwhite (reffile);
    if ((p = strrchr(reffile, '.')) != NULL)
      strcpy(p, ".head");
    else 
      strcat(reffile, ".head");

    if (!strcmp (reffile, filename)) {
      fprintf (stderr, "%s is %s, not altering\n", reffile, filename);
      continue;
    }

    status  = gfits_read_header (reffile, &refhead);
    if (!status) {
      fprintf (stderr, "error opening file %s\n", reffile);
      continue;
    }
    status &= gfits_scan (&refhead, "RA_O", "%lf", 1, &RA_O);
    status &= gfits_scan (&refhead, "RA_X", "%lf", 1, &RA_X);
    status &= gfits_scan (&refhead, "RA_Y", "%lf", 1, &RA_Y);
    status &= gfits_scan (&refhead, "DEC_O", "%lf", 1, &DEC_O);
    status &= gfits_scan (&refhead, "DEC_X", "%lf", 1, &DEC_X);
    status &= gfits_scan (&refhead, "DEC_Y", "%lf", 1, &DEC_Y);
    status &= gfits_scan (&refhead, "dRA",   "%lf", 1, &dRA);
    status &= gfits_scan (&refhead, "dDEC",  "%lf", 1, &dDEC);
    if (!status) {
      fprintf (stderr, "file missing rastro info: %s\n", reffile);
      gfits_free_header (&header);
      gfits_free_header (&refhead);
      continue;
    }

    fprintf (stderr, "%f %f %f\n%f %f %f\n", RA_O, RA_X, RA_Y, DEC_O, DEC_X, DEC_Y);
    fprintf (stderr, "\n%f %f %f\n%f %f %f\n", X_O, X_X, X_Y, Y_O, Y_X, Y_Y);
    
    ra_o = RA_X*X_O + RA_Y*Y_O + RA_O;
    dec_o = DEC_X*X_O + DEC_Y*Y_O + DEC_O;
    
    ra_x = RA_X*X_X + RA_Y*Y_X;
    ra_y = RA_X*X_Y + RA_Y*Y_Y;
    dec_x = DEC_X*X_X + DEC_Y*Y_X;
    dec_y = DEC_X*X_Y + DEC_Y*Y_Y;
    
    fprintf (stderr, "%f %f %f\n%f %f %f\n", ra_o, ra_x, ra_y, dec_o, dec_x, dec_y);

    dra = sqrt(dRA*dRA + SQ(3600*dX*RA_X) + SQ(3600*dY*RA_Y));
    ddec = sqrt(dDEC*dDEC + SQ(3600*dX*DEC_X) + SQ(3600*dY*DEC_Y));
			   
    status &= gfits_modify (&header, "RA_O", "%lf", 1, ra_o);
    status &= gfits_modify (&header, "RA_X", "%le", 1, ra_x);
    status &= gfits_modify (&header, "RA_Y", "%le", 1, ra_y);
    status &= gfits_modify (&header, "DEC_O", "%lf", 1, dec_o);
    status &= gfits_modify (&header, "DEC_X", "%le", 1, dec_x);
    status &= gfits_modify (&header, "DEC_Y", "%le", 1, dec_y);
    status &= gfits_modify (&header, "dRA",   "%lf", 1, dra);
    status &= gfits_modify (&header, "dDEC",  "%lf", 1, ddec);
    
    gfits_write_header (filename, &header);
    gfits_free_header (&header);
    gfits_free_header (&refhead);
    
  }

}

