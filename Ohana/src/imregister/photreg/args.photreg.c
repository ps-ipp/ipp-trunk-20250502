# include "imregister.h"
# include "photreg.h"

void usage ();

int regargs (int argc, char **argv, PhotPars *newdata) {

  int *list, Nlist;
  int N, Ntimes;
  time_t *tstart, *tstop;
  PhotCode *photcode, *depcode;

  ConfigInit (&argc, argv);

  photcode = NULL;
  output.modify = TRUE;
  bzero (newdata, sizeof(PhotPars));

  if (get_argument (argc, argv, "-h")) usage ();
  if (get_argument (argc, argv, "--help")) usage ();

  /* set the required database */
  output.db = strcreate ("phot");
  if ((N = get_argument (argc, argv, "-trans"))) {
    remove_argument (N, &argc, argv);
    output.db = strcreate ("trans");
  }

  /*** optional fields ***/
  strcpy (newdata[0].label, "elixir");
  if ((N = get_argument (argc, argv, "-label"))) {
    remove_argument (N, &argc, argv);
    strcpy (newdata[0].label, argv[N]);
    remove_argument (N, &argc, argv);
  }

  newdata[0].Nmeas = 1;
  if ((N = get_argument (argc, argv, "-Nmeas"))) {
    remove_argument (N, &argc, argv);
    newdata[0].Nmeas = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }

  newdata[0].Ntime = 1;
  if ((N = get_argument (argc, argv, "-Ntime"))) {
    remove_argument (N, &argc, argv);
    newdata[0].Ntime = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }

  output.offset = FALSE;
  if ((N = get_argument (argc, argv, "-offset"))) {
    remove_argument (N, &argc, argv);
    output.offset = TRUE;
  }

  /**** required arguments ****/
  if (!get_trange_arguments (&argc, argv, &tstart, &tstop, &Ntimes)) usage ();
  if (Ntimes != 1) usage ();
  newdata[0].tstart = tstart[0];
  newdata[0].tstop = tstop[0];

  if (!get_argument (argc, argv, "-zp")) goto required;
  if (!get_argument (argc, argv, "-dzp")) goto required;
  if (!get_argument (argc, argv, "-photcode")) goto required;

  /* observed zero point */
  if ((N = get_argument (argc, argv, "-zp"))) {
    remove_argument (N, &argc, argv);
    newdata[0].ZP = atof (argv[N]);
    remove_argument (N, &argc, argv);
  } 

  /* error on observed zero point */
  if ((N = get_argument (argc, argv, "-dzp"))) {
    remove_argument (N, &argc, argv);
    newdata[0].dZP = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  /* photcode system */
  if ((N = get_argument (argc, argv, "-photcode"))) {
    remove_argument (N, &argc, argv);
    if ((photcode = GetPhotcodebyName (argv[N])) == NULL) {
      fprintf (stderr, "ERROR: photcode not found in photcode table\n");
      exit (1);
    }
    remove_argument (N, &argc, argv);
  }

  /* remainding newdata values can be set once photcode is known 
     be careful about consistency for offset definition:
     dM == ZPi - ZPo  (ZPo = nominal zero point, ZPi = specific observed zero point)
     ie, clouds = -dM (ZPi < ZPo)
  */
  
  /* find first (!) dep photcode which is equivalent to this code */
  list = GetPhotcodeEquivList (photcode[0].code, &Nlist);
  depcode = GetPhotcodebyCode (list[0]);

  newdata[0].photcode = photcode[0].code;
  newdata[0].refcode  = photcode[0].equiv;
  newdata[0].X        = photcode[0].X[0];
  newdata[0].c1       = photcode[0].c1;
  newdata[0].c2       = photcode[0].c2;
  
  newdata[0].K        = depcode[0].K;
  newdata[0].ZPo      = 0.001*photcode[0].C + 0.001*depcode[0].C;
  if (output.offset) newdata[0].ZP += newdata[0].ZPo;
  return (TRUE);

required:
  fprintf (stderr, "missing required fields\n");
  usage ();
  return (FALSE);
}

/* differences between phot.db & trans.db:

   Ntime : photreg = 1 : transreg = N
   dBFile: phot.db     : trans.db
   ASCII EXTNAME: IMAGE_ZPTS : SUMMARY_ZPTS
   BINARY EXTNAME: ZERO_POINTS_3.0 : TRANS_POINTS_3.0
   
   photreg -photcode B -zp 26.1 -dzp 0.02 -date 2003/1/1,10:00:00 -Nmeas 5 -Ntime 1 -db trans
   photreg -photcode B -zp 26.1 -dzp 0.02 -date 2003/1/1,10:00:00 -Nmeas 5 -Ntime 1 -db trans

   photsearch -db trans
   photsearch -db phot
   
*/

void usage () {  
  fprintf (stderr, "USAGE: photreg (-zp zp) (-dzp dzp) (-trange start range|end) (-photcode code)\n");
  fprintf (stderr, "       [-label label] [-offset] [-Nmeas N] [-Ntime N] [-db trans]\n");
  exit (1);
}
