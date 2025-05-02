# include "addstar.h"

AddstarFile *LoadFilenames (int *nfile, char *filename, AddstarClientOptions *options) {

  int i, n, Nfile, NFILE;
  glob_t globList;
  AddstarFile *file;

  char *line, *word1, *word2, *sep;
  ALLOCATE (line, char, 4096);
  ALLOCATE (word1, char, 4096);
  ALLOCATE (word2, char, 4096);
  ALLOCATE (sep, char, 4096);

  if (options[0].filelist) {
    // read the list of input files from the supplied file
    FILE *f = fopen (filename, "r");
    if (f == NULL) {
      fprintf (stderr, "can't read input list %s, giving up\n", filename);
      exit (1);
    }

    Nfile = 0;
    NFILE = 10;
    ALLOCATE (file, AddstarFile, NFILE);
    for (i = 0; (scan_line (f, line) != EOF); i++) {
      // find first non-whitespace char & skip commented lines
      for (n = 0; OHANA_WHITESPACE (line[n]); n++);
      if (line[n] == '#') continue;
      if (line[n] == 0) continue;

      // file lines may have:
      // filename
      // filename = nebname
      // filename limited to 4096 chars
      int Nfield = sscanf (line, "%s %s %s", word1, sep, word2);
      if ((Nfield != 1) && (Nfield != 3)) {
	fprintf (stderr, "invalid line: %s\n", line);
	exit (3);
      }

      if (Nfield == 3) {
	if (strcmp(sep, "=") && strcmp (sep, ":")) {
	  fprintf (stderr, "ERROR: unexpected filename separator in list: %s\n", sep);
	  exit (3);
	}
	file[Nfile].filename  = strcreate (word1);
	file[Nfile].imagename = filebasename (word2);
      } else {
	file[Nfile].filename  = strcreate (word1);
	file[Nfile].imagename = filebasename (word1);
      }
      ohana_memcheck (TRUE);
      fprintf (stderr, "file: %s = %s\n", file[Nfile].filename, file[Nfile].imagename);
      Nfile ++;
      CHECK_REALLOCATE (file, AddstarFile, NFILE, Nfile, 10);
    }
    fclose (f);
  } else {
    // parse the filename as a glob
    globList.gl_offs = 0;
    glob (filename, 0, NULL, &globList);

    // if the glob does not match, save the literal word:
    // otherwise save all glob matches
    if (globList.gl_pathc == 0) {
      Nfile = 1;
      ALLOCATE (file, AddstarFile, Nfile);
      file->filename  = strcreate (filename);
      file->imagename = USE_NAME ? filebasename (USE_NAME) : filebasename (filename);
    } else {
      Nfile = globList.gl_pathc;
      if ((Nfile > 1) && USE_NAME) {
	fprintf (stderr, "ERROR: -use-name supplied but more than one file matches glob\n");
	exit (3);
      }
      ALLOCATE (file, AddstarFile, Nfile);
      for (i = 0; i < Nfile; i++) {
	file[i].filename  = strcreate (globList.gl_pathv[i]);
	file[i].imagename = USE_NAME ? filebasename (USE_NAME) : filebasename (file[i].filename);
      }
    }
  }
  *nfile = Nfile;

  free (line);
  free (word1);
  free (word2);
  free (sep);

  return (file);
}

void AddstarFileFree (AddstarFile *file, int Nfile) {

  if (!file) return;

  int i;
  for (i = 0; i < Nfile; i++) {
    free (file[i].filename);
    free (file[i].imagename);
  }
  FREE (file);
}

