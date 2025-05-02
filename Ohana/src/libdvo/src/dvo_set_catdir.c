# include "ohana.h"

// functions to set and get the current catdir.
static char *current_catdir = NULL;

char * dvo_get_catdir() {

  return current_catdir;
}

void dvo_set_catdir (char *catdir) {
    if (current_catdir) {
        free(current_catdir);
        current_catdir = NULL;
    }
    if (catdir) {
	// need to keep this in the ohana memory system, so we cannot use strdup
        current_catdir = strcreate(catdir);
    }
}
