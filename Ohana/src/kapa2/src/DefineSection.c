# include "Ximage.h"

// define the section, but do not create a graph or image 
int DefineSection (int sock) {
  
  int N, MoveSection, bg;
  char name[128];
  double x, y, dx, dy;
  Section *section;

  KiiScanMessage (sock, "%s %lf %lf %lf %lf %d", name, &x, &y, &dx, &dy, &bg);

  MoveSection = FALSE;

  N = GetSectionByName (name);
  if (N < 0) {
    section = AddSection (name, x, y, dx, dy, bg);
    MoveSection = TRUE;
  } else {
    section = GetSectionByNumber (N);
    SetActiveSectionByNumber (N);

    if (section[0].x != x)   MoveSection = TRUE;
    if (section[0].y != y)   MoveSection = TRUE;
    if (section[0].dx != dx) MoveSection = TRUE;
    if (section[0].dy != dy) MoveSection = TRUE;

    section[0].x = x;
    section[0].y = y;
    section[0].dx = dx;
    section[0].dy = dy;
    section[0].bg = bg;
  }

  if (MoveSection) {
    SetSectionSizes (section);
    Refresh ();
  }

  return (TRUE);
}

// define the section, but do not create a graph or image 
int DefineSectionByImage (int sock) {
  
  int N, bg;
  char name[128];
  double x, y, dx, dy;
  int dXm, dXp, dYm, dYp;
  double x0, y0, x1, y1, expand;
  Section *section;
  Graphic *graphic;
  KapaImageWidget *image;

  KiiScanMessage (sock, "%s %lf %lf %d", name, &x, &y, &bg);

  N = GetSectionByName (name);
  section = GetSectionByNumber (N);
  image = section->image;
  if (!image) {
    fprintf (stderr, "no image to define section\n");
    return (TRUE);
  }
  SetActiveSectionByNumber (N);

  graphic = GetGraphic ();

  GetGraphBoundary (section, &x0, &y0, &x1, &y1, &dXm, &dXp, &dYm, &dYp);

  expand = 1.0;
  if (image[0].picture.expand > 0) {
    expand = image[0].picture.expand;
  }
  if (image[0].picture.expand < 0) {
    expand = 1.0 / fabs((double)image[0].picture.expand);
  }

  // pixel dimensions of the imaging region + boundary
  dx = image[0].image[0].matrix.Naxis[0]*expand + x1;
  dy = image[0].image[0].matrix.Naxis[1]*expand + y1;

  section[0].x = x;
  section[0].y = y;
  section[0].bg = bg;
  section[0].dx = dx / graphic[0].dx;
  section[0].dy = dy / graphic[0].dy;

  // if (section[0].x != x)   MoveSection = TRUE;
  // if (section[0].y != y)   MoveSection = TRUE;
  // if (section[0].dx != dx) MoveSection = TRUE;
  // if (section[0].dy != dy) MoveSection = TRUE;

  SetSectionSizes (section);
  Refresh ();

  return (TRUE);
}
