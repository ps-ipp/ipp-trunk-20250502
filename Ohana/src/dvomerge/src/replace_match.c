# include "dvomerge.h"

// XXX this function does not work because it it loops over the average->Nmeasure entries
// as if they were sorted, but objects which have had entries added to them have elements
// out of sequence.  I probably need to step through next_meas to find the sequence.

// average and measure pointers to the objects of interest and their first / current measurement
int replace_match (Average *average_out, Measure *measure_out, off_t *next_meas, Average *average_in, Measure *measure_in) {
  OHANA_UNUSED_PARAM(average_in);

  int Nout;
  unsigned int averef;
  unsigned int catID;

  off_t m = average_out->measureOffset;

  // find the matching photcode in the object's list of measurements
  for (Nout = 0; Nout < average_out[0].Nmeasure; Nout ++) {
    myAssert (m > -1, "oops");
    if (measure_out[m].photcode != measure_in[0].photcode) {
      m = next_meas[m];
      continue;
    }
    
    // set the new measurements
    averef = measure_out[m].averef;
    catID  = measure_out[m].catID;
    measure_out[m] = measure_in[0];

    // old code: find R,D using average_in[0], the get offset relative to average_out[0].  no longer
    // needed since we carry around R,D
    // double Rin = average_in[0].R - measure_in[0].dR / 3600.0;
    // double Din = average_in[0].D - measure_in[0].dD / 3600.0;
    // measure_out[Nout].dR = 3600.0*(average_out[0].R - Rin);
    // measure_out[Nout].dD = 3600.0*(average_out[0].D - Din);

    measure_out[m].dbFlags  = 0;  // XXX why reset these?
    measure_out[m].averef   = averef;
    measure_out[m].objID    = average_out[0].objID;
    measure_out[m].catID    = catID;

    float dRoff = dvoOffsetR(&measure_out[m], average_out);

    // rationalize dR
    if (dRoff > +180.0*3600.0) {
      // average on high end of boundary, move star up
      measure_out[m].R += 360.0;
      dRoff -= 360.0*3600.0;
    }
    if (dRoff < -180.0*3600.0) {
      // average on low end of boundary, move star down
      measure_out[m].R -= 360.0;
      dRoff += 360.0*3600.0;
    }

    // warn on surprisingly distant detections
    if (fabs(dRoff) > 10*RADIUS) {
      // ok take declination into account and check again.
      double cosD = cos(RAD_DEG*average_out[0].D);
      if (fabs(dRoff*cosD) > 10*RADIUS) {
	fprintf (stderr, "surprisingly distant detection: %10.6f,%10.6f vs %10.6f,%10.6f\n", 
		 average_out[0].R, average_out[0].D, measure_out[m].R, measure_out[m].D);
      }
    }
    return TRUE; // found the matched entry
  }
  return FALSE; // did not find the matched entry
}

