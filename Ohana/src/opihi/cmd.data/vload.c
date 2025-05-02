# include "data.h"

int vload (int argc, char **argv) {
  
  int i, N, Noverlay;
  int kapa, type;
  char *name;
  double dx, dy, angle;
  KiiOverlay *overlay;
  Vector *vecx, *vecy;
  
  name = NULL;
  if ((N = get_argument (argc, argv, "-n"))) {
    remove_argument (N, &argc, argv);
    name = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!GetImage (NULL, &kapa, name)) return (FALSE);
  FREE (name);

  int channel = 0;
  if ((N = get_argument (argc, argv, "-ch"))) {
    channel = GetKapaChannelFromString (argv[N]);
    if (!channel) return FALSE;
    KiiSetChannel (kapa, channel - 1);
  }

  type = KII_OVERLAY_BOX;
  if ((N = get_argument (argc, argv, "-type"))) {
    remove_argument (N, &argc, argv);
    type = KiiOverlayTypeByName (argv[N]);
    remove_argument (N, &argc, argv);
    if (!type) {
      gprint (GP_ERR, "unknown Kii point type %s\n", argv[N]);
      return (FALSE);
    }
  }

  if (get_argument (argc, argv, "-size") && get_argument (argc, argv, "-dx")) {
      gprint (GP_ERR, "only specify one of -size, -dx, -dy\n");
      return (FALSE);
  }

  dx = dy = 1.0;
  if ((N = get_argument (argc, argv, "-size"))) {
    remove_argument (N, &argc, argv);
    dx = dy = fabs(atof (argv[N]));
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-dx"))) {
    remove_argument (N, &argc, argv);
    dx = fabs(atof (argv[N]));
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-dy"))) {
    remove_argument (N, &argc, argv);
    dy = fabs(atof (argv[N]));
    remove_argument (N, &argc, argv);
  }

  angle = 0.0;
  if ((N = get_argument (argc, argv, "-angle"))) {
    remove_argument (N, &argc, argv);
    angle = fabs(atof (argv[N]));
    remove_argument (N, &argc, argv);
  }

  if (argc != 4) {
    gprint (GP_ERR, "USAGE: vload (overlay) (xvec) (yvec) [-n] [-type] [-size] [-dx] [-dy]\n");
    return (FALSE);
  }
  
  if (type == KII_OVERLAY_CIRCLE) {
      dx /= 2.0;
      dy /= 2.0;
  }    

  if ((vecx = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((vecy = SelectVector (argv[3], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if (vecx[0].Nelements != vecy[0].Nelements) {
    gprint (GP_ERR, "mismatched vector lengths\n");
    return (FALSE);
  }

  Noverlay = vecx[0].Nelements;
  ALLOCATE (overlay, KiiOverlay, Noverlay);

  for (i = 0; i < Noverlay; i++) {
    overlay[i].type = type;
    overlay[i].text = NULL;
    overlay[i].x = (vecx[0].type == OPIHI_FLT) ? vecx[0].elements.Flt[i] : vecx[0].elements.Int[i];
    overlay[i].y = (vecy[0].type == OPIHI_FLT) ? vecy[0].elements.Flt[i] : vecy[0].elements.Int[i];
    overlay[i].x += 0.5;
    overlay[i].y += 0.5;
    overlay[i].dx = dx;
    overlay[i].dy = dy;
    overlay[i].angle = angle;
  }

  KiiLoadOverlay (kapa, overlay, Noverlay, argv[1]);
  free (overlay);
  return (TRUE);
}

