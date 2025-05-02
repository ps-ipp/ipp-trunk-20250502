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

        md = psMetadataAlloc();
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "camera", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "telescope", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "uri", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = pzDataStoreObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

        if (strncmp(object->camera, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->telescope, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->uri, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        summitExpRow    *object;

        md = psMetadataAlloc();
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "exp_name", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "camera", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "telescope", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "exp_type", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "uri", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddS32(md, PS_LIST_TAIL, "imfiles", 0, NULL, -32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = summitExpObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

        if (strncmp(object->exp_name, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->camera, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->telescope, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->exp_type, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->uri, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->imfiles == -32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        summitImfileRow *object;

        md = psMetadataAlloc();
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "exp_name", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "camera", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "telescope", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "file_id", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddS32(md, PS_LIST_TAIL, "bytes", 0, NULL, -32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "md5sum", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "class", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "class_id", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "uri", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = summitImfileObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

        if (strncmp(object->exp_name, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->camera, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->telescope, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->file_id, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bytes == -32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->md5sum, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->class, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->class_id, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->uri, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        pzDownloadExpRow *object;

        md = psMetadataAlloc();
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "exp_name", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "camera", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "telescope", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "state", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = pzDownloadExpObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

        if (strncmp(object->exp_name, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->camera, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->telescope, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->state, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        pzDownloadImfileRow *object;

        md = psMetadataAlloc();
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "exp_name", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "camera", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "telescope", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "class", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "class_id", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "uri", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = pzDownloadImfileObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

        if (strncmp(object->exp_name, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->camera, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->telescope, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->class, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->class_id, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->uri, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        newExpRow       *object;

        md = psMetadataAlloc();
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "tmp_exp_name", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "tmp_camera", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "tmp_telescope", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "state", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "workdir", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "workdir_state", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "reduction", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "dvodb", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "tess_id", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "end_stage", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "label", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = newExpObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->tmp_exp_name, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->tmp_camera, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->tmp_telescope, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->state, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->workdir, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->workdir_state, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->reduction, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->dvodb, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->tess_id, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->end_stage, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->label, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        newImfileRow    *object;

        md = psMetadataAlloc();
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "tmp_class_id", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "uri", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = newImfileObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->tmp_class_id, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->uri, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        rawExpRow       *object;

        md = psMetadataAlloc();
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "exp_name", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "camera", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "telescope", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "exp_tag", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "exp_type", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "filelevel", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "workdir", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "reduction", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "dvodb", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "tess_id", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "end_stage", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "filter", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "comment", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "obs_mode", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "obs_group", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "airmass", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "ra", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "decl", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "exp_time", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "sat_pixel_frac", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "bg", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "bg_stdev", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "bg_mean_stdev", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "alt", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "az", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "ccd_temp", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "posang", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "m1_x", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "m1_y", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "m1_z", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "m1_tip", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "m1_tilt", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "m2_x", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "m2_y", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "m2_z", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "m2_tip", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "m2_tilt", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "env_temperature", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "env_humidity", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "env_wind_speed", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "env_wind_dir", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "teltemp_m1", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "teltemp_m1cell", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "teltemp_m2", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "teltemp_spider", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "teltemp_truss", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "teltemp_extra", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "pon_time", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "user_1", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "user_2", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "user_3", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "user_4", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "user_5", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "object", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "sun_angle", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "sun_alt", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "moon_angle", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "moon_alt", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "moon_phase", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "hostname", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = rawExpObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->exp_name, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->camera, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->telescope, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->exp_tag, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->exp_type, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->filelevel, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->workdir, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->reduction, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->dvodb, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->tess_id, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->end_stage, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->filter, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->comment, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->obs_mode, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->obs_group, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->airmass == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->ra == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->decl == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->exp_time == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->sat_pixel_frac == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bg == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bg_stdev == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bg_mean_stdev == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->alt == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->az == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->ccd_temp == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->posang == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->m1_x == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->m1_y == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->m1_z == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->m1_tip == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->m1_tilt == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->m2_x == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->m2_y == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->m2_z == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->m2_tip == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->m2_tilt == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->env_temperature == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->env_humidity == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->env_wind_speed == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->env_wind_dir == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->teltemp_m1 == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->teltemp_m1cell == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->teltemp_m2 == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->teltemp_spider == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->teltemp_truss == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->teltemp_extra == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->pon_time == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->user_1 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->user_2 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->user_3 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->user_4 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->user_5 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->object, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->sun_angle == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->sun_alt == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->moon_angle == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->moon_alt == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->moon_phase == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->hostname, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        rawImfileRow    *object;

        md = psMetadataAlloc();
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "exp_name", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "camera", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "telescope", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "tmp_class_id", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "class_id", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "uri", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "exp_type", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "filelevel", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "filter", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "comment", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "obs_mode", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "obs_group", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "airmass", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "ra", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "decl", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "exp_time", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "sat_pixel_frac", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "bg", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "bg_stdev", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "bg_mean_stdev", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "alt", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "az", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "ccd_temp", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "posang", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "m1_x", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "m1_y", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "m1_z", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "m1_tip", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "m1_tilt", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "m2_x", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "m2_y", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "m2_z", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "m2_tip", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "m2_tilt", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "env_temperature", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "env_humidity", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "env_wind_speed", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "env_wind_dir", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "teltemp_m1", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "teltemp_m1cell", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "teltemp_m2", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "teltemp_spider", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "teltemp_truss", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "teltemp_extra", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "pon_time", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "user_1", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "user_2", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "user_3", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "user_4", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "user_5", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "object", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "sun_angle", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "sun_alt", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "moon_angle", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "moon_alt", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "moon_phase", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "hostname", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = rawImfileObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->exp_name, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->camera, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->telescope, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->tmp_class_id, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->class_id, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->uri, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->exp_type, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->filelevel, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->filter, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->comment, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->obs_mode, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->obs_group, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->airmass == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->ra == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->decl == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->exp_time == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->sat_pixel_frac == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bg == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bg_stdev == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bg_mean_stdev == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->alt == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->az == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->ccd_temp == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->posang == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->m1_x == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->m1_y == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->m1_z == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->m1_tip == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->m1_tilt == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->m2_x == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->m2_y == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->m2_z == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->m2_tip == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->m2_tilt == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->env_temperature == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->env_humidity == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->env_wind_speed == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->env_wind_dir == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->teltemp_m1 == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->teltemp_m1cell == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->teltemp_m2 == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->teltemp_spider == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->teltemp_truss == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->teltemp_extra == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->pon_time == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->user_1 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->user_2 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->user_3 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->user_4 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->user_5 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->object, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->sun_angle == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->sun_alt == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->moon_angle == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->moon_alt == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->moon_phase == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->hostname, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        guidePendingExpRow *object;

        md = psMetadataAlloc();
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "recipe", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = guidePendingExpObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->recipe, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        chipRunRow      *object;

        md = psMetadataAlloc();
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "state", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "workdir", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "workdir_state", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "label", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "reduction", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "expgroup", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "dvodb", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "tess_id", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "end_stage", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = chipRunObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->state, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->workdir, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->workdir_state, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->label, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->reduction, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->expgroup, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->dvodb, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->tess_id, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->end_stage, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        chipProcessedImfileRow *object;

        md = psMetadataAlloc();
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "class_id", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "data_state", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "uri", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "bg", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "bg_stdev", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "bg_mean_stdev", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "bias", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "bias_stdev", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "fringe_0", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "fringe_1", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "fringe_2", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "ap_resid", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "ap_resid_stdev", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "fwhm_major", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "fwhm_minor", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "iq_m2", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "iq_m2_err", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "iq_m2_lq", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "iq_m2_uq", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "iq_m2c", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "iq_m2c_err", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "iq_m2c_lq", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "iq_m2c_uq", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "iq_m2s", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "iq_m2s_err", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "iq_m2s_lq", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "iq_m2s_uq", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "iq_m3", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "iq_m3_err", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "iq_m3_lq", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "iq_m3_uq", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "iq_m4", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "iq_m4_err", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "iq_m4_lq", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "iq_m4_uq", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "dtime_detrend", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "dtime_photom", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "dtime_total", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "dtime_script", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "hostname", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddS32(md, PS_LIST_TAIL, "n_stars", 0, NULL, -32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddS32(md, PS_LIST_TAIL, "n_psfstars", 0, NULL, -32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddS32(md, PS_LIST_TAIL, "n_iqstars", 0, NULL, -32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddS32(md, PS_LIST_TAIL, "n_extended", 0, NULL, -32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddS32(md, PS_LIST_TAIL, "n_cr", 0, NULL, -32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "path_base", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = chipProcessedImfileObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->class_id, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->data_state, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->uri, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bg == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bg_stdev == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bg_mean_stdev == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bias == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bias_stdev == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->fringe_0 == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->fringe_1 == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->fringe_2 == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->ap_resid == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->ap_resid_stdev == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->fwhm_major == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->fwhm_minor == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iq_m2 == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iq_m2_err == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iq_m2_lq == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iq_m2_uq == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iq_m2c == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iq_m2c_err == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iq_m2c_lq == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iq_m2c_uq == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iq_m2s == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iq_m2s_err == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iq_m2s_lq == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iq_m2s_uq == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iq_m3 == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iq_m3_err == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iq_m3_lq == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iq_m3_uq == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iq_m4 == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iq_m4_err == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iq_m4_lq == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iq_m4_uq == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->dtime_detrend == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->dtime_photom == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->dtime_total == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->dtime_script == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->hostname, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->n_stars == -32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->n_psfstars == -32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->n_iqstars == -32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->n_extended == -32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->n_cr == -32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->path_base, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        chipMaskRow     *object;

        md = psMetadataAlloc();
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "label", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = chipMaskObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

        if (strncmp(object->label, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        camRunRow       *object;

        md = psMetadataAlloc();
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "state", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "workdir", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "workdir_state", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "label", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "reduction", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "expgroup", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "dvodb", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "tess_id", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "end_stage", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = camRunObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->state, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->workdir, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->workdir_state, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->label, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->reduction, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->expgroup, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->dvodb, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->tess_id, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->end_stage, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        camProcessedExpRow *object;

        md = psMetadataAlloc();
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "uri", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "bg", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "bg_stdev", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "bg_mean_stdev", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "bias", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "bias_stdev", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "fringe_0", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "fringe_1", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "fringe_2", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "sigma_ra", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "sigma_dec", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "ap_resid", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "ap_resid_stdev", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "zp_mean", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "zp_stdev", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "fwhm_major", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "fwhm_minor", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "iq_m2", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "iq_m2_err", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "iq_m2_lq", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "iq_m2_uq", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "iq_m2c", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "iq_m2c_err", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "iq_m2c_lq", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "iq_m2c_uq", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "iq_m2s", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "iq_m2s_err", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "iq_m2s_lq", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "iq_m2s_uq", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "iq_m3", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "iq_m3_err", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "iq_m3_lq", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "iq_m3_uq", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "iq_m4", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "iq_m4_err", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "iq_m4_lq", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "iq_m4_uq", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "dtime_script", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "dtime_astrom", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "dtime_addstar", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "hostname", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddS32(md, PS_LIST_TAIL, "n_stars", 0, NULL, -32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddS32(md, PS_LIST_TAIL, "n_psfstars", 0, NULL, -32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddS32(md, PS_LIST_TAIL, "n_iqstars", 0, NULL, -32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddS32(md, PS_LIST_TAIL, "n_extended", 0, NULL, -32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddS32(md, PS_LIST_TAIL, "n_cr", 0, NULL, -32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddS32(md, PS_LIST_TAIL, "n_astrom", 0, NULL, -32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "path_base", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = camProcessedExpObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->uri, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bg == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bg_stdev == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bg_mean_stdev == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bias == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bias_stdev == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->fringe_0 == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->fringe_1 == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->fringe_2 == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->sigma_ra == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->sigma_dec == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->ap_resid == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->ap_resid_stdev == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->zp_mean == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->zp_stdev == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->fwhm_major == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->fwhm_minor == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iq_m2 == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iq_m2_err == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iq_m2_lq == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iq_m2_uq == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iq_m2c == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iq_m2c_err == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iq_m2c_lq == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iq_m2c_uq == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iq_m2s == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iq_m2s_err == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iq_m2s_lq == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iq_m2s_uq == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iq_m3 == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iq_m3_err == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iq_m3_lq == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iq_m3_uq == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iq_m4 == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iq_m4_err == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iq_m4_lq == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iq_m4_uq == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->dtime_script == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->dtime_astrom == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->dtime_addstar == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->hostname, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->n_stars == -32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->n_psfstars == -32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->n_iqstars == -32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->n_extended == -32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->n_cr == -32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->n_astrom == -32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->path_base, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        camMaskRow      *object;

        md = psMetadataAlloc();
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "label", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = camMaskObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

        if (strncmp(object->label, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        fakeRunRow      *object;

        md = psMetadataAlloc();
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "state", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "workdir", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "label", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "reduction", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "expgroup", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "dvodb", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "tess_id", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "end_stage", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = fakeRunObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->state, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->workdir, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->label, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->reduction, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->expgroup, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->dvodb, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->tess_id, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->end_stage, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        fakeProcessedImfileRow *object;

        md = psMetadataAlloc();
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "class_id", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "uri", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "dtime_fake", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "dtime_script", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "hostname", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "path_base", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "data_state", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = fakeProcessedImfileObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->class_id, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->uri, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->dtime_fake == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->dtime_script == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->hostname, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->path_base, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->data_state, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        fakeMaskRow     *object;

        md = psMetadataAlloc();
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "label", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = fakeMaskObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

        if (strncmp(object->label, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        warpRunRow      *object;

        md = psMetadataAlloc();
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "mode", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "state", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "workdir", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "workdir_state", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "label", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "dvodb", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "tess_id", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "end_stage", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAdd(md, PS_LIST_TAIL, "magiced", PS_DATA_BOOL, NULL, true)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = warpRunObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->mode, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->state, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->workdir, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->workdir_state, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->label, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->dvodb, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->tess_id, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->end_stage, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->magiced == true) {
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        warpSkyCellMapRow *object;

        md = psMetadataAlloc();
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "skycell_id", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "tess_id", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "class_id", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = warpSkyCellMapObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->skycell_id, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->tess_id, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->class_id, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        warpSkyfileRow  *object;

        md = psMetadataAlloc();
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "skycell_id", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "tess_id", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "uri", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "path_base", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "data_state", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "bg", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "bg_stdev", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "dtime_warp", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "dtime_script", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "hostname", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "good_frac", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddS32(md, PS_LIST_TAIL, "xmin", 0, NULL, -32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddS32(md, PS_LIST_TAIL, "xmax", 0, NULL, -32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddS32(md, PS_LIST_TAIL, "ymin", 0, NULL, -32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddS32(md, PS_LIST_TAIL, "ymax", 0, NULL, -32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAdd(md, PS_LIST_TAIL, "ignored", PS_DATA_BOOL, NULL, true)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = warpSkyfileObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->skycell_id, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->tess_id, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->uri, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->path_base, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->data_state, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bg == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bg_stdev == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->dtime_warp == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->dtime_script == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->hostname, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->good_frac == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->xmin == -32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->xmax == -32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->ymin == -32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->ymax == -32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->ignored == true) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        warpMaskRow     *object;

        md = psMetadataAlloc();
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "label", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = warpMaskObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

        if (strncmp(object->label, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        diffRunRow      *object;

        md = psMetadataAlloc();
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "state", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "label", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "reduction", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "workdir", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "dvodb", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "skycell_id", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "tess_id", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = diffRunObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->state, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->label, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->reduction, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->workdir, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->dvodb, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->skycell_id, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->tess_id, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        diffInputSkyfileRow *object;

        md = psMetadataAlloc();
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAdd(md, PS_LIST_TAIL, "template", PS_DATA_BOOL, NULL, true)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "skycell_id", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "tess_id", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "kind", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = diffInputSkyfileObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->template == true) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->skycell_id, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->tess_id, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->kind, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        diffSkyfileRow  *object;

        md = psMetadataAlloc();
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "uri", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "path_base", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "bg", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "bg_stdev", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddS32(md, PS_LIST_TAIL, "stamps_num", 0, NULL, -32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "stamps_mean", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "stamps_rms", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "norm", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "bg_diff", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "kernel_x", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "kernel_y", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "kernel_xx", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "kernel_xy", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "kernel_yy", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddS32(md, PS_LIST_TAIL, "sources", 0, NULL, -32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "dtime_diff", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "dtime_match", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "dtime_phot", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "dtime_script", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "hostname", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "good_frac", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = diffSkyfileObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->uri, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->path_base, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bg == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bg_stdev == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->stamps_num == -32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->stamps_mean == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->stamps_rms == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->norm == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bg_diff == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->kernel_x == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->kernel_y == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->kernel_xx == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->kernel_xy == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->kernel_yy == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->sources == -32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->dtime_diff == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->dtime_match == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->dtime_phot == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->dtime_script == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->hostname, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->good_frac == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        stackRunRow     *object;

        md = psMetadataAlloc();
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "state", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "workdir", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "label", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "reduction", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "dvodb", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "skycell_id", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "tess_id", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "filter", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = stackRunObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->state, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->workdir, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->label, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->reduction, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->dvodb, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->skycell_id, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->tess_id, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->filter, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        stackInputSkyfileRow *object;

        md = psMetadataAlloc();
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = stackInputSkyfileObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        stackSumSkyfileRow *object;

        md = psMetadataAlloc();
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "uri", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "path_base", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "bg", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "bg_stdev", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "dtime_stack", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "dtime_match_mean", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "dtime_match_stdev", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "dtime_initial", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "dtime_reject", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "dtime_final", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "dtime_phot", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "dtime_script", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "match_mean", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "match_stdev", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "match_rms", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "stamps_mean", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "stamps_stdev", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddS32(md, PS_LIST_TAIL, "stamps_min", 0, NULL, -32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddS32(md, PS_LIST_TAIL, "reject_images", 0, NULL, -32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "reject_pix_mean", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "reject_pix_stdev", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddS32(md, PS_LIST_TAIL, "sources", 0, NULL, -32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "hostname", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "good_frac", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = stackSumSkyfileObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->uri, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->path_base, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bg == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bg_stdev == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->dtime_stack == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->dtime_match_mean == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->dtime_match_stdev == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->dtime_initial == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->dtime_reject == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->dtime_final == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->dtime_phot == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->dtime_script == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->match_mean == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->match_stdev == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->match_rms == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->stamps_mean == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->stamps_stdev == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->stamps_min == -32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->reject_images == -32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->reject_pix_mean == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->reject_pix_stdev == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->sources == -32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->hostname, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->good_frac == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        detRunRow       *object;

        md = psMetadataAlloc();
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddS32(md, PS_LIST_TAIL, "iteration", 0, NULL, -32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "det_type", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "mode", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "state", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "filelevel", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "workdir", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "camera", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "telescope", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "exp_type", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "reduction", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "filter", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "airmass_min", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "airmass_max", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "exp_time_min", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "exp_time_max", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "ccd_temp_min", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "ccd_temp_max", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "posang_min", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "posang_max", 0, NULL, 64.64)) {
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
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "solang_min", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "solang_max", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "label", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddS32(md, PS_LIST_TAIL, "ref_iter", 0, NULL, -32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = detRunObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iteration == -32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->det_type, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->mode, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->state, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->filelevel, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->workdir, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->camera, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->telescope, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->exp_type, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->reduction, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->filter, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->airmass_min == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->airmass_max == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->exp_time_min == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->exp_time_max == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->ccd_temp_min == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->ccd_temp_max == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->posang_min == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->posang_max == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->solang_min == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->solang_max == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->label, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->ref_iter == -32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        detInputExpRow  *object;

        md = psMetadataAlloc();
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddS32(md, PS_LIST_TAIL, "iteration", 0, NULL, -32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAdd(md, PS_LIST_TAIL, "include", PS_DATA_BOOL, NULL, true)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = detInputExpObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iteration == -32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->include == true) {
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        detProcessedImfileRow *object;

        md = psMetadataAlloc();
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "class_id", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "uri", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "recipe", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "bg", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "bg_stdev", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "bg_mean_stdev", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "fringe_0", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "fringe_1", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "fringe_2", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "user_1", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "user_2", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "user_3", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "user_4", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "user_5", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "path_base", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "data_state", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = detProcessedImfileObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->class_id, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->uri, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->recipe, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bg == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bg_stdev == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bg_mean_stdev == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->fringe_0 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->fringe_1 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->fringe_2 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->user_1 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->user_2 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->user_3 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->user_4 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->user_5 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->path_base, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->data_state, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        detProcessedExpRow *object;

        md = psMetadataAlloc();
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "recipe", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "bg", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "bg_stdev", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "bg_mean_stdev", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "fringe_0", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "fringe_1", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "fringe_2", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "user_1", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "user_2", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "user_3", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "user_4", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "user_5", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "path_base", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "data_state", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = detProcessedExpObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->recipe, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bg == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bg_stdev == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bg_mean_stdev == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->fringe_0 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->fringe_1 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->fringe_2 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->user_1 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->user_2 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->user_3 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->user_4 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->user_5 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->path_base, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->data_state, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        detStackedImfileRow *object;

        md = psMetadataAlloc();
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddS32(md, PS_LIST_TAIL, "iteration", 0, NULL, -32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "class_id", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "uri", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "recipe", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "bg", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "bg_stdev", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "bg_mean_stdev", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "user_1", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "user_2", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "user_3", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "user_4", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "user_5", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "data_state", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = detStackedImfileObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iteration == -32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->class_id, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->uri, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->recipe, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bg == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bg_stdev == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bg_mean_stdev == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->user_1 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->user_2 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->user_3 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->user_4 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->user_5 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->data_state, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        detNormalizedStatImfileRow *object;

        md = psMetadataAlloc();
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddS32(md, PS_LIST_TAIL, "iteration", 0, NULL, -32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "class_id", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF32(md, PS_LIST_TAIL, "norm", 0, NULL, 32.32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "data_state", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = detNormalizedStatImfileObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iteration == -32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->class_id, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->norm == 32.32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->data_state, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        detNormalizedImfileRow *object;

        md = psMetadataAlloc();
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddS32(md, PS_LIST_TAIL, "iteration", 0, NULL, -32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "class_id", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "uri", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "bg", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "bg_stdev", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "bg_mean_stdev", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "user_1", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "user_2", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "user_3", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "user_4", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "user_5", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "path_base", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "data_state", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = detNormalizedImfileObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iteration == -32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->class_id, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->uri, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bg == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bg_stdev == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bg_mean_stdev == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->user_1 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->user_2 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->user_3 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->user_4 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->user_5 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->path_base, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->data_state, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        detNormalizedExpRow *object;

        md = psMetadataAlloc();
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddS32(md, PS_LIST_TAIL, "iteration", 0, NULL, -32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "recipe", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "bg", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "bg_stdev", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "bg_mean_stdev", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "user_1", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "user_2", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "user_3", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "user_4", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "user_5", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "path_base", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "data_state", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = detNormalizedExpObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iteration == -32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->recipe, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bg == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bg_stdev == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bg_mean_stdev == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->user_1 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->user_2 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->user_3 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->user_4 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->user_5 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->path_base, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->data_state, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        detResidImfileRow *object;

        md = psMetadataAlloc();
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddS32(md, PS_LIST_TAIL, "iteration", 0, NULL, -32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddS32(md, PS_LIST_TAIL, "ref_iter", 0, NULL, -32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "class_id", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "uri", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "recipe", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "bg", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "bg_stdev", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "bg_mean_stdev", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "bg_skewness", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "bg_kurtosis", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "bin_stdev", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "fringe_0", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "fringe_1", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "fringe_2", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "fringe_resid_0", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "fringe_resid_1", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "fringe_resid_2", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "user_1", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "user_2", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "user_3", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "user_4", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "user_5", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "path_base", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "data_state", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = detResidImfileObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iteration == -32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->ref_iter == -32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->class_id, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->uri, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->recipe, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bg == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bg_stdev == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bg_mean_stdev == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bg_skewness == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bg_kurtosis == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bin_stdev == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->fringe_0 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->fringe_1 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->fringe_2 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->fringe_resid_0 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->fringe_resid_1 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->fringe_resid_2 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->user_1 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->user_2 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->user_3 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->user_4 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->user_5 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->path_base, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->data_state, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        detResidExpRow  *object;

        md = psMetadataAlloc();
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddS32(md, PS_LIST_TAIL, "iteration", 0, NULL, -32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "recipe", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "bg", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "bg_stdev", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "bg_mean_stdev", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "bg_skewness", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "bg_kurtosis", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "bin_stdev", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "fringe_0", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "fringe_1", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "fringe_2", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "fringe_resid_0", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "fringe_resid_1", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "fringe_resid_2", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "user_1", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "user_2", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "user_3", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "user_4", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "user_5", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "path_base", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "data_state", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAdd(md, PS_LIST_TAIL, "accept", PS_DATA_BOOL, NULL, true)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = detResidExpObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iteration == -32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->recipe, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bg == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bg_stdev == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bg_mean_stdev == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bg_skewness == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bg_kurtosis == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bin_stdev == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->fringe_0 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->fringe_1 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->fringe_2 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->fringe_resid_0 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->fringe_resid_1 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->fringe_resid_2 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->user_1 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->user_2 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->user_3 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->user_4 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->user_5 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->path_base, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->data_state, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->accept == true) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        detRunSummaryRow *object;

        md = psMetadataAlloc();
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddS32(md, PS_LIST_TAIL, "iteration", 0, NULL, -32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "data_state", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "bg", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "bg_stdev", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "bg_mean_stdev", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAdd(md, PS_LIST_TAIL, "accept", PS_DATA_BOOL, NULL, true)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = detRunSummaryObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iteration == -32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->data_state, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bg == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bg_stdev == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bg_mean_stdev == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->accept == true) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        detRegisteredImfileRow *object;

        md = psMetadataAlloc();
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddS32(md, PS_LIST_TAIL, "iteration", 0, NULL, -32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "class_id", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "uri", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "bg", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "bg_stdev", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "bg_mean_stdev", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "user_1", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "user_2", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "user_3", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "user_4", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddF64(md, PS_LIST_TAIL, "user_5", 0, NULL, 64.64)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "path_base", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "data_state", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = detRegisteredImfileObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iteration == -32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->class_id, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->uri, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bg == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bg_stdev == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->bg_mean_stdev == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->user_1 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->user_2 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->user_3 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->user_4 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->user_5 == 64.64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->path_base, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->data_state, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        detCorrectedExpRow *object;

        md = psMetadataAlloc();
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "uri", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "corr_type", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "recipe", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "path_base", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = detCorrectedExpObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->uri, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->corr_type, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->recipe, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->path_base, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        detCorrectedImfileRow *object;

        md = psMetadataAlloc();
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "class_id", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "uri", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "path_base", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = detCorrectedImfileObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->class_id, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->uri, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->path_base, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        magicRunRow     *object;

        md = psMetadataAlloc();
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "state", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "workdir", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "workdir_state", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "label", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "dvodb", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = magicRunObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->state, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->workdir, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->workdir_state, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->label, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->dvodb, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        magicInputSkyfileRow *object;

        md = psMetadataAlloc();
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "node", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = magicInputSkyfileObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->node, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        magicTreeRow    *object;

        md = psMetadataAlloc();
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "node", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "dep", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = magicTreeObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->node, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->dep, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        magicNodeResultRow *object;

        md = psMetadataAlloc();
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "node", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "uri", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = magicNodeResultObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->node, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->uri, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        magicMaskRow    *object;

        md = psMetadataAlloc();
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "uri", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddS32(md, PS_LIST_TAIL, "streaks", 0, NULL, -32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = magicMaskObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->uri, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->streaks == -32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        calDBRow        *object;

        md = psMetadataAlloc();
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "dvodb", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "state", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = calDBObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->dvodb, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->state, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        calRunRow       *object;

        md = psMetadataAlloc();
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "region", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "last_step", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "state", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = calRunObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->region, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->last_step, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->state, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        flatcorrRunRow  *object;

        md = psMetadataAlloc();
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "dvodb", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "filter", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "state", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "workdir", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "label", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "reduction", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "region", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "hostname", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = flatcorrRunObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->dvodb, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->filter, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->state, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->workdir, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->label, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->reduction, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->region, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->hostname, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        flatcorrChipLinkRow *object;

        md = psMetadataAlloc();
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = flatcorrChipLinkObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        flatcorrCamLinkRow *object;

        md = psMetadataAlloc();
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = flatcorrCamLinkObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        pstampDataStoreRow *object;

        md = psMetadataAlloc();
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "state", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "lastFileset", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "outProduct", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "uri", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = pstampDataStoreObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->state, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->lastFileset, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->outProduct, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->uri, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        pstampProjectRow *object;

        md = psMetadataAlloc();
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "name", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "state", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "dbname", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "dvodb", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "camera", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "telescope", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAdd(md, PS_LIST_TAIL, "need_magic", PS_DATA_BOOL, NULL, true)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = pstampProjectObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->name, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->state, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->dbname, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->dvodb, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->camera, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->telescope, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->need_magic == true) {
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        pstampRequestRow *object;

        md = psMetadataAlloc();
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "state", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "name", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "reqType", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "outProduct", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "uri", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddS32(md, PS_LIST_TAIL, "fault", 0, NULL, -32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = pstampRequestObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->state, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->name, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->reqType, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->outProduct, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->uri, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->fault == -32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        psMetadata      *md;
        pstampJobRow    *object;

        md = psMetadataAlloc();
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "rownum", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "state", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "jobType", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddS32(md, PS_LIST_TAIL, "fault", 0, NULL, -32)) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "uri", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "outputBase", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }
        if (!psMetadataAddStr(md, PS_LIST_TAIL, "args", 0, NULL, "a string")) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        object = pstampJobObjectFromMetadata(md);
        if (!object) {
            psFree(md);
            exit(EXIT_FAILURE);
        }

        psFree(md);

            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->rownum, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->state, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->jobType, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->fault == -32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->uri, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->outputBase, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->args, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    exit(EXIT_SUCCESS);
}
