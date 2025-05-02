# include "basic.h"

int file (int argc, char **argv) {
  
  /* usage: file (filename) [var] */

  int status, vstat, N;
  struct stat fstats;

  if ((N = get_argument (argc, argv, "-h"))) goto usage;
  if ((N = get_argument (argc, argv, "-help"))) goto usage;

  int CAN_READ = FALSE;
  if ((N = get_argument (argc, argv, "-can-read"))) {
    remove_argument (N, &argc, argv);
    CAN_READ = TRUE;
  }
  int CAN_WRITE = FALSE;
  if ((N = get_argument (argc, argv, "-can-write"))) {
    if (CAN_READ) goto badopts;
    remove_argument (N, &argc, argv);
    CAN_WRITE = TRUE;
  }
  int GET_STATS = FALSE;
  if ((N = get_argument (argc, argv, "-stats"))) {
    if (CAN_READ || CAN_WRITE) goto badopts;
    remove_argument (N, &argc, argv);
    GET_STATS = TRUE;
  }

  if ((argc != 2) && (argc != 3)) goto usage;

  if (CAN_READ) {
    status = !access (argv[1], F_OK | R_OK);
    if (argc == 3) {
      set_int_variable (argv[2], status);
    } else {
      gprint (GP_LOG, "file %s can", argv[1]);
      if (!status) gprint (GP_LOG, "not");
      gprint (GP_LOG, " be read\n");
    }
    return TRUE;
  }
  if (CAN_WRITE) {
    status = !access (argv[1], F_OK | R_OK | W_OK);
    if (argc == 3) {
      set_int_variable (argv[2], status);
    } else {
      gprint (GP_LOG, "file %s can", argv[1]);
      if (!status) gprint (GP_LOG, "not");
      gprint (GP_LOG, " be written\n");
    }
    return TRUE;
  }

  status = stat (argv[1], &fstats);

  if (!GET_STATS) {
    vstat = !status;

    if (argc == 3) {
      set_int_variable (argv[2], vstat);
    } else {
      gprint (GP_LOG, "file %s is ", argv[1]);
      if (!vstat) gprint (GP_LOG, "not ");
      gprint (GP_LOG, "found\n");
    }
    return (TRUE);
  }

  int TimeFormat;
  time_t TimeReference;

  // use the variable to set the stats fields
  GetTimeFormat (&TimeReference, &TimeFormat);

  if (argc == 3) {
    char varname[1024];

    snprintf (varname, 1024, "%s:devID",   argv[2]); set_int_variable (varname, fstats.st_dev);
    snprintf (varname, 1024, "%s:inode",   argv[2]); set_int_variable (varname, fstats.st_ino);
    snprintf (varname, 1024, "%s:mode",    argv[2]); set_int_variable (varname, fstats.st_mode);
    snprintf (varname, 1024, "%s:nlink",   argv[2]); set_int_variable (varname, fstats.st_nlink);
    snprintf (varname, 1024, "%s:uid",     argv[2]); set_int_variable (varname, fstats.st_uid);
    snprintf (varname, 1024, "%s:gid",     argv[2]); set_int_variable (varname, fstats.st_gid);
    snprintf (varname, 1024, "%s:rdev",    argv[2]); set_int_variable (varname, fstats.st_rdev);
    snprintf (varname, 1024, "%s:size",    argv[2]); set_int_variable (varname, fstats.st_size);
    snprintf (varname, 1024, "%s:blksize", argv[2]); set_int_variable (varname, fstats.st_blksize);
    snprintf (varname, 1024, "%s:blocks",  argv[2]); set_int_variable (varname, fstats.st_blocks);

    snprintf (varname, 1024, "%s:atime", argv[2]); set_variable (varname, TimeValue (fstats.st_atim.tv_sec, TimeReference, TimeFormat));
    snprintf (varname, 1024, "%s:mtime", argv[2]); set_variable (varname, TimeValue (fstats.st_mtim.tv_sec, TimeReference, TimeFormat));
    snprintf (varname, 1024, "%s:ctime", argv[2]); set_variable (varname, TimeValue (fstats.st_ctim.tv_sec, TimeReference, TimeFormat));
  } else {
    gprint (GP_LOG, "devID   : %d\n", (int) fstats.st_dev);
    gprint (GP_LOG, "inode   : %d\n", (int) fstats.st_ino);
    gprint (GP_LOG, "mode    : %o\n", (int) fstats.st_mode);
    gprint (GP_LOG, "nlink   : %d\n", (int) fstats.st_nlink);
    gprint (GP_LOG, "uid     : %d\n", (int) fstats.st_uid);
    gprint (GP_LOG, "gid     : %d\n", (int) fstats.st_gid);
    gprint (GP_LOG, "rdev    : %d\n", (int) fstats.st_rdev);
    gprint (GP_LOG, "size    : %d\n", (int) fstats.st_size);
    gprint (GP_LOG, "blksize : %d\n", (int) fstats.st_blksize);
    gprint (GP_LOG, "blocks  : %d\n", (int) fstats.st_blocks);

    char *adate = ohana_sec_to_date (fstats.st_atim.tv_sec);
    gprint (GP_LOG, "atime   : %s\n", adate);
    free (adate);

    char *mdate = ohana_sec_to_date (fstats.st_mtim.tv_sec);
    gprint (GP_LOG, "mtime   : %s\n", mdate);
    free (mdate);

    char *cdate = ohana_sec_to_date (fstats.st_ctim.tv_sec);
    gprint (GP_LOG, "ctime   : %s\n", cdate);
    free (cdate);
  }
  return TRUE;

badopts:
gprint (GP_ERR, "  NOTE: only one (or none) of -can-read, -can-write, -stats is allowed\n");

 usage:
  gprint (GP_ERR, "USAGE: file [var] [-can-read] [-can-write] [-stats]\n");
  gprint (GP_ERR, "  by default, reports if file exists\n");
  gprint (GP_ERR, "  -can-read: reports if file exists and can be read\n");
  gprint (GP_ERR, "  -can-write: reports if file exists and can be written to\n");
  gprint (GP_ERR, "  -stats: reports if file exists and can be written to\n");
  return FALSE;
}

