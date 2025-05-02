# include "data.h"

int vzload (int argc, char **argv) {
  
  int i, N, Noverlay;
  int kapa, type;
  char *name;
  double size, min, range, MAX_OUTPUT_SIZE;
  KiiOverlay *overlay;
  Vector *vecx, *vecy, *vecz;
  
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

  MAX_OUTPUT_SIZE = 10.0;
  if ((N = get_argument (argc, argv, "-max"))) {
    remove_argument (N, &argc, argv);
    MAX_OUTPUT_SIZE = atof (argv[N]);
    remove_argument (N, &argc, argv);
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

  if (argc != 7) {
    gprint (GP_ERR, "USAGE: vzload (overlay) (xvec) (yvec) (zvec) (min) (max) [-n] [-type]\n");
    return (FALSE);
  }
  
  if ((vecx = SelectVector (argv[2], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((vecy = SelectVector (argv[3], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if ((vecz = SelectVector (argv[4], OLDVECTOR, TRUE)) == NULL) return (FALSE);
  if (vecx[0].Nelements != vecy[0].Nelements) {
    gprint (GP_ERR, "mismatched vector lengths\n");
    return (FALSE);
  }

  min = atof(argv[5]);
  range = (atof(argv[6]) - min) / MAX_OUTPUT_SIZE;
  // renormalize to the max output size (output range is 0.1 - MAX_OUTPUT_SIZE)

  Noverlay = vecx[0].Nelements;
  ALLOCATE (overlay, KiiOverlay, Noverlay);

  for (i = N = 0; i < Noverlay; i++) {
    float z = (vecz[0].type == OPIHI_FLT) ? vecz[0].elements.Flt[i] : vecz[0].elements.Int[i];
    size = MIN (MAX_OUTPUT_SIZE, (z - min) / range);
    if (size < 0.1) continue;

    overlay[N].type = type;
    overlay[N].text = NULL;
    overlay[N].x = (vecx[0].type == OPIHI_FLT) ? vecx[0].elements.Flt[i] : vecx[0].elements.Int[i];
    overlay[N].y = (vecy[0].type == OPIHI_FLT) ? vecy[0].elements.Flt[i] : vecy[0].elements.Int[i];

    overlay[N].x += 0.5;
    overlay[N].y += 0.5;

    overlay[N].angle = 0.0;

    if (type == KII_OVERLAY_CIRCLE) {
      overlay[N].dx = size / 2.0;
      overlay[N].dy = size / 2.0;
    } else {
      overlay[N].dx = size;
      overlay[N].dy = size;
    }    
    N++;
  }

  KiiLoadOverlay (kapa, overlay, N, argv[1]);
  free (overlay);
  return (TRUE);
}

