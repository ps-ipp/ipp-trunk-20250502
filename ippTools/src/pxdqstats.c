/* pxdqstats.c
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <ippdb.h>
#include <string.h>

#include "pxtools.h"
#include "pxdqstats.h"

bool pxdqstatsSetSearchArgs (psMetadata *md) {

  psMetadataAddS64(md,  PS_LIST_TAIL, "-cam_id",            0, "search by cam_id", 0);
  psMetadataAddS64(md,  PS_LIST_TAIL, "-exp_id",             0, "search by exp_id", 0);
  psMetadataAddStr(md,  PS_LIST_TAIL, "-exp_name",           0, "search by exp_name", NULL);
  psMetadataAddStr(md,  PS_LIST_TAIL, "-inst",               0, "search for camera", NULL);
  psMetadataAddStr(md,  PS_LIST_TAIL, "-telescope",          0, "search for telescope", NULL);
  psMetadataAddTime(md, PS_LIST_TAIL, "-dateobs_begin",      0, "search for exposures by time (>=)", NULL);
  psMetadataAddTime(md, PS_LIST_TAIL, "-dateobs_end",        0, "search for exposures by time (<)", NULL);
  psMetadataAddStr(md,  PS_LIST_TAIL, "-exp_tag",            0, "search by exp_tag", NULL);
  psMetadataAddStr(md,  PS_LIST_TAIL, "-exp_type",           0, "search by exp_type", NULL);
  psMetadataAddStr(md,  PS_LIST_TAIL, "-comment",            0, "search by comment", NULL);
  psMetadataAddStr(md,  PS_LIST_TAIL, "-filelevel",          0, "search by filelevel", NULL);
  psMetadataAddStr(md,  PS_LIST_TAIL, "-filter",             0, "search for filter", NULL);
  psMetadataAddF64(md,  PS_LIST_TAIL, "-airmass_min",        0, "define min airmass", NAN);
  psMetadataAddF64(md,  PS_LIST_TAIL, "-airmass_max",        0, "define max airmass", NAN);
  psMetadataAddF64(md,  PS_LIST_TAIL, "-ra_min",             0, "define min RA (degrees) ", NAN);
  psMetadataAddF64(md,  PS_LIST_TAIL, "-ra_max",             0, "define max RA (degrees) ", NAN);
  psMetadataAddF64(md,  PS_LIST_TAIL, "-decl_min",           0, "define min DEC (degrees)", NAN);
  psMetadataAddF64(md,  PS_LIST_TAIL, "-decl_max",           0, "define max DEC (degrees)", NAN);
  psMetadataAddF32(md,  PS_LIST_TAIL, "-exp_time_min",       0, "define min exposure time", NAN);
  psMetadataAddF32(md,  PS_LIST_TAIL, "-exp_time_max",       0, "define max exposure time", NAN);
  psMetadataAddF32(md,  PS_LIST_TAIL, "-sat_pixel_frac_min", 0, "define max fraction of saturated pixels", NAN);
  psMetadataAddF32(md,  PS_LIST_TAIL, "-sat_pixel_frac_max", 0, "define max fraction of saturated pixels", NAN);
  psMetadataAddF64(md,  PS_LIST_TAIL, "-bg_min",             0, "define min background", NAN);
  psMetadataAddF64(md,  PS_LIST_TAIL, "-bg_max",             0, "define max background", NAN);
  psMetadataAddF64(md,  PS_LIST_TAIL, "-bg_stdev_min",       0, "define min background standard deviation", NAN);
  psMetadataAddF64(md,  PS_LIST_TAIL, "-bg_stdev_max",       0, "define max background standard deviation", NAN);
  psMetadataAddF64(md,  PS_LIST_TAIL, "-bg_mean_stdev_min",  0, "define min background mean standard deviation (across imfiles)", NAN);
  psMetadataAddF64(md,  PS_LIST_TAIL, "-bg_mean_stdev_max",  0, "define max background mean standard deviation (across imfiles)", NAN);
  psMetadataAddF64(md,  PS_LIST_TAIL, "-alt_min",            0, "define min altitude", NAN);
  psMetadataAddF64(md,  PS_LIST_TAIL, "-alt_max",            0, "define max altitude", NAN);
  psMetadataAddF64(md,  PS_LIST_TAIL, "-az_min",             0, "define min azimuth ", NAN);
  psMetadataAddF64(md,  PS_LIST_TAIL, "-az_max",             0, "define max azimuth ", NAN);
  psMetadataAddF32(md,  PS_LIST_TAIL, "-ccd_temp_min",       0, "define min ccd tempature", NAN);
  psMetadataAddF32(md,  PS_LIST_TAIL, "-ccd_temp_max",       0, "define max ccd tempature", NAN);
  psMetadataAddF64(md,  PS_LIST_TAIL, "-posang_min",         0, "define min rotator position angle", NAN);
  psMetadataAddF64(md,  PS_LIST_TAIL, "-posang_max",         0, "define max rotator position angle", NAN);
  psMetadataAddStr(md,  PS_LIST_TAIL, "-object",             0, "search by exposure object", NULL);
  psMetadataAddF32(md,  PS_LIST_TAIL, "-sun_angle_min",         0, "define min solar angle", NAN);
  psMetadataAddF32(md,  PS_LIST_TAIL, "-sun_angle_max",         0, "define max solar angle", NAN);

  return true;
}

bool pxdqstatsGetSearchArgs (pxConfig *config, psMetadata *where) {

  PXOPT_COPY_S64(config->args,   where, "-cam_id",            "camRun.cam_id",      "==");
  PXOPT_COPY_S64(config->args,   where, "-exp_id",             "rawExp.exp_id",        "==");
  PXOPT_COPY_STR(config->args,   where, "-exp_name",           "rawExp.exp_name",      "==");
  PXOPT_COPY_STR(config->args,   where, "-inst",               "rawExp.camera",        "==");
  PXOPT_COPY_STR(config->args,   where, "-telescope",          "rawExp.telescope",     "==");
  PXOPT_COPY_TIME(config->args,  where, "-dateobs_begin",      "rawExp.dateobs",       ">=");
  PXOPT_COPY_TIME(config->args,  where, "-dateobs_end",        "rawExp.dateobs",       "<=");
  PXOPT_COPY_STR(config->args,   where, "-exp_tag",            "rawExp.exp_tag",       "==");
  PXOPT_COPY_STR(config->args,   where, "-exp_type",           "rawExp.exp_type",      "==");
  PXOPT_COPY_STR(config->args,   where, "-comment",            "rawExp.comment",       "LIKE");
  PXOPT_COPY_STR(config->args,   where, "-filelevel",          "rawExp.filelevel",     "==");
  PXOPT_COPY_STR(config->args,   where, "-filter",             "rawExp.filter",         "==");
  PXOPT_COPY_F64(config->args,   where, "-airmass_min",        "rawExp.airmass",        ">=");
  PXOPT_COPY_F64(config->args,   where, "-airmass_max",        "rawExp.airmass",        "<");
  PXOPT_COPY_RADEC(config->args, where, "-ra_min",             "rawExp.ra",             ">=");
  PXOPT_COPY_RADEC(config->args, where, "-ra_max",             "rawExp.ra",             "<");
  PXOPT_COPY_RADEC(config->args, where, "-decl_min",           "rawExp.decl",           ">=");
  PXOPT_COPY_RADEC(config->args, where, "-decl_max",           "rawExp.decl",           "<");
  PXOPT_COPY_F32(config->args,   where, "-exp_time_min",       "rawExp.exp_time",       ">=");
  PXOPT_COPY_F32(config->args,   where, "-exp_time_max",       "rawExp.exp_time",       "<");
  PXOPT_COPY_F32(config->args,   where, "-sat_pixel_frac_min", "rawExp.sat_pixel_frac", ">=");
  PXOPT_COPY_F32(config->args,   where, "-sat_pixel_frac_max", "rawExp.sat_pixel_frac", "<");
  PXOPT_COPY_F64(config->args,   where, "-bg_min",             "rawExp.bg",             ">=");
  PXOPT_COPY_F64(config->args,   where, "-bg_max",             "rawExp.bg",             "<");
  PXOPT_COPY_F64(config->args,   where, "-bg_stdev_min",       "rawExp.bg_stdev",       ">=");
  PXOPT_COPY_F64(config->args,   where, "-bg_stdev_max",       "rawExp.bg_stdev",       "<");
  PXOPT_COPY_F64(config->args,   where, "-bg_mean_stdev_min",  "rawExp.bg_mean_stdev",  ">=");
  PXOPT_COPY_F64(config->args,   where, "-bg_mean_stdev_max",  "rawExp.bg_mean_stdev",  "<");
  PXOPT_COPY_F64(config->args,   where, "-alt_min",            "rawExp.alt",            ">=");
  PXOPT_COPY_F64(config->args,   where, "-alt_max",            "rawExp.alt",            "<");
  PXOPT_COPY_F64(config->args,   where, "-az_min",             "rawExp.az",             ">=");
  PXOPT_COPY_F64(config->args,   where, "-az_max",             "rawExp.az",             "<");
  PXOPT_COPY_F32(config->args,   where, "-ccd_temp_min",       "rawExp.ccd_temp",       ">=");
  PXOPT_COPY_F32(config->args,   where, "-ccd_temp_max",       "rawExp.ccd_temp",       "<");
  PXOPT_COPY_F64(config->args,   where, "-posang_min",         "rawExp.posang",         ">=");
  PXOPT_COPY_F64(config->args,   where, "-posang_max",         "rawExp.posang",         "<");
  PXOPT_COPY_STR(config->args,   where, "-object",             "rawExp.object",         "==");
  PXOPT_COPY_F32(config->args,   where, "-sun_angle_min",         "rawExp.sun_angle",         ">=");
  PXOPT_COPY_F32(config->args,   where, "-sun_angle_max",         "rawExp.sun_angle",         "<");

  return true;
}
