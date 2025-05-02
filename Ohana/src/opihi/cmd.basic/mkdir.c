# include "basic.h"

int mkdir_opihi (int argc, char **argv) {

  int mode, status;
  struct stat fstats;

  if (argc < 2) {
    gprint (GP_ERR, "USAGE: mkdir (path)\n");
    return (FALSE);
  }

  mode = S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH;

  status = stat (argv[1], &fstats);
  if (!status) {
    // argv[1] exists, is it a directory?
    if (!S_ISDIR(fstats.st_mode)) {
      gprint (GP_ERR, "cannot create directory %s: is an existing file\n", argv[1]);
      return (FALSE);
    }
    return (TRUE);
  }

  status = mkdirhier (argv[1], mode);
  if (status == -1) {
    gprint (GP_ERR, "cannot create directory %s\n", argv[1]);
    return (FALSE);
  }
  return (TRUE);
}

// XXX need to add mode option
// XXX need to respect umask (need umask command?)
