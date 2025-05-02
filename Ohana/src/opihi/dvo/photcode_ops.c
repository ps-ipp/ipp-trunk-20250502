# include "dvoshell.h"

/* (re)load photcodes from photcode table */
int InitPhotcodes () {

  double ZERO_POINT;
  char MasterPhotcodeFile[256];
  char CatdirPhotcodeFile[256];
  char *catdir;

  if (VarConfig ("ZERO_PT", "%lf", &ZERO_POINT) == (char *) NULL) {
    gprint (GP_ERR, "ZERO_PT undefined in config\n");
    return (FALSE);
  }
  SetZeroPoint (ZERO_POINT);

  catdir = GetCATDIR();
  if (catdir == NULL) {
    CatdirPhotcodeFile[0] = 0;
  } else {
    sprintf (CatdirPhotcodeFile, "%s/Photcodes.dat", catdir);
  }

  if (VarConfig ("PHOTCODE_FILE", "%s", MasterPhotcodeFile) == (char *) NULL) {
    gprint (GP_ERR, "PHOTCODE_FILE undefined in config\n");
    return (FALSE);
  }

  // XXX now that DVO does not allow write access, we can drop the MasterPhotcodeFile
  if (!LoadPhotcodes (CatdirPhotcodeFile, MasterPhotcodeFile, FALSE)) {
    gprint (GP_ERR, "error loading photcode table %s or master file %s\n", CatdirPhotcodeFile, MasterPhotcodeFile);
    return (FALSE);
  }
  return (TRUE);
}
