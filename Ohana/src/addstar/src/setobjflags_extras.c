# include "addstar.h"
# include "setobjflags.h"

int setobjflags_tmpdir (void) {
  
  struct stat fstats;
  char fullpath[DVO_MAX_PATH];

  snprintf (fullpath, DVO_MAX_PATH, "%s/tmpdir", CATDIR);

  int mode = S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH;
  int status = stat (fullpath, &fstats);
  if (!status) {
    // path exists, is it a directory?
    if (!S_ISDIR(fstats.st_mode)) {
      gprint (GP_ERR, "cannot create directory %s: is an existing file\n", fullpath);
      return (FALSE);
    }
    return (TRUE);
  }

  status = mkdirhier (fullpath, mode);
  if (status == -1) {
    gprint (GP_ERR, "cannot create directory %s\n", fullpath);
    return (FALSE);
  }
  return (TRUE);
}


int setobjflags_sortStars (MyStars *stars, int Nstars) {

# define SWAPFUNC(A,B){ MyStars temp = stars[A]; stars[A] = stars[B]; stars[B] = temp; }
# define COMPARE(A,B)(stars[A].R < stars[B].R)

  OHANA_SORT (Nstars, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE
  
  return TRUE;
}
