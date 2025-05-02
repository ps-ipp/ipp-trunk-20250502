# include <kapa_internal.h>

KapaLineType KapaLineTypeFromString (char *string) {

  char *endptr;

  int iValue = strtol (string, &endptr, 10);
  if (*endptr == 0) {
    if ((iValue > KAPA_LINE_INVALID_MIN) && 
	(iValue < KAPA_LINE_INVALID_MAX)) {
      return iValue;
    }
    // silently return a valid value?
    return KAPA_LINE_SOLID;
  }

  if (!strcasecmp (string,  "solid"))     return KAPA_LINE_SOLID;
  if (!strcasecmp (string,  "dot"))       return KAPA_LINE_DOT;
  if (!strcasecmp (string,  "longdash"))  return KAPA_LINE_DASH_LONG;
  if (!strcasecmp (string,  "shortdash")) return KAPA_LINE_DASH_SHORT;
  if (!strcasecmp (string,  "dotdash"))   return KAPA_LINE_DOT_DASH;

  if (!strcasecmp (string,  "dash"))      return KAPA_LINE_DASH_LONG;
  if (!strcasecmp (string,  "dashdot"))   return KAPA_LINE_DOT_DASH;

  return KAPA_LINE_SOLID;
}

KapaPlotStyle KapaPlotStyleFromString (char *string) {

  char *endptr;

  int iValue = strtol (string, &endptr, 10);
  if (*endptr == 0) {
    if ((iValue > KAPA_PLOT_INVALID_MIN) && 
	(iValue < KAPA_PLOT_INVALID_MAX)) {
      return iValue;
    }
    // silently return a valid value?
    return KAPA_PLOT_POINTS;
  }

  if (!strcasecmp (string,  "points"))    return KAPA_PLOT_POINTS;
  if (!strcasecmp (string,  "pts"))       return KAPA_PLOT_POINTS;
  if (!strcasecmp (string,  "connect"))   return KAPA_PLOT_CONNECT;
  if (!strcasecmp (string,  "line"))      return KAPA_PLOT_CONNECT;
  if (!strcasecmp (string,  "histogram")) return KAPA_PLOT_HISTOGRAM;

  if (!strcasecmp (string,  "bars"))  return KAPA_PLOT_BARS_SOLID;
  if (!strcasecmp (string,  "obars")) return KAPA_PLOT_BARS_OUTLINE;
  if (!strcasecmp (string,  "fbars")) return KAPA_PLOT_BARS_OUTFILL;
  if (!strcasecmp (string,  "bars_outline")) return KAPA_PLOT_BARS_OUTLINE;
  if (!strcasecmp (string,  "bars_outfill")) return KAPA_PLOT_BARS_OUTFILL;
  if (!strcasecmp (string,  "outline")) return KAPA_PLOT_BARS_OUTLINE;
  if (!strcasecmp (string,  "outfill")) return KAPA_PLOT_BARS_OUTFILL;

  if (!strcasecmp (string,  "polygon"))   return KAPA_PLOT_POLYGON;
  if (!strcasecmp (string,  "polyfill"))  return KAPA_PLOT_POLYFILL;

  if (strlen(string) > 2) {
    if (!strncasecmp (string, "histogram", strlen(string))) return KAPA_PLOT_HISTOGRAM;
    if (!strncasecmp (string, "connect",   strlen(string))) return KAPA_PLOT_CONNECT;
    if (!strncasecmp (string, "points",    strlen(string))) return KAPA_PLOT_POINTS;
  }
  // silently return a valid value?
  return KAPA_PLOT_POINTS;
}

