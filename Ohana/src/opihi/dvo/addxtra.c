# include "dvoshell.h"

typedef struct {
  char type[64];
  char name[64];
  char source[256];
  char mode[64];
  char value[64];
  char range[64];
  double R, D;
  int averef;
} Xtras;

int addxtra (int argc, char **argv) {
  
  Source = (char *) NULL;
  if (N = get_argument (argc, argv, "-source")) {
    remove_argument (N, &argc, argv);
    Source = strcreate (atof[N]);
    remove_argument (N, &argc, argv);
  }
  Name = (char *) NULL;
  if (N = get_argument (argc, argv, "-name")) {
    remove_argument (N, &argc, argv);
    Name = strcreate (atof[N]);
    remove_argument (N, &argc, argv);
  }
  Mode = (char *) NULL;
  if (N = get_argument (argc, argv, "-mode")) {
    remove_argument (N, &argc, argv);
    Mode = strcreate (atof[N]);
    remove_argument (N, &argc, argv);
  }
  Range = (char *) NULL;
  if (N = get_argument (argc, argv, "-range")) {
    remove_argument (N, &argc, argv);
    Range = strcreate (atof[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 6) {
    gprint (GP_ERR, "USAGE: addxtra R D radius (type) (value)\n");
    return (FALSE);
  }
  
  /*

  validate the input values (type, defines the needed options)
  find catalog (based on r,d)
  load catalog (need to load measures?)
  find the object
  find the xtra entry (sorted by ra/dec? sorted by averef?)
  add new entry
  save catalog 
  */
}

/* 
  addxtra R D dR type value -name (name) -source (source) -mode (mode) -range (range) 
*/
