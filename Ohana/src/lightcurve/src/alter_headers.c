# include "relphot.h"

void alter_headers  (images, Nimages)
Image  *images;
int     Nimages;
{

  int i;
  Header header;
  char line[1000], head[1000];
  double clouds;
  FILE *f;

  if (!strcmp(IMFILE, "-")) {
    fprintf (stderr, "using stderr\n");
    f = stderr;
  }
  else {
    fprintf (stderr, "using %s for image stats\n", IMFILE);
    f = fopen (IMFILE, "w");
    if (f == NULL) {
      fprintf (stderr, "could not open output file, using stderr\n");
      f = stderr;
    }
  }
  
  fprintf (f, "# name                Mcal    dMcal   [time]  airmass   clouds <ap-fit> Nstars  Nunique  fixed  empty\n");
  for (i = 0; i < Nimages; i++) {
    strcpy (head, images[i].name);
    strcpy (strchr(head, '.'), ".head");
    gfits_read_header (head, &header);

    sprintf (line, "mv %s %s~\0", head, head);
    system (line);

    images[i].clouds = images[i].Mcal + C_LAMBDA - A_LAMBDA*images[i].airmass + images[i].AmF;

    fprintf (f, "%s  %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f  %6d   %6d      %1d      %1d\n",  
	     images[i].name, images[i].Mcal, images[i].dMcal, images[i].Mtime, 
	     images[i].airmass, images[i].clouds, images[i].AmF, images[i].Nstars, 
	     images[i].Nunique, images[i].fixed, images[i].empty);

    gfits_modify (&header, "Mcal",   "%lf", 1, images[i].Mcal);
    gfits_modify (&header, "dMcal",  "%lf", 1, images[i].dMcal);
    gfits_modify (&header, "clouds", "%lf", 1, images[i].clouds);
    gfits_modify (&header, "NMcal",  "%d", 1,  images[i].Nstars);

    gfits_modify_alt (&header, "Mcal",   "%C", 1, "relphot: calibration magnitude");
    gfits_modify_alt (&header, "dMcal",  "%C", 1, "relphot: calibration error");
    gfits_modify_alt (&header, "clouds", "%C", 1, "relphot: cloud level");
    gfits_modify_alt (&header, "NMcal",  "%C", 1, "relphot: number of stars");

    gfits_write_header (head, &header);
    gfits_free_header (&header);
  }
  fclose (f);
}
