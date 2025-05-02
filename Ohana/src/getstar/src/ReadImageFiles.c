# include "dvoImageOverlaps.h"

// given FOO:BAR:file.ext return file.ext 
char *nebbasename (char *name) {
 
  char *c, *file;

  ALLOCATE (file, char, strlen(name) + 16);

  c = strrchr (name, ':');
  if (c == (char *) NULL) {
    strcpy (file, name);
  } else {
    strcpy (file, c+1);
  }
  return (file);
}

Image *ReadImageFiles (char *filename, off_t *Nimages) {

  off_t Nskip, *extsize;
  int i, j, N, Nfile, Nheader, Nimage, NHEADER, NIMAGE;
  int Nhead, Ndata, done, status, mode;
  char **file, *name;
  FILE *f;
  glob_t globList;
  char **exthead, **extdata, **exttype, tmpword[80];
  int *extnum_head, *extnum_data;
  Header *header, **headers;
  Image *image;

  // parse the filename as a glob
  globList.gl_offs = 0;
  glob (filename, 0, NULL, &globList);

  // if the glob does not match, save the literal word:
  // otherwise save all glob matches
  if (globList.gl_pathc == 0) {
    Nfile = 1;
    ALLOCATE (file, char *, Nfile);
    file[0] = strcreate (filename);
  } else {
    Nfile = globList.gl_pathc;
    ALLOCATE (file, char *, Nfile);
    for (i = 0; i < Nfile; i++) {
      file[i] = strcreate (globList.gl_pathv[i]);
    }
  }

  // open the first file, read the PHU header
  f = fopen (file[0], "r");
  if (f == NULL) {
    fprintf (stderr, "ERROR: can't read header for %s\n", file[0]);
    exit (1);
  }
  ALLOCATE (header, Header, 1);
  gfits_fread_header (f, header);

  mode = GetFileMode (header);

  initMosaicCoords ();

  /*** load data from a single PHU or a collection of PHU files ***/
  if ((mode != SIMPLE_MEF) && (mode != MOSAIC_MEF)) {
    Nimage = Nfile;
    ALLOCATE (image, Image, Nimage);
    for (i = N = 0; i < Nfile; i++) {
      if (i > 0) {
	f = fopen (file[i], "r");
	if (f == NULL) {
	  fprintf (stderr, "can't read header for %s, skipping\n", file[i]);
	  continue;
	}
	gfits_fread_header (f, header);
      }

      if (!ReadImageHeader (header, &image[N])) {
	fprintf (stderr, "skipping %s\n", file[i]);
	continue;
      }

      // the image name just needs to be a useful identifier
      // remove the full path and in the case of nebulous files, remove the rest of
      // the nebulous path:
      // 14508045500.gpc1:ipp.tdbtest.tag.ps1.20210527.gentoo:o8966g0113o.1602706:o8966g0113o.1602706.cm.2417132.smf
      // -> o8966g0113o.1602706.cm.2417132.smf
      
      /* find image rootname */
      char *rawname = filebasename (file[i]);
      name = nebbasename (rawname); // 
      free (rawname);

      snprintf (image[N].name, DVO_IMAGE_NAME_LEN, "%s", name);
      free (name);

      fclose (f);
      gfits_free_header (header);
      N++;
    }
    *Nimages = N;
    
    if (N == 0) {
      fprintf (stderr, "ERROR: no valid image data in %s, giving up\n", filename);
      exit (1);
    }
    return image;
  }
    
  /* we have a multi-chip image */

  /* we need to examine the extensions to determine the headers and the data */
  NHEADER = 10;
  ALLOCATE (headers, Header *, NHEADER);

  // the first header is already loaded
  headers[0] = header;
  Nskip = gfits_data_size (header);
  fseeko (f, Nskip, SEEK_CUR); 

  // load all headers into memory
  done = FALSE;
  for (i = 1; !done; i++) {
      ALLOCATE (headers[i], Header, 1);
      status = gfits_fread_header (f, headers[i]);
      if (!status) { 
	  done = TRUE;
      } else {
	  Nskip = gfits_data_size (headers[i]);
	  fseek (f, Nskip, SEEK_CUR); 
      }
      if (i == NHEADER - 1) {
	  NHEADER += 10;
	  REALLOCATE (headers, Header *, NHEADER);
      }
  }
  Nheader = i - 1; /* we failed on the last loop */
    
  // space to store the images, indexes to the matching headers
  Nimage = 0;
  NIMAGE = Nheader;
  ALLOCATE (image, Image, NIMAGE);
  ALLOCATE (exthead, char *, NIMAGE);
  ALLOCATE (extdata, char *, NIMAGE);
  ALLOCATE (exttype, char *, NIMAGE);
  ALLOCATE (extnum_head, int, NIMAGE);
  ALLOCATE (extnum_data, int, NIMAGE);
  ALLOCATE (extsize, off_t, Nheader);

  if (mode == MOSAIC_MEF) {
      exthead[Nimage] = strcreate ("PHU");
      extdata[Nimage] = strcreate ("NONE");
      extnum_data[Nimage] = -1;
      extnum_head[Nimage] = 0;
      Nimage ++;
  }

  // now examine the headers, count the table entries, find corresponding headers
  for (i = 0; i < Nheader; i++) {
      extsize[i] = headers[i][0].datasize + gfits_data_size (headers[i]);
      gfits_scan (headers[i], "EXTTYPE", "%s", 1, tmpword);

      if (!strcmp (tmpword, "SMPDATA")   ||  
	  !strcmp (tmpword, "PS1_DEV_0") ||  
	  !strcmp (tmpword, "PS1_DEV_1") ||  
	  !strcmp (tmpword, "PS1_V1") ||  
	  !strcmp (tmpword, "PS1_V2") ||  
	  !strcmp (tmpword, "PS1_V3") ||  
	  !strcmp (tmpword, "PS1_V4") ||  
	  !strcmp (tmpword, "PS1_V5")) {

	  exttype[Nimage] = strcreate (tmpword);
	  gfits_scan (headers[i], "EXTNAME", "%s", 1, tmpword);
	  extdata[Nimage] = strcreate (tmpword);
	  gfits_scan (headers[i], "EXTHEAD", "%s", 1, tmpword);
	  exthead[Nimage] = strcreate (tmpword);
	  extnum_data[Nimage] = i;
	  extnum_head[Nimage] = -1;
	  // find the matching exthead entry
	  for (j = 0; j < Nheader; j++) {
	    if (gfits_scan (headers[j], "EXTNAME", "%s", 1, tmpword)) {
	      if (!strcmp (tmpword, exthead[Nimage])) {
		extnum_head[Nimage] = j;
	      }
	    }
	  }
	  // skip or crash on table with missing matching header?
	  if (extnum_head[Nimage] == -1) {
	      fprintf (stderr, "ERROR: can't read header for %s\n", file[0]);
	      exit (1);
	  }
	  Nimage ++;
      }
  }

  // some old format files did not write EXTTYPE.  they have a single table in the first
  // extension matched to the header in the PHU
  if (Nimage == 0) {
      extsize[0] = headers[0][0].datasize + gfits_data_size (headers[0]);
      extsize[1] = headers[1][0].datasize + gfits_data_size (headers[1]);
      gfits_scan (headers[1], "EXTNAME", "%s", 1, tmpword);
      if (!strcmp (tmpword, "SMPFILE")) {
	  extdata[Nimage] = strcreate (tmpword);
	  exttype[Nimage] = strcreate ("SMPDATA");
	  exthead[Nimage] = strcreate ("PHU");
	  extnum_head[Nimage] = 0;
	  extnum_data[Nimage] = 1;
	  Nimage = 1;
      }
  }
  if (Nimage == 0) Shutdown ("no object data in file");
    
  if (VERBOSE) fprintf (stderr, "file %s has %d headers, including %d images\n", file[0], Nheader, Nimage);

  /* find image rootname */
  char *rawname = filebasename (file[0]);
  name = nebbasename (rawname);
  free (rawname);
  
  // now run through the images, interpret the headers and read the stars
  for (i = N = 0; i < Nimage; i++) {
      Nhead = extnum_head[i];

      // XXX do I need to advance the file pointer, or does ReadImageHeader do this?
      if (VERBOSE) fprintf (stderr, "reading header for %s (%s)\n", exthead[i], extdata[i]);
      if (!ReadImageHeader (headers[Nhead], &image[N])) {
	  fprintf (stderr, "skipping %s\n", exthead[i]);
	  continue;
      }

      // XXX use something to set the chip name? EXTNAME?
      if (!strcmp(exthead[i], "PHU") && (Nimage == 1)) {
	snprintf (image[N].name, DVO_IMAGE_NAME_LEN, "%s", name);
      } else {
	snprintf (image[N].name, DVO_IMAGE_NAME_LEN, "%s[%s]", name, exthead[i]);
      }

      // skip the table if there is not data segment (eg, mosaic WRP image)
      if (!strcmp(extdata[i], "NONE")) {
	  N++;
	  continue;
      }

      // advance the pointer to the start of the corresponding table block
      Ndata = extnum_data[i];
      Nskip = 0;
      for (j = 0; j < Ndata; j++) {
	  Nskip += extsize[j];
      }
      fseek (f, Nskip, SEEK_SET); 
      N++;
  }
  free (name);
  *Nimages = N;

  if (N == 0) {
    fprintf (stderr, "ERROR: no valid image data in %s, giving up\n", filename);
    exit (1);
  }

  return image;
}
