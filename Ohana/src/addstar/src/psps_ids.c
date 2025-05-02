# include "addstar.h"

// Compute IDs for PSPS based on recipe they defined

uint64_t
CreatePSPSDetectionID(double tobs, int ccdid, int detID)
{
    // t0 for detection id is 2007-01-01 00:00:00 utc
    double  t0 = 54101.0;
    double diff = floor( 100000. * (tobs - t0) );
    int itmp = diff;

    // ccdid must be < 100
    uint64_t detectid = 1000000000*((uint64_t) itmp) + 10000000 * ((uint64_t) ccdid) +
             ((uint64_t) detID);

    return detectid;
}
    
uint64_t
CreatePSPSStackDetectionID(int sourceID, int imageID, int detID)
{
  // sourceID : ID of database + table that tracked the image (< 0x7f = 127)
  // imageID : external ID of the image which provided the detections (< 0x1000.0000 ~ 2.7e8)
  // detID : detection sequence in image (< 0x1000.0000 ~ 2.7e8)

  assert (detID    < 0x10000000);
  assert (imageID  < 0x10000000);
  assert (sourceID < 0x7f);
  
  uint64_t detectid = ((uint64_t)sourceID << 56) + ((uint64_t)imageID << 28) + (uint64_t)detID;
  return detectid;
}
    
uint64_t
CreatePSPSObjectID(double ra, double dec)
{
    double zh = 0.0083333;
    double zid = (dec + 90.) / zh;             // 0 - 180*60*2 = 21600 < 15 bits
    int izone = (int) floor(zid);
    double zresid = zid -  ((float) izone);    // 0.0 - 1.0

    uint64_t part1, part2, part3;
    part1 = (uint64_t)( izone  * 10000000000000LL) ; // 10,000,000,000,000
    part2 = ((uint64_t)(ra * 1000000.)) * 10000 ; // 0 - 360*1e6 = 3.6e8 (< 29 bits)
    part3 = (int) (zresid * 10000.0) ; // 0 - 10000 (1 bit == 30/10000 arcsec) (< 14 bits)

    return part1 + part2 + part3;
}

// 10 000 000 000 000
