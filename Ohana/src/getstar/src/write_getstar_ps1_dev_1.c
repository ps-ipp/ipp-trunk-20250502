# include "getstar.h"

// convert the average/secfilt values to getstar format 
// PS1_DEV_1 has no errors, only positions, motions and magnitudes
int write_getstar_PS1_DEV_1 (Catalog *catalog) {    

  int i, m, offset;
  int Nsec_c0, Nsec_c1, Nsec_c2, Nsecfilt;
  int code_c0, code_c1, code_c2;
  Average *average;
  Measure *measure;
  SecFilt *secfilt;
  Getstar_PS1_DEV_1 *output;
  Header header;
  Matrix matrix;
  Header theader;
  FTable ftable;
  FILE *f;
  int Noutput;

  Noutput = catalog[0].Naverage;
  ALLOCATE (output, Getstar_PS1_DEV_1, Noutput);

  // photcode is a global
  if (photcode == NULL) {
    fprintf (stderr, "undefined photcode\n");
    exit (2);
  }

  Nsec_c0 = GetPhotcodeNsec (photcode[0].code);
  Nsec_c1 = GetPhotcodeNsec (photcode[0].c1);
  Nsec_c2 = GetPhotcodeNsec (photcode[0].c2);
  Nsecfilt = GetPhotcodeNsecfilt ();

  code_c0 = photcode[0].code;
  code_c1 = photcode[0].c1;
  code_c2 = photcode[0].c2;
  measure = catalog[0].measure;
  average = catalog[0].average;
  secfilt = catalog[0].secfilt;

  // do we skip any of catalog entries? (probably not)
  for (i = 0; i < catalog[0].Naverage; i++) {
    
    output[i].R        = average[i].R;
    output[i].D        = average[i].D;
    output[i].uR       = average[i].uR;
    output[i].uD       = average[i].uD;
    output[i].P        = average[i].P;

    output[i].code     = average[i].flags;
    output[i].photcode = code_c0;

    // It is not necessary for the output color terms to be average values.  If they are,
    // we grab them quickly & easily from the secfilt table.  If not, then we need to scan
    // the list of measures to find the value of interest

    // find primary magnitude
    if (Nsec_c0 != -1) {
      output[i].mag = secfilt[i*Nsecfilt + Nsec_c0].MpsfChp;
    } else {
      output[i].mag = NAN;
      offset = average[i].measureOffset;
      for (m = 0; m < average[i].Nmeasure; m++) {
        if (measure[offset + m].photcode == code_c0) {
          output[i].mag = PhotRel (&measure[offset + m], &average[i], &secfilt[i*Nsecfilt], MAG_CLASS_PSF);
	  break;
        }
      }
    }

    // find color term 1
    if (Nsec_c1 != -1) {
      output[i].c1 = secfilt[i*Nsecfilt + Nsec_c1].MpsfChp;
    } else {
      output[i].c1 = NAN;
      offset = average[i].measureOffset;
      for (m = 0; m < average[i].Nmeasure; m++) {
        if (measure[offset + m].photcode == code_c1) {
          output[i].c1 = PhotRel (&measure[offset + m], &average[i], &secfilt[i*Nsecfilt], MAG_CLASS_PSF);
	  break;
        }
      }
    }

    // find color term 2
    if (Nsec_c2 != -1) {
      output[i].c2 = secfilt[i*Nsecfilt + Nsec_c2].MpsfChp;
    } else {
      output[i].c2 = NAN;
      offset = average[i].measureOffset;
      for (m = 0; m < average[i].Nmeasure; m++) {
        if (measure[offset + m].photcode == code_c2) {
          output[i].c2 = PhotRel (&measure[offset + m], &average[i], &secfilt[i*Nsecfilt], MAG_CLASS_PSF);
	  break;
        }
      }
    }
  }

  // open file for output
  f = fopen (OUTPUT, "w");
  if (f == NULL) {
    fprintf (stderr, "ERROR: can't open output file %s\n", OUTPUT);
    exit (1);
  }

  // create primary header
  gfits_init_header (&header);    
  header.extend = TRUE;
  gfits_create_header (&header);
  gfits_create_matrix (&header, &matrix);
  gfits_print (&header, "NEXTEND", "%d", 1, 1);

  ftable.header = &theader;
  gfits_table_set_Getstar_PS1_DEV_1 (&ftable, output, Noutput, TRUE);

  gfits_fwrite_header  (f, &header);
  gfits_fwrite_matrix  (f, &matrix);
  gfits_fwrite_Theader (f, &theader);
  gfits_fwrite_table   (f, &ftable);

  fprintf (stderr, "SUCCESS\n");
  exit (0);
}
