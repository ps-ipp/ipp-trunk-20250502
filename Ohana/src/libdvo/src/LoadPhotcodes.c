# include <dvo.h>

int LoadPhotcodes (char *catdir_file, char *master_file, int readwrite) {

  /* first try to load the photcodes from the specified CATDIR location */
  if (LoadPhotcodesFITS (catdir_file)) return TRUE;
  
  if (!readwrite) {
    fprintf (stderr, "db is missing a photcode table & access is read-only -- create one with photcode-table -import\n");
    return FALSE;
  }

  if (!master_file) return FALSE;

  /* next try to load the photcodes from the master text photcode file */
  /* automatically (or on demand?) save the text file to the FITS version */
  if (LoadPhotcodesText (master_file)) { 
    if (!check_file_access (catdir_file, TRUE, TRUE, TRUE)) return TRUE;
    if (!SavePhotcodesFITS (catdir_file)) return FALSE;
    return TRUE;
  }

  return FALSE;
}
