# include "addstar.h"
# include "loadstarpar.h"

int loadstarpar_tmpdir () {
  
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

