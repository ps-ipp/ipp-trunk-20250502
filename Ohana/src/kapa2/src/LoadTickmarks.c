# include "Ximage.h"

int LoadTickmarks (int sock) {
  
  char line[129], type[16];
  double x, y, dx, dy;
  int status, NOBJECTS, Nobjects, done;
  Section *section;
  KapaImageWidget *image;

  section = GetActiveSection();
  if (section->image == NULL) {
    section->image = InitImageWidget ();
    SetSectionSizes (section);
  }
  image   = section->image;

  Nobjects = image[0].tickmarks.Nobjects;
  NOBJECTS = Nobjects + 100;
  REALLOCATE (image[0].tickmarks.objects, KiiOverlay, NOBJECTS);
  
  done = FALSE;
  while (!done) {
    status = read (sock, line, 128); 
    line[128] = 0; 

    if (!strncmp (line, "DONE", 4)) {
      done = TRUE;
      break;
    }
    
    sscanf (line, "%s %lf %lf %lf %lf\n", type, &x, &y, &dx, &dy);
    
    if (strcmp (type, "TEXT") && strcmp (type, "LINE") && strcmp (type, "BOX") && strcmp (type, "CIRCLE")) {  /* skip */
      fprintf (stderr, "don't know %s, skipping\n", type);
      continue;
    }
    
    strcpy (image[0].tickmarks.objects[Nobjects].type, type);
    image[0].tickmarks.objects[Nobjects].x = x;
    image[0].tickmarks.objects[Nobjects].y = y;
    image[0].tickmarks.objects[Nobjects].dx = dx;
    image[0].tickmarks.objects[Nobjects].dy = dy;
    
    if (!strcmp (type, "TEXT")) { /* dx = Nchar, dy = angle (not yet used) */
      status = read (sock, line, 128); 
      line[128] = 0; 
      ALLOCATE (image[0].tickmarks.objects[Nobjects].text, char, (int) dx + 1);
      strncpy_nowarn (image[0].tickmarks.objects[Nobjects].text, line, (int) dx);
    }      
    
    Nobjects++;
    if (Nobjects >= NOBJECTS) {
      NOBJECTS = Nobjects + 100;
      REALLOCATE (image[0].tickmarks.objects, KiiOverlay, NOBJECTS);
    }

  }

  REALLOCATE (image[0].tickmarks.objects, KiiOverlay, MAX(Nobjects, 1));
  image[0].tickmarks.Nobjects = Nobjects;

  if (USE_XWINDOW) Refresh ();
  return (TRUE);
}