KapaPointStyle KapaPointStyleFromString (char *string) {

  char *endptr;

  int iValue = strtol (string, &endptr, 10);
  if (*endptr == 0) {
    // note that PAIR_CONNECT was historically 100 [outside MIN - MAX]
    if (iValue == KAPA_POINT_PAIR_CONNECT) return iValue;
    if ((iValue > KAPA_POINT_INVALID_MIN) && 
	(iValue < KAPA_POINT_INVALID_MAX)) {
      return iValue;
    }
    // silently return a valid value?
    return KAPA_POINT_BOX_SOLID;
  }

  if (!strcasecmp (string,  "box"))              return KAPA_POINT_BOX_SOLID;
  if (!strcasecmp (string,  "circle"))           return KAPA_POINT_CIRCLE_SOLID        ;
  if (!strcasecmp (string,  "cross"))            return KAPA_POINT_CROSS               ;
  if (!strcasecmp (string,  "fbox"))             return KAPA_POINT_BOX_SOLID;
  if (!strcasecmp (string,  "fcircle"))          return KAPA_POINT_CIRCLE_SOLID        ;
  if (!strcasecmp (string,  "fhexagon"))         return KAPA_POINT_HEXAGON             ;
  if (!strcasecmp (string,  "fpentagon"))        return KAPA_POINT_PENTAGON            ;
  if (!strcasecmp (string,  "ftriangle"))        return KAPA_POINT_TRIANGLE_SOLID      ;
  if (!strcasecmp (string,  "ftriangledown"))    return KAPA_POINT_TRIANGLE_SOLID_DOWN ;
  if (!strcasecmp (string,  "ftridown"))         return KAPA_POINT_TRIANGLE_SOLID_DOWN ;
  if (!strcasecmp (string,  "hexagon"))          return KAPA_POINT_HEXAGON             ;
  if (!strcasecmp (string,  "obox"))             return KAPA_POINT_BOX_OPEN;
  if (!strcasecmp (string,  "ocircle"))          return KAPA_POINT_CIRCLE_OPEN         ;
  if (!strcasecmp (string,  "openbox"))          return KAPA_POINT_BOX_OPEN            ;
  if (!strcasecmp (string,  "opencircle"))       return KAPA_POINT_CIRCLE_OPEN         ;
  if (!strcasecmp (string,  "opentriangle"))     return KAPA_POINT_TRIANGLE_OPEN       ;
  if (!strcasecmp (string,  "opentriangledown")) return KAPA_POINT_TRIANGLE_OPEN_DOWN  ;
  if (!strcasecmp (string,  "otriangle"))        return KAPA_POINT_TRIANGLE_OPEN       ;
  if (!strcasecmp (string,  "otriangledown"))    return KAPA_POINT_TRIANGLE_OPEN_DOWN  ;
  if (!strcasecmp (string,  "otridown"))         return KAPA_POINT_TRIANGLE_OPEN_DOWN  ;
  if (!strcasecmp (string,  "pairs"))            return KAPA_POINT_PAIR_CONNECT;
  if (!strcasecmp (string,  "pentagon"))         return KAPA_POINT_PENTAGON            ;
  if (!strcasecmp (string,  "plus"))             return KAPA_POINT_CROSS               ;
  if (!strcasecmp (string,  "solidtriangle"))    return KAPA_POINT_TRIANGLE_SOLID      ;
  if (!strcasecmp (string,  "triangle"))         return KAPA_POINT_TRIANGLE_SOLID      ;
  if (!strcasecmp (string,  "triangledown"))     return KAPA_POINT_TRIANGLE_SOLID_DOWN ;
  if (!strcasecmp (string,  "tridown"))          return KAPA_POINT_TRIANGLE_SOLID_DOWN ;
  if (!strcasecmp (string,  "x"))                return KAPA_POINT_X                   ;
  if (!strcasecmp (string,  "y"))                return KAPA_POINT_Y                   ;
  if (!strcasecmp (string,  "ydown"))            return KAPA_POINT_Y_DOWN              ;

  int Nchar = strlen(string);
  if (Nchar > 2) {
    if (!strncasecmp (string,  "box", Nchar))              return KAPA_POINT_BOX_SOLID           ;
    if (!strncasecmp (string,  "circle", Nchar))           return KAPA_POINT_CIRCLE_SOLID        ;
    if (!strncasecmp (string,  "cross", Nchar))            return KAPA_POINT_CROSS               ;
    if (!strncasecmp (string,  "fbox", Nchar))             return KAPA_POINT_BOX_SOLID;
    if (!strncasecmp (string,  "fcircle", Nchar))          return KAPA_POINT_CIRCLE_SOLID        ;
    if (!strncasecmp (string,  "fhexagon", Nchar))         return KAPA_POINT_HEXAGON             ;
    if (!strncasecmp (string,  "fpentagon", Nchar))        return KAPA_POINT_PENTAGON            ;
    if (!strncasecmp (string,  "ftriangle", Nchar))        return KAPA_POINT_TRIANGLE_SOLID      ;
    if (!strncasecmp (string,  "ftriangledown", Nchar))    return KAPA_POINT_TRIANGLE_SOLID_DOWN ;
    if (!strncasecmp (string,  "ftridown", Nchar))         return KAPA_POINT_TRIANGLE_SOLID_DOWN ;
    if (!strncasecmp (string,  "hexagon", Nchar))          return KAPA_POINT_HEXAGON             ;
    if (!strncasecmp (string,  "obox", Nchar))             return KAPA_POINT_BOX_OPEN;
    if (!strncasecmp (string,  "ocircle", Nchar))          return KAPA_POINT_CIRCLE_OPEN         ;
    if (!strncasecmp (string,  "openbox", Nchar))          return KAPA_POINT_BOX_OPEN            ;
    if (!strncasecmp (string,  "opencircle", Nchar))       return KAPA_POINT_CIRCLE_OPEN         ;
    if (!strncasecmp (string,  "opentriangle", Nchar))     return KAPA_POINT_TRIANGLE_OPEN       ;
    if (!strncasecmp (string,  "opentriangledown", Nchar)) return KAPA_POINT_TRIANGLE_OPEN_DOWN  ;
    if (!strncasecmp (string,  "opentridown", Nchar))      return KAPA_POINT_TRIANGLE_OPEN_DOWN  ;
    if (!strncasecmp (string,  "otriangle", Nchar))        return KAPA_POINT_TRIANGLE_OPEN       ;
    if (!strncasecmp (string,  "otriangledown", Nchar))    return KAPA_POINT_TRIANGLE_OPEN_DOWN  ;
    if (!strncasecmp (string,  "otridown", Nchar))         return KAPA_POINT_TRIANGLE_OPEN_DOWN  ;
    if (!strncasecmp (string,  "pairs", Nchar))            return KAPA_POINT_PAIR_CONNECT;
    if (!strncasecmp (string,  "pentagon", Nchar))         return KAPA_POINT_PENTAGON            ;
    if (!strncasecmp (string,  "plus", Nchar))             return KAPA_POINT_CROSS               ;
    if (!strncasecmp (string,  "solidtriangle", Nchar))    return KAPA_POINT_TRIANGLE_SOLID      ;
    if (!strncasecmp (string,  "triangle", Nchar))         return KAPA_POINT_TRIANGLE_SOLID      ;
    if (!strncasecmp (string,  "triangledown", Nchar))     return KAPA_POINT_TRIANGLE_SOLID_DOWN ;
    if (!strncasecmp (string,  "tridown", Nchar))          return KAPA_POINT_TRIANGLE_SOLID_DOWN ;
    if (!strncasecmp (string,  "ydown", Nchar))            return KAPA_POINT_Y_DOWN              ;
  }

  return KAPA_POINT_BOX_SOLID;
}

