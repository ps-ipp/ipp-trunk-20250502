#include <pslib.h>
#include <ippdb.h>
#include <stdlib.h>
#include <string.h>

#define MAX_STRING_LENGTH 1024

int main ()
{
    {
        psMetadata      *md;
        pzDataStoreRow  *object;
        bool            status;

        object = pzDataStoreRowAlloc("a string", "a string", "a string", "0001-01-01T00:00:00Z");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = pzDataStoreMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

        if (strncmp(psMetadataLookupPtr(&status, md, "camera"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "telescope"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "uri"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        summitExpRow    *object;
        bool            status;

        object = summitExpRowAlloc("a string", "a string", "a string", "0001-01-01T00:00:00Z", "a string", "a string", -32, -16, "0001-01-01T00:00:00Z");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = summitExpMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

        if (strncmp(psMetadataLookupPtr(&status, md, "exp_name"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "camera"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "telescope"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "exp_type"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "uri"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupS32(&status, md, "imfiles") == -32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        summitImfileRow *object;
        bool            status;

        object = summitImfileRowAlloc("a string", "a string", "a string", "a string", -32, "a string", "a string", "a string", "a string", "0001-01-01T00:00:00Z");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = summitImfileMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

        if (strncmp(psMetadataLookupPtr(&status, md, "exp_name"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "camera"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "telescope"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "file_id"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupS32(&status, md, "bytes") == -32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "md5sum"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "class"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "class_id"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "uri"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        pzDownloadExpRow *object;
        bool            status;

        object = pzDownloadExpRowAlloc("a string", "a string", "a string", "a string", "0001-01-01T00:00:00Z");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = pzDownloadExpMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

        if (strncmp(psMetadataLookupPtr(&status, md, "exp_name"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "camera"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "telescope"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "state"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        pzDownloadImfileRow *object;
        bool            status;

        object = pzDownloadImfileRowAlloc("a string", "a string", "a string", "a string", "a string", "a string", -16, "0001-01-01T00:00:00Z");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = pzDownloadImfileMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

        if (strncmp(psMetadataLookupPtr(&status, md, "exp_name"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "camera"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "telescope"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "class"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "class_id"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "uri"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        newExpRow       *object;
        bool            status;

        object = newExpRowAlloc(-64, "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "0001-01-01T00:00:00Z");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = newExpMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "tmp_exp_name"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "tmp_camera"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "tmp_telescope"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "state"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "workdir"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "workdir_state"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "reduction"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "dvodb"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "tess_id"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "end_stage"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "label"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        newImfileRow    *object;
        bool            status;

        object = newImfileRowAlloc(-64, "a string", "a string", "0001-01-01T00:00:00Z");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = newImfileMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "tmp_class_id"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "uri"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        rawExpRow       *object;
        bool            status;

        object = rawExpRowAlloc(-64, "a string", "a string", "a string", "0001-01-01T00:00:00Z", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", 32.32, 64.64, 64.64, 32.32, 32.32, 64.64, 64.64, 64.64, 64.64, 64.64, 32.32, 64.64, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 64.64, 64.64, 64.64, 64.64, 64.64, "a string", 32.32, 32.32, 32.32, 32.32, 32.32, "a string", -16, "0001-01-01T00:00:00Z");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = rawExpMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "exp_name"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "camera"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "telescope"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "exp_tag"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "exp_type"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "filelevel"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "workdir"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "reduction"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "dvodb"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "tess_id"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "end_stage"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "filter"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "comment"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "obs_mode"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "obs_group"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "airmass") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "ra") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "decl") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "exp_time") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "sat_pixel_frac") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "bg") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "bg_stdev") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "bg_mean_stdev") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "alt") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "az") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "ccd_temp") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "posang") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "m1_x") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "m1_y") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "m1_z") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "m1_tip") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "m1_tilt") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "m2_x") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "m2_y") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "m2_z") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "m2_tip") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "m2_tilt") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "env_temperature") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "env_humidity") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "env_wind_speed") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "env_wind_dir") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "teltemp_m1") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "teltemp_m1cell") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "teltemp_m2") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "teltemp_spider") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "teltemp_truss") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "teltemp_extra") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "pon_time") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "user_1") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "user_2") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "user_3") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "user_4") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "user_5") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "object"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "sun_angle") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "sun_alt") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "moon_angle") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "moon_alt") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "moon_phase") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "hostname"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        rawImfileRow    *object;
        bool            status;

        object = rawImfileRowAlloc(-64, "a string", "a string", "a string", "0001-01-01T00:00:00Z", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", 32.32, 64.64, 64.64, 32.32, 32.32, 64.64, 64.64, 64.64, 64.64, 64.64, 32.32, 64.64, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 64.64, 64.64, 64.64, 64.64, 64.64, "a string", 32.32, 32.32, 32.32, 32.32, 32.32, "a string", -16, "0001-01-01T00:00:00Z");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = rawImfileMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "exp_name"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "camera"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "telescope"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "tmp_class_id"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "class_id"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "uri"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "exp_type"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "filelevel"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "filter"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "comment"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "obs_mode"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "obs_group"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "airmass") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "ra") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "decl") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "exp_time") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "sat_pixel_frac") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "bg") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "bg_stdev") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "bg_mean_stdev") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "alt") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "az") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "ccd_temp") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "posang") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "m1_x") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "m1_y") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "m1_z") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "m1_tip") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "m1_tilt") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "m2_x") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "m2_y") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "m2_z") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "m2_tip") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "m2_tilt") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "env_temperature") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "env_humidity") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "env_wind_speed") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "env_wind_dir") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "teltemp_m1") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "teltemp_m1cell") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "teltemp_m2") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "teltemp_spider") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "teltemp_truss") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "teltemp_extra") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "pon_time") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "user_1") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "user_2") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "user_3") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "user_4") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "user_5") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "object"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "sun_angle") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "sun_alt") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "moon_angle") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "moon_alt") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "moon_phase") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "hostname"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        guidePendingExpRow *object;
        bool            status;

        object = guidePendingExpRowAlloc(-64, -64, "a string");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = guidePendingExpMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "recipe"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        chipRunRow      *object;
        bool            status;

        object = chipRunRowAlloc(-64, -64, "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = chipRunMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "state"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "workdir"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "workdir_state"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "label"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "reduction"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "expgroup"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "dvodb"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "tess_id"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "end_stage"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        chipProcessedImfileRow *object;
        bool            status;

        object = chipProcessedImfileRowAlloc(-64, -64, "a string", "a string", "a string", 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, "a string", -32, -32, -32, -32, -32, "a string", -16);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = chipProcessedImfileMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "class_id"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "data_state"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "uri"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "bg") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "bg_stdev") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "bg_mean_stdev") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "bias") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "bias_stdev") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "fringe_0") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "fringe_1") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "fringe_2") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "ap_resid") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "ap_resid_stdev") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "fwhm_major") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "fwhm_minor") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "iq_m2") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "iq_m2_err") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "iq_m2_lq") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "iq_m2_uq") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "iq_m2c") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "iq_m2c_err") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "iq_m2c_lq") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "iq_m2c_uq") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "iq_m2s") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "iq_m2s_err") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "iq_m2s_lq") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "iq_m2s_uq") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "iq_m3") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "iq_m3_err") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "iq_m3_lq") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "iq_m3_uq") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "iq_m4") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "iq_m4_err") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "iq_m4_lq") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "iq_m4_uq") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "dtime_detrend") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "dtime_photom") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "dtime_total") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "dtime_script") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "hostname"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupS32(&status, md, "n_stars") == -32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupS32(&status, md, "n_psfstars") == -32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupS32(&status, md, "n_iqstars") == -32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupS32(&status, md, "n_extended") == -32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupS32(&status, md, "n_cr") == -32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "path_base"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        chipMaskRow     *object;
        bool            status;

        object = chipMaskRowAlloc("a string");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = chipMaskMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

        if (strncmp(psMetadataLookupPtr(&status, md, "label"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        camRunRow       *object;
        bool            status;

        object = camRunRowAlloc(-64, -64, "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = camRunMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "state"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "workdir"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "workdir_state"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "label"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "reduction"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "expgroup"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "dvodb"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "tess_id"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "end_stage"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        camProcessedExpRow *object;
        bool            status;

        object = camProcessedExpRowAlloc(-64, "a string", 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, "a string", -32, -32, -32, -32, -32, -32, "a string", -16);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = camProcessedExpMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "uri"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "bg") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "bg_stdev") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "bg_mean_stdev") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "bias") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "bias_stdev") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "fringe_0") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "fringe_1") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "fringe_2") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "sigma_ra") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "sigma_dec") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "ap_resid") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "ap_resid_stdev") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "zp_mean") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "zp_stdev") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "fwhm_major") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "fwhm_minor") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "iq_m2") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "iq_m2_err") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "iq_m2_lq") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "iq_m2_uq") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "iq_m2c") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "iq_m2c_err") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "iq_m2c_lq") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "iq_m2c_uq") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "iq_m2s") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "iq_m2s_err") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "iq_m2s_lq") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "iq_m2s_uq") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "iq_m3") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "iq_m3_err") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "iq_m3_lq") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "iq_m3_uq") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "iq_m4") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "iq_m4_err") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "iq_m4_lq") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "iq_m4_uq") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "dtime_script") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "dtime_astrom") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "dtime_addstar") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "hostname"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupS32(&status, md, "n_stars") == -32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupS32(&status, md, "n_psfstars") == -32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupS32(&status, md, "n_iqstars") == -32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupS32(&status, md, "n_extended") == -32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupS32(&status, md, "n_cr") == -32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupS32(&status, md, "n_astrom") == -32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "path_base"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        camMaskRow      *object;
        bool            status;

        object = camMaskRowAlloc("a string");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = camMaskMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

        if (strncmp(psMetadataLookupPtr(&status, md, "label"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        fakeRunRow      *object;
        bool            status;

        object = fakeRunRowAlloc(-64, -64, "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "0001-01-01T00:00:00Z");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = fakeRunMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "state"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "workdir"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "label"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "reduction"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "expgroup"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "dvodb"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "tess_id"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "end_stage"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        fakeProcessedImfileRow *object;
        bool            status;

        object = fakeProcessedImfileRowAlloc(-64, -64, "a string", "a string", 32.32, 32.32, "a string", "a string", "a string", -16, "0001-01-01T00:00:00Z");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = fakeProcessedImfileMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "class_id"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "uri"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "dtime_fake") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "dtime_script") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "hostname"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "path_base"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "data_state"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        fakeMaskRow     *object;
        bool            status;

        object = fakeMaskRowAlloc("a string");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = fakeMaskMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

        if (strncmp(psMetadataLookupPtr(&status, md, "label"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        warpRunRow      *object;
        bool            status;

        object = warpRunRowAlloc(-64, -64, "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "0001-01-01T00:00:00Z", true);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = warpRunMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "mode"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "state"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "workdir"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "workdir_state"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "label"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "dvodb"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "tess_id"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "end_stage"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupBool(&status, md, "magiced") == true) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        warpSkyCellMapRow *object;
        bool            status;

        object = warpSkyCellMapRowAlloc(-64, "a string", "a string", "a string", -16);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = warpSkyCellMapMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "skycell_id"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "tess_id"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "class_id"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        warpSkyfileRow  *object;
        bool            status;

        object = warpSkyfileRowAlloc(-64, "a string", "a string", "a string", "a string", "a string", 64.64, 64.64, 32.32, 32.32, "a string", 32.32, -32, -32, -32, -32, true, -16);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = warpSkyfileMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "skycell_id"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "tess_id"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "uri"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "path_base"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "data_state"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "bg") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "bg_stdev") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "dtime_warp") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "dtime_script") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "hostname"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "good_frac") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupS32(&status, md, "xmin") == -32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupS32(&status, md, "xmax") == -32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupS32(&status, md, "ymin") == -32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupS32(&status, md, "ymax") == -32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupBool(&status, md, "ignored") == true) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        warpMaskRow     *object;
        bool            status;

        object = warpMaskRowAlloc("a string");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = warpMaskMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

        if (strncmp(psMetadataLookupPtr(&status, md, "label"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        diffRunRow      *object;
        bool            status;

        object = diffRunRowAlloc(-64, "a string", "a string", "a string", "a string", "a string", "0001-01-01T00:00:00Z", "a string", "a string");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = diffRunMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "state"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "label"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "reduction"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "workdir"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "dvodb"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "skycell_id"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "tess_id"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        diffInputSkyfileRow *object;
        bool            status;

        object = diffInputSkyfileRowAlloc(-64, true, -64, -64, "a string", "a string", "a string");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = diffInputSkyfileMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupBool(&status, md, "template") == true) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "skycell_id"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "tess_id"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "kind"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        diffSkyfileRow  *object;
        bool            status;

        object = diffSkyfileRowAlloc(-64, "a string", "a string", 64.64, 64.64, -32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, -32, 32.32, 32.32, 32.32, 32.32, "a string", 32.32, -16);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = diffSkyfileMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "uri"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "path_base"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "bg") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "bg_stdev") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupS32(&status, md, "stamps_num") == -32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "stamps_mean") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "stamps_rms") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "norm") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "bg_diff") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "kernel_x") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "kernel_y") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "kernel_xx") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "kernel_xy") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "kernel_yy") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupS32(&status, md, "sources") == -32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "dtime_diff") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "dtime_match") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "dtime_phot") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "dtime_script") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "hostname"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "good_frac") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        stackRunRow     *object;
        bool            status;

        object = stackRunRowAlloc(-64, "a string", "a string", "a string", "a string", "a string", "0001-01-01T00:00:00Z", "a string", "a string", "a string");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = stackRunMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "state"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "workdir"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "label"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "reduction"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "dvodb"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "skycell_id"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "tess_id"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "filter"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        stackInputSkyfileRow *object;
        bool            status;

        object = stackInputSkyfileRowAlloc(-64, -64);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = stackInputSkyfileMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        stackSumSkyfileRow *object;
        bool            status;

        object = stackSumSkyfileRowAlloc(-64, "a string", "a string", 64.64, 64.64, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, -32, -32, 32.32, 32.32, -32, "a string", 32.32, -16);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = stackSumSkyfileMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "uri"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "path_base"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "bg") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "bg_stdev") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "dtime_stack") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "dtime_match_mean") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "dtime_match_stdev") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "dtime_initial") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "dtime_reject") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "dtime_final") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "dtime_phot") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "dtime_script") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "match_mean") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "match_stdev") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "match_rms") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "stamps_mean") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "stamps_stdev") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupS32(&status, md, "stamps_min") == -32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupS32(&status, md, "reject_images") == -32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "reject_pix_mean") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "reject_pix_stdev") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupS32(&status, md, "sources") == -32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "hostname"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "good_frac") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        detRunRow       *object;
        bool            status;

        object = detRunRowAlloc(-64, -32, "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 64.64, 64.64, "0001-01-01T00:00:00Z", "0001-01-01T00:00:00Z", "0001-01-01T00:00:00Z", "0001-01-01T00:00:00Z", "0001-01-01T00:00:00Z", 32.32, 32.32, "a string", -64, -32);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = detRunMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupS32(&status, md, "iteration") == -32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "det_type"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "mode"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "state"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "filelevel"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "workdir"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "camera"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "telescope"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "exp_type"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "reduction"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "filter"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "airmass_min") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "airmass_max") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "exp_time_min") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "exp_time_max") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "ccd_temp_min") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "ccd_temp_max") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "posang_min") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "posang_max") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "solang_min") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "solang_max") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "label"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupS32(&status, md, "ref_iter") == -32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        detInputExpRow  *object;
        bool            status;

        object = detInputExpRowAlloc(-64, -32, -64, true);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = detInputExpMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupS32(&status, md, "iteration") == -32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupBool(&status, md, "include") == true) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        detProcessedImfileRow *object;
        bool            status;

        object = detProcessedImfileRowAlloc(-64, -64, "a string", "a string", "a string", 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, "a string", "a string", -16);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = detProcessedImfileMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "class_id"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "uri"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "recipe"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "bg") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "bg_stdev") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "bg_mean_stdev") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "fringe_0") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "fringe_1") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "fringe_2") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "user_1") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "user_2") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "user_3") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "user_4") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "user_5") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "path_base"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "data_state"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        detProcessedExpRow *object;
        bool            status;

        object = detProcessedExpRowAlloc(-64, -64, "a string", 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, "a string", "a string", -16);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = detProcessedExpMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "recipe"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "bg") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "bg_stdev") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "bg_mean_stdev") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "fringe_0") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "fringe_1") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "fringe_2") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "user_1") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "user_2") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "user_3") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "user_4") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "user_5") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "path_base"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "data_state"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        detStackedImfileRow *object;
        bool            status;

        object = detStackedImfileRowAlloc(-64, -32, "a string", "a string", "a string", 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, "a string", -16);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = detStackedImfileMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupS32(&status, md, "iteration") == -32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "class_id"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "uri"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "recipe"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "bg") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "bg_stdev") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "bg_mean_stdev") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "user_1") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "user_2") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "user_3") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "user_4") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "user_5") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "data_state"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        detNormalizedStatImfileRow *object;
        bool            status;

        object = detNormalizedStatImfileRowAlloc(-64, -32, "a string", 32.32, "a string", -16);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = detNormalizedStatImfileMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupS32(&status, md, "iteration") == -32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "class_id"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF32(&status, md, "norm") == 32.32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "data_state"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        detNormalizedImfileRow *object;
        bool            status;

        object = detNormalizedImfileRowAlloc(-64, -32, "a string", "a string", 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, "a string", "a string", -16);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = detNormalizedImfileMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupS32(&status, md, "iteration") == -32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "class_id"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "uri"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "bg") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "bg_stdev") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "bg_mean_stdev") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "user_1") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "user_2") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "user_3") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "user_4") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "user_5") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "path_base"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "data_state"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        detNormalizedExpRow *object;
        bool            status;

        object = detNormalizedExpRowAlloc(-64, -32, "a string", 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, "a string", "a string", -16);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = detNormalizedExpMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupS32(&status, md, "iteration") == -32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "recipe"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "bg") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "bg_stdev") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "bg_mean_stdev") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "user_1") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "user_2") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "user_3") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "user_4") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "user_5") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "path_base"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "data_state"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        detResidImfileRow *object;
        bool            status;

        object = detResidImfileRowAlloc(-64, -32, -64, -32, -64, "a string", "a string", "a string", 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, "a string", "a string", -16);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = detResidImfileMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupS32(&status, md, "iteration") == -32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupS32(&status, md, "ref_iter") == -32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "class_id"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "uri"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "recipe"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "bg") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "bg_stdev") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "bg_mean_stdev") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "bg_skewness") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "bg_kurtosis") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "bin_stdev") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "fringe_0") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "fringe_1") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "fringe_2") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "fringe_resid_0") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "fringe_resid_1") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "fringe_resid_2") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "user_1") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "user_2") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "user_3") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "user_4") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "user_5") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "path_base"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "data_state"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        detResidExpRow  *object;
        bool            status;

        object = detResidExpRowAlloc(-64, -32, -64, "a string", 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, "a string", "a string", true, -16);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = detResidExpMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupS32(&status, md, "iteration") == -32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "recipe"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "bg") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "bg_stdev") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "bg_mean_stdev") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "bg_skewness") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "bg_kurtosis") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "bin_stdev") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "fringe_0") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "fringe_1") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "fringe_2") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "fringe_resid_0") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "fringe_resid_1") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "fringe_resid_2") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "user_1") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "user_2") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "user_3") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "user_4") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "user_5") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "path_base"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "data_state"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupBool(&status, md, "accept") == true) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        detRunSummaryRow *object;
        bool            status;

        object = detRunSummaryRowAlloc(-64, -32, "a string", 64.64, 64.64, 64.64, true, -16);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = detRunSummaryMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupS32(&status, md, "iteration") == -32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "data_state"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "bg") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "bg_stdev") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "bg_mean_stdev") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupBool(&status, md, "accept") == true) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        detRegisteredImfileRow *object;
        bool            status;

        object = detRegisteredImfileRowAlloc(-64, -32, "a string", "a string", 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, "a string", "a string", -16);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = detRegisteredImfileMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupS32(&status, md, "iteration") == -32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "class_id"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "uri"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "bg") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "bg_stdev") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "bg_mean_stdev") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "user_1") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "user_2") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "user_3") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "user_4") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupF64(&status, md, "user_5") == 64.64) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "path_base"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "data_state"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        detCorrectedExpRow *object;
        bool            status;

        object = detCorrectedExpRowAlloc(-64, -64, "a string", -64, "a string", "a string", "a string", -16);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = detCorrectedExpMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "uri"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "corr_type"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "recipe"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "path_base"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        detCorrectedImfileRow *object;
        bool            status;

        object = detCorrectedImfileRowAlloc(-64, -64, "a string", "a string", "a string", -16);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = detCorrectedImfileMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "class_id"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "uri"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "path_base"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        magicRunRow     *object;
        bool            status;

        object = magicRunRowAlloc(-64, -64, "a string", "a string", "a string", "a string", "a string", "0001-01-01T00:00:00Z", -16);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = magicRunMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "state"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "workdir"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "workdir_state"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "label"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "dvodb"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        magicInputSkyfileRow *object;
        bool            status;

        object = magicInputSkyfileRowAlloc(-64, -64, "a string");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = magicInputSkyfileMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "node"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        magicTreeRow    *object;
        bool            status;

        object = magicTreeRowAlloc(-64, "a string", "a string");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = magicTreeMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "node"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "dep"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        magicNodeResultRow *object;
        bool            status;

        object = magicNodeResultRowAlloc(-64, "a string", "a string", -16);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = magicNodeResultMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "node"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "uri"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        magicMaskRow    *object;
        bool            status;

        object = magicMaskRowAlloc(-64, "a string", -32, -16);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = magicMaskMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "uri"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupS32(&status, md, "streaks") == -32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        calDBRow        *object;
        bool            status;

        object = calDBRowAlloc(-64, "a string", "a string");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = calDBMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "dvodb"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "state"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        calRunRow       *object;
        bool            status;

        object = calRunRowAlloc(-64, "a string", "a string", "a string");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = calRunMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "region"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "last_step"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "state"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        flatcorrRunRow  *object;
        bool            status;

        object = flatcorrRunRowAlloc(-64, "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", -16);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = flatcorrRunMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "dvodb"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "filter"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "state"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "workdir"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "label"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "reduction"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "region"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "hostname"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        flatcorrChipLinkRow *object;
        bool            status;

        object = flatcorrChipLinkRowAlloc(-64, -64);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = flatcorrChipLinkMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        flatcorrCamLinkRow *object;
        bool            status;

        object = flatcorrCamLinkRowAlloc(-64, -64, -64);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = flatcorrCamLinkMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        pstampDataStoreRow *object;
        bool            status;

        object = pstampDataStoreRowAlloc(-64, "a string", "a string", "a string", "a string");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = pstampDataStoreMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "state"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "lastFileset"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "outProduct"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "uri"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        pstampProjectRow *object;
        bool            status;

        object = pstampProjectRowAlloc(-64, "a string", "a string", "a string", "a string", "a string", "a string", true);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = pstampProjectMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "name"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "state"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "dbname"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "dvodb"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "camera"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "telescope"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupBool(&status, md, "need_magic") == true) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        pstampRequestRow *object;
        bool            status;

        object = pstampRequestRowAlloc(-64, -64, "a string", "a string", "a string", "a string", "a string", -32);
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = pstampRequestMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "state"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "name"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "reqType"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "outProduct"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "uri"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupS32(&status, md, "fault") == -32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    {
        psMetadata      *md;
        pstampJobRow    *object;
        bool            status;

        object = pstampJobRowAlloc(-64, -64, "a string", "a string", "a string", -32, "a string", -64, "a string", "a string");
        if (!object) {
            exit(EXIT_FAILURE);
        }

        md = pstampJobMetadataFromObject(object);
        if (!md) {
            exit(EXIT_FAILURE);
        }

        psFree(object);

            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "rownum"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "state"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "jobType"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataLookupS32(&status, md, "fault") == -32) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "uri"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "outputBase"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (strncmp(psMetadataLookupPtr(&status, md, "args"), "a string", MAX_STRING_LENGTH)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);
    }

    exit(EXIT_SUCCESS);
}
