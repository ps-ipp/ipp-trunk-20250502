# include "astro.h"

int csystem (int argc, char **argv) {

  /* USAGE: csystem [C/G/E/H] [C/G/E/H] [epoch] */
  int i, N;
  double X, Y, x, y, uX, uY, ux, uy;
  CoordTransformSystem input, output;

  Vector *xvec = NULL;
  Vector *yvec = NULL;
  Vector *uxvec = NULL;
  Vector *uyvec = NULL;

  int Quiet = FALSE;
  if ((N = get_argument (argc, argv, "-q"))) {
    Quiet = TRUE;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-quiet"))) {
    Quiet = TRUE;
    remove_argument (N, &argc, argv);
  }

  // transform proper motions at the same time
  char *uXname = NULL;
  char *uYname = NULL;
  int pmBackwards = FALSE;
  if ((N = get_argument (argc, argv, "-pm"))) {
    remove_argument (N, &argc, argv);
    uXname = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
    uYname = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  } 
  if ((N = get_argument (argc, argv, "-pmback"))) {
    if (uXname) {
      gprint (GP_ERR, "cannot mix -pm and -pmback\n");
      return FALSE;
    }
    remove_argument (N, &argc, argv);
    uXname = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
    uYname = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
    pmBackwards = TRUE;
  } 

  if (argc != 5) goto syntax;

  if (!strcmp(argv[1], "G2004")) {
    input = COORD_GALACTIC_REID_2004;
  } else {
    switch (argv[1][0]) {
      case 'C': input = COORD_CELESTIAL; break;
      case 'G': input = COORD_GALACTIC; break;
      case 'E': input = COORD_ECLIPTIC; break;
      default: goto syntax;
    }
  }

  if (!strcmp(argv[2], "G2004")) {
    output = COORD_GALACTIC_REID_2004;
  } else {
    switch (argv[2][0]) {
      case 'C': output = COORD_CELESTIAL; break;
      case 'G': output = COORD_GALACTIC; break;
      case 'E': output = COORD_ECLIPTIC; break;
      default: goto syntax;
    }
  }

  CoordTransform *transform = InitTransform (input, output);
  if (transform == NULL) {
    gprint (GP_ERR, "transform %c to %c is not yet defined\n", argv[1][0], argv[2][0]);
    return (FALSE);
  }
    
  if (SelectScalar (argv[3], &X)) {
    if (!SelectScalar (argv[4], &Y)) goto escape;
      
    // apply transform at the current coordinate to transform motions:
    if (uXname) {
      if (!SelectScalar (uXname, &uX)) goto escape;
      if (!SelectScalar (uYname, &uY)) goto escape;

      if (pmBackwards) {
	TransformProperMotionBackwards (&ux, &uy, uX, uY, X, Y, transform);
      } else {
	TransformProperMotionForewards (&ux, &uy, uX, uY, X, Y, transform);
      }
    }
    ApplyTransform (&x, &y, X, Y, transform);

    if (uXname) {
      if (!Quiet) {
	gprint (GP_LOG, "%10.6f %10.6f @ %10.6f %10.6f\n", x, y, ux, uy);
	switch (output) {
	  case COORD_CELESTIAL:
	    set_variable ("uR", ux);
	    set_variable ("uD", uy);
	    break;
	  case COORD_ECLIPTIC:
	    set_variable ("uL", ux);
	    set_variable ("uB", uy);
	    break;
	  case COORD_GALACTIC:
	  case COORD_GALACTIC_REID_2004:
	    set_variable ("uLon", ux);
	    set_variable ("uLat", uy);
	    break;
	  default:
	    break;
	}
      }
    } else {
      if (!Quiet) {
	gprint (GP_LOG, "%10.6f %10.6f\n", x, y);
      }
    }
    switch (output) {
      case COORD_CELESTIAL:
	set_variable ("RA", x);
	set_variable ("DEC", y);
	break;
      case COORD_ECLIPTIC:
	set_variable ("Lambda", x);
	set_variable ("Beta", y);
	break;
      case COORD_GALACTIC:
      case COORD_GALACTIC_REID_2004:
	set_variable ("gLon", x);
	set_variable ("gLat", y);
	break;
      default:
	break;
    }
    free (transform);
    return (TRUE);
  }

  /* find vectors */
  if ((xvec = SelectVector (argv[3], OLDVECTOR, TRUE)) == NULL) goto escape;
  if ((yvec = SelectVector (argv[4], OLDVECTOR, TRUE)) == NULL) goto escape;

  if (xvec[0].Nelements != yvec[0].Nelements) {
    gprint (GP_ERR, "vectors %s and %s not the same length\n", argv[3], argv[4]);
    goto escape;
  }
  
  // apply transform at the current coordinate to transform motions:
  if (uXname) {
    if ((uxvec = SelectVector (uXname, OLDVECTOR, TRUE)) == NULL) goto escape;
    if ((uyvec = SelectVector (uYname, OLDVECTOR, TRUE)) == NULL) goto escape;
    
    if (xvec[0].Nelements != uxvec[0].Nelements) {
      gprint (GP_ERR, "vectors %s and %s not the same length\n", argv[3], uXname);
      goto escape;
    }
    if (xvec[0].Nelements != uyvec[0].Nelements) {
      gprint (GP_ERR, "vectors %s and %s not the same length\n", argv[3], uYname);
      goto escape;
    }

    CastVector (uxvec, OPIHI_FLT);
    CastVector (uyvec, OPIHI_FLT);
  }

  CastVector (xvec, OPIHI_FLT);
  CastVector (yvec, OPIHI_FLT);

  opihi_flt *xptr = xvec[0].elements.Flt;
  opihi_flt *yptr = yvec[0].elements.Flt;

  opihi_flt *uxptr = uxvec ? uxvec[0].elements.Flt : NULL;
  opihi_flt *uyptr = uyvec ? uyvec[0].elements.Flt : NULL;

  for (i = 0; i < xvec[0].Nelements; i++, xptr++, yptr++) {
    if (uXname) {
      if (pmBackwards) {
	TransformProperMotionBackwards (uxptr, uyptr, *uxptr, *uyptr, *xptr, *yptr, transform);
      } else {
	TransformProperMotionForewards (uxptr, uyptr, *uxptr, *uyptr, *xptr, *yptr, transform);
      }
      uxptr ++;
      uyptr ++;
    }
    ApplyTransform (xptr, yptr, *xptr, *yptr, transform);
  }

  FREE (transform);
  return (TRUE);

escape:
  FREE (transform);
  return FALSE;

syntax:
  gprint (GP_ERR, "USAGE   : csystems [C/G/E/H] [C/G/E/H] X Y [-pm uX uY] [-pmback ux uy]\n");
  gprint (GP_ERR, " -pm    : transform motions in source system to target system\n");
  gprint (GP_ERR, " -pmback: transform motions in target system to source system\n");
  gprint (GP_ERR, " e.g.   : csystem C G R D -pm uR uD -> R D will contain glon, glat; uR, uD will contain uL, uB\n");
  gprint (GP_ERR, " e.g.   : csystem C G R D -pmback uL uB -> R D will contain glon, glat; uL, uB will contain uR, uD\n");
  gprint (GP_ERR, "        :  if input values are vectors, their values are replaced with the transformed values\n");
  gprint (GP_ERR, "        :  if input values are scalars, the transformed coordinates are printed (disable with -q)\n");
  gprint (GP_ERR, "        :    and the following output variables will be set (names depend on output system)\n");
  gprint (GP_ERR, "        :  output positions (RA, DEC), (Lambda, Beta), (gLon, gLat)\n");
  gprint (GP_ERR, "        :  output motions   (uR, uD),  (uL, uB),       (uLon, uLat)\n");
  return FALSE;
}
