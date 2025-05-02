# include "dvomerge.h"

int main (int argc, char **argv) {

  SetSignals ();
  dvomerge_client_help (argc, argv);
  ConfigInit (&argc, argv);
  dvomerge_client_args (&argc, argv);

  SkyTable *outsky, *insky;
  SkyList *inlist;
  char filename[256], *input, *output;
  IDmapType *IDmap = NULL;
  PhotCodeData *inputPhotcodes;
  PhotCodeData *outputPhotcodes;
  int *secfiltMap = NULL;
  int NsecfiltInput, NsecfiltOutput;

  input  = argv[1];
  output = argv[3];

  // we need input and output photcode tables to correctly match secfilt entries

  // load the input photcodes
  SetPhotcodeTable(NULL);
  sprintf (filename, "%s/Photcodes.dat", input);
  if (!LoadPhotcodes (filename, NULL, FALSE)) {
    fprintf (stderr, "error reading input database directory %s\n", input);
    exit (1);
  }	
  inputPhotcodes = GetPhotcodeTable();
  NsecfiltInput = GetPhotcodeNsecfilt();

  // since we are merging the input db into the output db, the output defines the photcode
  // table & db layout but, this requires the output to exist.  if it does not, fail
  // (master should have created the output photcode table)

  // load the output photcodes
  SetPhotcodeTable(NULL);
  sprintf (filename, "%s/Photcodes.dat", output);
  if (!LoadPhotcodes (filename, NULL, FALSE)) {
    fprintf (stderr, "error reading output database directory %s\n", output);
    exit (1);
  }
  outputPhotcodes = GetPhotcodeTable();
  NsecfiltOutput = GetPhotcodeNsecfilt();

  secfiltMap = GetSecFiltMap(outputPhotcodes, inputPhotcodes);
  if (!secfiltMap) {
    fprintf (stderr, "failed to map input secfilt photcodes to output photcodes table\n");
    exit (1);
  }

  char *absoutput = abspath (output, DVO_MAX_PATH);
  char IDmapFilename[DVO_MAX_PATH];
  snprintf (IDmapFilename, DVO_MAX_PATH, "%s/IDmap.fits", absoutput);

  if (!VERIFY_CATALOG_ONLY) {
    // save IDmap information
    IDmap = IDmapLoad (IDmapFilename);
    if (!IDmap) {
      fprintf (stderr, "ERROR: failure to save the image ID map\n");
      exit (1);
    }
  }

  // load the sky table for the existing database
  insky = SkyTableLoadOptimal (input, NULL, NULL, FALSE, SKY_DEPTH_HST, VERBOSE);
  if (!insky) {
      Shutdown ("can't read SkyTable for %s", input);
  }
  SkyTableSetFilenames (insky, input, "cpt");

  // XXX apply this...generate the subset matching the user-selected region
  inlist = SkyListByPatch (insky, -1, &UserPatch);

  // modify the list if we are restricting:
  if (CPTLIST) {
    inlist = SkyListMatchList (inlist, CPTLIST, NCPTLIST);
  }

  // generate an output table populated at the desired depth
  outsky = SkyTableLoadOptimal (output, NULL, GSCFILE, TRUE, SKY_DEPTH, VERBOSE);
  if (!outsky) {
      Shutdown ("can't read or create SkyTable for %s", output);
  }
  SkyTableSetFilenames (outsky, output, "cpt");

  // loop over the populatable output tables; check for data in input in the corresponding regions

  if (REPLACE_TYCHO) replace_tycho_init();

  dvomergeUpdate_catalogs (input, output, inlist, outsky, NsecfiltInput, NsecfiltOutput, IDmap, secfiltMap);

  SkyTableFree (insky);
  SkyTableFree (outsky);
  SkyListFree (inlist);

  if (IDmap) {
    dvo_image_map_free (IDmap);
    free (IDmap);
  }
  free (absoutput);
  free (secfiltMap);
  FreePhotcodeData (inputPhotcodes);
  FreePhotcodeData (outputPhotcodes);

  SetPhotcodeTable(NULL);
  FreePhotcodeTable();

  dvomerge_client_args_free ();
  ohana_memcheck (TRUE);
  // ohana_memdump (TRUE);

  exit (0);
}
