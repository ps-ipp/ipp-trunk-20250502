# include <dvo.h>

char typename[N_TYPE][32] = {"none", "object", "dark", "bias", "flat", "mask", "fringe", "scatter", "modes", "frpts", "any"};
char typecode[N_TYPE]     = {'x',    'o',      'd',    'b',    'f',    'm',    'r',      's',       'M',     'F'};
char modename[N_MODE][32] = {"none", "MEF", "SPLIT", "SINGLE", "CUBE", "SLICE", "MODES"};

int get_image_type (char *name) {

  int i;
  
  for (i = 0; i < N_TYPE; i++) {
    if (!strncasecmp (name, typename[i], strlen(typename[i]))) {
      return (i);
    }
  }

  /* special cases */
  if (!strncasecmp (name, "SKYFLAT", strlen("SKYFLAT"))) {
    return (T_FLAT);
  }

  return (T_UNDEF);
}

char *get_type_name (int type) {

  if (type < T_NONE) return ("undef");
  if (type >= N_TYPE) return ("undef");
  return (typename[type]);
}

int get_image_mode (char *name) {

  int i;
  
  for (i = 0; i < N_MODE; i++) {
    if (!strncasecmp (name, modename[i], strlen(modename[i]))) {
      return (i);
    }
  }
  return (M_UNDEF);
}

char *get_mode_name (int mode) {

  if (mode < M_NONE) return ("undef");
  if (mode >= N_MODE) return ("undef");
  return (modename[mode]);
}

