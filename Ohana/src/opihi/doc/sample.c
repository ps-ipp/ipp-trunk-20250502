# include <stdio.h>

main (int argc, char **argv) {

  int i;
  char line[1024];

  for (i = 0; i <= argc; i++) {
    gprint (GP_ERR, "arg %2d: %s\n", i, argv[i]);
  }
  while (argv[i] != NULL) {
    gprint (GP_ERR, "env %2d: %s\n", i, argv[i]);
    i++;
  } 
  while (fscanf (stdin, "%s", line) != EOF) {
    gprint (GP_ERR, "line: ...%s...\n", line);
  }
}
