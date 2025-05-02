# include "imregister.h"

/*** this needs to be written using dvo_image functions !!! ***/

Image *LoadImageTable (FILE *f, Header *header, off_t *nimage);
static char *version = "imphotcopy $Revision: 1.6 $";

int main (int argc, char **argv) {
 
  int N, Ntimes;
  off_t status, Nimage;
  off_t i, j, Ninput, *index;
  Header header, theader;
  Image *image, *input;
  FILE *f, *g;
  char *dBFile;
  time_t *tstart, *tstop;
  int VERBOSE, PHOTCODE, dbstate;
  int PhotCodeSelect, Nin;
  char *NameSelect;
  int NameSelectLength;

  get_version (argc, argv, version);
  ConfigInit (&argc, argv);

  /* interpret command-line arguments */
  if (!get_trange_arguments (&argc, argv, &tstart, &tstop, &Ntimes)) {
    fprintf (stderr, "ERROR: syntax error\n");
    exit (1);
  }

  /* select by image photcode */
  PhotCodeSelect = FALSE;
  PHOTCODE = 0;
  if ((N = get_argument (argc, argv, "-photcode"))) {
    remove_argument (N, &argc, argv);
    if (!(PHOTCODE = GetPhotcodeCodebyName (argv[N]))) {
      fprintf (stderr, "ERROR: photcode not found in photcode table\n");
      exit (1);
    }
    remove_argument (N, &argc, argv);
    PhotCodeSelect = TRUE;
  }

  /* string in image name */
  NameSelect = (char *) NULL;
  NameSelectLength = 0;
  if ((N = get_argument (argc, argv, "-name"))) {
    remove_argument (N, &argc, argv);
    NameSelect = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
    NameSelectLength = strlen (NameSelect);
  }

  // char *FitsOutput = (char *) NULL;
  // if ((N = get_argument (argc, argv, "-fits"))) {
  //   remove_argument (N, &argc, argv);
  //   FitsOutput = strcreate (argv[N]);
  //   remove_argument (N, &argc, argv);
  // }

  VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    remove_argument (N, &argc, argv);
    VERBOSE = TRUE;
  }

  if (argc != 3) {
    fprintf (stderr, "USAGE: imphotmerge (input) (target) [config ops] [-trange start stop/delta] [-photcode code]\n");
    exit (1);
  }
 
  /* load image database - still a non-FITS file */
  dBFile = argv[2];

  f = fsetlockfile (dBFile, 120.0, LCK_HARD, &dbstate);
  if (f == (FILE *) NULL) {
    fprintf (stderr, "ERROR: can't set lock on %s, state is %d\n", dBFile, dbstate);
    exit (1);
  }
  image = LoadImageTable (f, &header, &Nimage);

  /* load input table */
  g = fopen (argv[1], "r");
  if (g == (FILE *) NULL) {
    fprintf (stderr, "error opening input data file\n");
    exit (1);
  }
  input = LoadImageTable (g, &theader, &Ninput);
  fclose (g);

  /* allocate space for reference lists */
  Nin = 0;
  ALLOCATE (index, off_t, Ninput);
  
  /* these filters are applied to input NOT image */
  for (i = 0; i < Ninput; i++) {
    for (j = 0, status = FALSE; !status && (j < Ntimes); j++) {
      status = (input[i].tzero >= tstart[j]) && (input[i].tzero <= tstop[j]);
    }
    if (!status && Ntimes) continue;
    if (PhotCodeSelect && (input[i].photcode != PHOTCODE)) continue;
    if ((NameSelect != (char *) NULL) && (strncasecmp (input[i].name, NameSelect, NameSelectLength))) continue;

    index[Nin] = i;
    Nin ++;
  }

  /* add new images to images */
  REALLOCATE (image, Image, Nimage + Nin);
  for (j = 0, i = Nimage; i < Nimage + Nin; i++, j++) {
    image[i] = input[index[j]];
  }
  Nimage += Nin;
  gfits_modify (&header, "NIMAGES", OFF_T_FMT, 1,  Nimage);

  /* position to begining of file to write header */
  fseeko (f, 0, SEEK_SET);
  status = Fwrite (header.buffer, 1, header.datasize, f, "char");
  if (status != header.datasize) {
    fprintf (stderr, "ERROR: failed writing data to image header\n");
    exit (0);
  }

  /* position to end of file for new image data */
  fseeko (f, header.datasize, SEEK_SET);
  status = Fwrite (image, sizeof(Image), Nimage, f, "image");
  if (status != Nimage) {
    fprintf (stderr, "ERROR: failed writing data to image catalog\n");
    exit (0);
  }

  fclearlockfile (dBFile, f, LCK_HARD, &dbstate);

  if (VERBOSE) fprintf (stderr, "SUCCESS\n");
  exit (0);

}

Image *LoadImageTable (FILE *f, Header *header, off_t *nimage) {

  off_t Nimage, Ndata, Nread, size;
  struct stat filestatus;
  Image *image;

  /* read header */
  if (!gfits_fread_header (f, header)) {
    fprintf (stderr, "ERROR: can't read image catalog\n");
    exit (1);
  }

  /* check that file size makes sense */
  Nimage = 0;
  gfits_scan (header, "NIMAGES", OFF_T_FMT, 1,  &Nimage);
  if (fstat (fileno(f), &filestatus) == -1) {
    fprintf (stderr, "ERROR: failed to get status of image catalog\n");
    exit (1);
  }
  size = Nimage*sizeof(Image) + header[0].datasize;
  if (size != filestatus.st_size) {
    Ndata = (filestatus.st_size - header[0].datasize) / sizeof (Image);
    fprintf (stderr, "ERROR: image catalog has inconsistent size\n");
    fprintf (stderr, "header: "OFF_T_FMT", data: "OFF_T_FMT"\n",  Nimage,  Ndata);
    Nimage = Ndata;
  } 

  /* alloc, read images */
  ALLOCATE (image, Image, MAX (Nimage, 1));
  Nread = Fread (image, sizeof(Image), Nimage, f, "image");
  if (Nread != Nimage) {
    fprintf (stderr, "ERROR: problem loading image catalog\n");
    exit (1);
  } 

  *nimage = Nimage;
  return (image);
}

