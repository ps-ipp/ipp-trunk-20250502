# include "Ximage.h"

static int  ActiveSection  = 0;
static int       Nsections = 0;
static Section **sections = NULL;

Section *InitSection () {

  Section *section;
  ALLOCATE (section, Section, 1);

  section[0].graph = NULL;
  section[0].image = NULL;
  section[0].x = 0;
  section[0].y = 0;
  section[0].dx = 0;
  section[0].dy = 0;
  section[0].bg = -1;

  section[0].name = NULL;
  return (section);
}

void FreeSection (Section *section) {

  if (section == NULL) return;
  FreeGraph (section[0].graph);
  FreeImage (section[0].image);
  FREE (section[0].name);
  free (section);
}

void FreeSections () {

  int i;

  for (i = 0; i < Nsections; i++) {
    FreeSection (sections[i]);
  }
  free (sections);
  Nsections = 0;
  sections = NULL;
}

Section *AddSection (char *name, float x, float y, float dx, float dy, int bg) {

  int N;

  N = Nsections;

  if (sections == NULL) {
    Nsections = 1;
    ALLOCATE (sections, Section *, Nsections);
  } else {
    Nsections ++;
    REALLOCATE (sections, Section *, MAX (1, Nsections));
  }
  sections[N] = InitSection();
  sections[N][0].name = strcreate (name);
  sections[N][0].x = x;
  sections[N][0].y = y;
  sections[N][0].dx = dx;
  sections[N][0].dy = dy;
  sections[N][0].bg = bg;
  ActiveSection = N;
  return (sections[N]);
}

int DelSection (char *name) {

  int i, j;

  for (i = 0; i < Nsections; i++) {
    if (strcmp (name, sections[i][0].name)) continue;
    FreeSection (sections[i]);
    for (j = i; j < Nsections - 1; j++) {
      sections[j] = sections[j+1];
    }
    return (TRUE);
  }
  return (FALSE);
}

int GetSectionByName (char *name) {

  int i;

  for (i = 0; i < Nsections; i++) {
    if (strcmp (name, sections[i][0].name)) continue;
    return (i);
  }
  return (-1);
}

int GetNumberOfSections () {
  return (Nsections);
}

Section *GetSectionByNumber (int N) {
  if (N >= Nsections) return NULL;
  if (N < 0) return NULL;
  return sections[N];
}

Section *GetActiveSection () {
  int N;
  N = ActiveSection;
  return (sections[N]);
}

int SetActiveSectionByNumber (int N) {
  if (N >= Nsections) return FALSE;
  if (N < 0) return FALSE;
  ActiveSection = N;
  return TRUE;
}

int ListSection (int sock) {
  
  int i, ThisSection;
  char name[128];

  KiiScanMessage (sock, "%s", name);

  if (!strcmp (name, "*")) {
    for (i = 0; i < Nsections; i++) {
      fprintf (stderr, "%s: %6.3f %6.3f %6.3f %6.3f\n", 
	       sections[i][0].name, sections[i][0].x, sections[i][0].y, sections[i][0].dx, sections[i][0].dy);
    }
    return (TRUE);
  }

  ThisSection = -1;
  for (i = 0; i < Nsections; i++) {
    if (!strcmp (name, sections[i][0].name)) {
      ThisSection = i;
      break;
    }
  }
  if (ThisSection == -1) {
    fprintf (stderr, "section %s not found\n", name);
    return (TRUE);
  }

  fprintf (stderr, "%s: %6.3f %6.3f %6.3f %6.3f\n", 
	   sections[ThisSection][0].name, 
	   sections[ThisSection][0].x,  sections[ThisSection][0].y, 
	   sections[ThisSection][0].dx, sections[ThisSection][0].dy);
  return (TRUE);
}

void SetSectionSizes (Section *section) {

    SetGraphSize (section);
    SetImageSize (section);
    return;
}

int SetSectionBG (int sock) {

  int N, background;
  char name[128];

  KiiScanMessage (sock, "%s %d", name, &background);
  
  N = GetSectionByName (name);
  if (N < 0) {
    fprintf (stderr, "section %s not found\n", name);
    return (TRUE);
  }

  sections[N][0].bg = background;

  return (TRUE);
}

// return TRUE even for nonsense cases to avoid quitting kapa
int MoveSection (int sock) {

  int i, N;
  char name[128];
  char direction[16];
  Section *tmpSection = NULL;

  KiiScanMessage (sock, "%s %s", name, direction);
  
  N = GetSectionByName (name);
  if (N < 0) {
    fprintf (stderr, "section %s not found\n", name);
    return (TRUE);
  }

  if (!strcasecmp (direction, "up")) {
      if (N < 0) return (TRUE);
      if (N > Nsections - 2) return (TRUE);
      tmpSection = sections[N];
      sections[N] = sections[N+1];
      sections[N+1] = tmpSection;
      Refresh ();
      return (TRUE);
  }

  if (!strcasecmp (direction, "down")) {
      if (N < 1) return (TRUE);
      if (N > Nsections - 1) return (TRUE);
      tmpSection = sections[N];
      sections[N] = sections[N-1];
      sections[N-1] = tmpSection;
      Refresh ();
      return (TRUE);
  }

  if (!strcasecmp (direction, "top")) {
      if (N < 0) return (TRUE);
      if (N > Nsections - 2) return (TRUE);
      tmpSection = sections[N];
      for (i = N; i < Nsections - 1; i++) {
	sections[i] = sections[i+1];
      }
      sections[i] = tmpSection;
      Refresh ();
      return (TRUE);
  }

  if (!strcasecmp (direction, "bottom")) {
      if (N < 1) return (TRUE);
      if (N > Nsections - 1) return (TRUE);
      tmpSection = sections[N];
      for (i = N; i >= 1; i--) {
	sections[i] = sections[i-1];
      }
      sections[i] = tmpSection;
      Refresh ();
      return (TRUE);
  }
  fprintf (stderr, "unknown direction %s for MoveSection\n", direction);
  return (TRUE);
}

int SectionMinBoundary (Graphic *graphic) {

    int i;
    int Xs = graphic->dx;
    int Ys = graphic->dy;
    int Xe = 0;
    int Ye = 0;

# if (1)
    graphic->xwin  = 0;
    graphic->ywin  = 0;
    graphic->dxwin = graphic->dx;
    graphic->dywin = graphic->dy; 
    return TRUE;
# endif

    // the boundary for a single section should probably be adjusted depending on the 
    // image status (should not include the imtool portion)
    for (i = 0; i < Nsections; i++) {
	Xs = MIN (Xs, sections[i][0].x);
	Ys = MIN (Ys, sections[i][0].y);
	Xe = MAX (Xe, sections[i][0].x + sections[i][0].dx);
	Ye = MAX (Ye, sections[i][0].y + sections[i][0].dy);
    }

    if ((Xs >= Xe) || (Ys >- Ye)) {
	// default values for the region window
	graphic->xwin  = 0;
	graphic->ywin  = 0;
	graphic->dxwin = graphic->dx;
	graphic->dywin = graphic->dy; 
    }	
    
    // set min/max boundary (min window containing max range of sections)
    graphic->xwin = Xs;
    graphic->ywin = Ys;
    graphic->dxwin = Xe - Xs;
    graphic->dywin = Ye - Ys;
    return TRUE;
}
