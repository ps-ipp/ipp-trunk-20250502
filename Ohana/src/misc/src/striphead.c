# include <ohana.h>

main (argc, argv) 
int argc;
char *argv[];
{

  int i, j, status, N, FORCE;
  char head[1000];
  Header header;
  char *p;
  FILE *f;

  if (get_argument (argc, argv, "-help") || get_argument (argc, argv, "-h")) {
    fprintf (stderr, "USAGE: %s [-f] (filenames)\n", argv[0]);
    exit (0);
  }

  if (N = get_argument (argc, argv, "-f")) {
    remove_argument (N, &argc, argv);
    FORCE = TRUE;
  }
  else 
    FORCE = FALSE;
  
  if (argc == 1) {
    fprintf (stderr, "USAGE: %s [-f] (filenames)\n", argv[0]);
    exit (0);
  }

  for (i = 1; i < argc; i++) {
    
    strcpy (head, argv[i]);
    if (strrchr(head, '.') !=NULL)
      strcpy(strrchr(head, '.'), ".head");
    else 
      strcat(head, ".head");
    
    if (!FORCE) {
      f = fopen (head, "r");
      if (f != NULL) {
	fprintf (stderr, "header file exists for %s, skipping\n", argv[i]);
	fclose (f);
	continue;
      }
    }
    
    if (!gfits_read_header (argv[i], &header)) {
      fprintf (stderr, "failed to open file %s\n", argv[i]);
      continue;
    }
    
    for (j = 79; j < header.datasize; j+= 80) {
      header.buffer[j] = 10;
    }

    if (!gfits_write_header (head, &header)) {
      fprintf (stderr, "failed to write file %s\n", head);
      continue;
    }
    
    gfits_free_header (&header);
  }
}
