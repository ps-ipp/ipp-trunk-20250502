# include "dvoshell.h"

int getxtra (int argc, char **argv) {
  
  SelectXtra = (char *) NULL;
  if (N = get_argument (argc, argv, "-xtra")) {
    SelectXtra = TRUE;
    remove_argument (N, &argc, argv);
    XtraType = strcreate (atof[N]);
    remove_argument (N, &argc, argv);
    XtraValue = strcreate (atof[N]);
    remove_argument (N, &argc, argv);
  }
  SelectRegion = (char *) NULL;
  if (N = get_argument (argc, argv, "-region")) {
    SelectRegion = TRUE;
    remove_argument (N, &argc, argv);
    RA = atof (atof[N]);
    remove_argument (N, &argc, argv);
    Dec = atof (atof[N]);
    remove_argument (N, &argc, argv);
    Radius = atof (atof[N]);
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
   get specified values for specified objects 

   getxtra type -xtra name K01.01 
   getxtra type -region R D radius 

*/
