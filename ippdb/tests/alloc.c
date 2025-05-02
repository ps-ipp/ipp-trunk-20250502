#include <pslib.h>
#include <ippdb.h>
#include <stdlib.h>
#include <string.h>

#define MAX_STRING_LENGTH 1024

int main ()
{
    {
        pzDataStoreRow  *object;

        object = pzDataStoreRowAlloc("a string", "a string", "a string", "0001-01-01T00:00:00Z"    );

        if (!object) {
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
        summitExpRow    *object;

        object = summitExpRowAlloc("a string", "a string", "a string", "0001-01-01T00:00:00Z", "a string", "a string", -32, -16, "0001-01-01T00:00:00Z"    );

        if (!object) {
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
        if (!object->fault == -16) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        summitImfileRow *object;

        object = summitImfileRowAlloc("a string", "a string", "a string", "a string", -32, "a string", "a string", "a string", "a string", "0001-01-01T00:00:00Z"    );

        if (!object) {
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
        pzDownloadExpRow *object;

        object = pzDownloadExpRowAlloc("a string", "a string", "a string", "a string", "0001-01-01T00:00:00Z"    );

        if (!object) {
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
        pzDownloadImfileRow *object;

        object = pzDownloadImfileRowAlloc("a string", "a string", "a string", "a string", "a string", "a string", -16, "0001-01-01T00:00:00Z"    );

        if (!object) {
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
        if (!object->fault == -16) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        newExpRow       *object;

        object = newExpRowAlloc(-64, "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "0001-01-01T00:00:00Z"    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!object->exp_id == -64) {
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
        newImfileRow    *object;

        object = newImfileRowAlloc(-64, "a string", "a string", "0001-01-01T00:00:00Z"    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!object->exp_id == -64) {
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
        rawExpRow       *object;

        object = rawExpRowAlloc(-64, "a string", "a string", "a string", "0001-01-01T00:00:00Z", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", 32.32, 64.64, 64.64, 32.32, 32.32, 64.64, 64.64, 64.64, 64.64, 64.64, 32.32, 64.64, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 64.64, 64.64, 64.64, 64.64, 64.64, "a string", 32.32, 32.32, 32.32, 32.32, 32.32, "a string", -16, "0001-01-01T00:00:00Z"    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!object->exp_id == -64) {
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
        if (!object->fault == -16) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        rawImfileRow    *object;

        object = rawImfileRowAlloc(-64, "a string", "a string", "a string", "0001-01-01T00:00:00Z", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", 32.32, 64.64, 64.64, 32.32, 32.32, 64.64, 64.64, 64.64, 64.64, 64.64, 32.32, 64.64, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 64.64, 64.64, 64.64, 64.64, 64.64, "a string", 32.32, 32.32, 32.32, 32.32, 32.32, "a string", -16, "0001-01-01T00:00:00Z"    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!object->exp_id == -64) {
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
        if (!object->fault == -16) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        guidePendingExpRow *object;

        object = guidePendingExpRowAlloc(-64, -64, "a string"    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!object->guide_id == -64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->exp_id == -64) {
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
        chipRunRow      *object;

        object = chipRunRowAlloc(-64, -64, "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string"    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!object->chip_id == -64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->exp_id == -64) {
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
        chipProcessedImfileRow *object;

        object = chipProcessedImfileRowAlloc(-64, -64, "a string", "a string", "a string", 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, "a string", -32, -32, -32, -32, -32, "a string", -16    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!object->chip_id == -64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->exp_id == -64) {
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
        if (!object->fault == -16) {
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        chipMaskRow     *object;

        object = chipMaskRowAlloc("a string"    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (strncmp(object->label, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        camRunRow       *object;

        object = camRunRowAlloc(-64, -64, "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string"    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!object->cam_id == -64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->chip_id == -64) {
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
        camProcessedExpRow *object;

        object = camProcessedExpRowAlloc(-64, "a string", 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, "a string", -32, -32, -32, -32, -32, -32, "a string", -16    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!object->cam_id == -64) {
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
        if (!object->fault == -16) {
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        camMaskRow      *object;

        object = camMaskRowAlloc("a string"    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (strncmp(object->label, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        fakeRunRow      *object;

        object = fakeRunRowAlloc(-64, -64, "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "0001-01-01T00:00:00Z"    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!object->fake_id == -64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->cam_id == -64) {
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
        fakeProcessedImfileRow *object;

        object = fakeProcessedImfileRowAlloc(-64, -64, "a string", "a string", 32.32, 32.32, "a string", "a string", "a string", -16, "0001-01-01T00:00:00Z"    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!object->fake_id == -64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->exp_id == -64) {
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
        if (!object->fault == -16) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        fakeMaskRow     *object;

        object = fakeMaskRowAlloc("a string"    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (strncmp(object->label, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        warpRunRow      *object;

        object = warpRunRowAlloc(-64, -64, "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "0001-01-01T00:00:00Z", true    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!object->warp_id == -64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->fake_id == -64) {
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
        warpSkyCellMapRow *object;

        object = warpSkyCellMapRowAlloc(-64, "a string", "a string", "a string", -16    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!object->warp_id == -64) {
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
        if (!object->fault == -16) {
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        warpSkyfileRow  *object;

        object = warpSkyfileRowAlloc(-64, "a string", "a string", "a string", "a string", "a string", 64.64, 64.64, 32.32, 32.32, "a string", 32.32, -32, -32, -32, -32, true, -16    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!object->warp_id == -64) {
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
        if (!object->fault == -16) {
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        warpMaskRow     *object;

        object = warpMaskRowAlloc("a string"    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (strncmp(object->label, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        diffRunRow      *object;

        object = diffRunRowAlloc(-64, "a string", "a string", "a string", "a string", "a string", "0001-01-01T00:00:00Z", "a string", "a string"    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!object->diff_id == -64) {
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
        diffInputSkyfileRow *object;

        object = diffInputSkyfileRowAlloc(-64, true, -64, -64, "a string", "a string", "a string"    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!object->diff_id == -64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->template == true) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->stack_id == -64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->warp_id == -64) {
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
        diffSkyfileRow  *object;

        object = diffSkyfileRowAlloc(-64, "a string", "a string", 64.64, 64.64, -32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, -32, 32.32, 32.32, 32.32, 32.32, "a string", 32.32, -16    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!object->diff_id == -64) {
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
        if (!object->fault == -16) {
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        stackRunRow     *object;

        object = stackRunRowAlloc(-64, "a string", "a string", "a string", "a string", "a string", "0001-01-01T00:00:00Z", "a string", "a string", "a string"    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!object->stack_id == -64) {
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
        stackInputSkyfileRow *object;

        object = stackInputSkyfileRowAlloc(-64, -64    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!object->stack_id == -64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->warp_id == -64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        stackSumSkyfileRow *object;

        object = stackSumSkyfileRowAlloc(-64, "a string", "a string", 64.64, 64.64, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, -32, -32, 32.32, 32.32, -32, "a string", 32.32, -16    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!object->stack_id == -64) {
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
        if (!object->fault == -16) {
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        detRunRow       *object;

        object = detRunRowAlloc(-64, -32, "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", 32.32, 32.32, 32.32, 32.32, 32.32, 32.32, 64.64, 64.64, "0001-01-01T00:00:00Z", "0001-01-01T00:00:00Z", "0001-01-01T00:00:00Z", "0001-01-01T00:00:00Z", "0001-01-01T00:00:00Z", 32.32, 32.32, "a string", -64, -32    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!object->det_id == -64) {
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
        if (!object->ref_det_id == -64) {
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
        detInputExpRow  *object;

        object = detInputExpRowAlloc(-64, -32, -64, true    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!object->det_id == -64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iteration == -32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->exp_id == -64) {
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
        detProcessedImfileRow *object;

        object = detProcessedImfileRowAlloc(-64, -64, "a string", "a string", "a string", 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, "a string", "a string", -16    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!object->det_id == -64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->exp_id == -64) {
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
        if (!object->fault == -16) {
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        detProcessedExpRow *object;

        object = detProcessedExpRowAlloc(-64, -64, "a string", 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, "a string", "a string", -16    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!object->det_id == -64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->exp_id == -64) {
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
        if (!object->fault == -16) {
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        detStackedImfileRow *object;

        object = detStackedImfileRowAlloc(-64, -32, "a string", "a string", "a string", 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, "a string", -16    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!object->det_id == -64) {
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
        if (!object->fault == -16) {
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        detNormalizedStatImfileRow *object;

        object = detNormalizedStatImfileRowAlloc(-64, -32, "a string", 32.32, "a string", -16    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!object->det_id == -64) {
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
        if (!object->fault == -16) {
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        detNormalizedImfileRow *object;

        object = detNormalizedImfileRowAlloc(-64, -32, "a string", "a string", 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, "a string", "a string", -16    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!object->det_id == -64) {
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
        if (!object->fault == -16) {
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        detNormalizedExpRow *object;

        object = detNormalizedExpRowAlloc(-64, -32, "a string", 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, "a string", "a string", -16    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!object->det_id == -64) {
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
        if (!object->fault == -16) {
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        detResidImfileRow *object;

        object = detResidImfileRowAlloc(-64, -32, -64, -32, -64, "a string", "a string", "a string", 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, "a string", "a string", -16    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!object->det_id == -64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iteration == -32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->ref_det_id == -64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->ref_iter == -32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->exp_id == -64) {
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
        if (!object->fault == -16) {
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        detResidExpRow  *object;

        object = detResidExpRowAlloc(-64, -32, -64, "a string", 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, "a string", "a string", true, -16    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!object->det_id == -64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->iteration == -32) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->exp_id == -64) {
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
        if (!object->fault == -16) {
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        detRunSummaryRow *object;

        object = detRunSummaryRowAlloc(-64, -32, "a string", 64.64, 64.64, 64.64, true, -16    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!object->det_id == -64) {
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
        if (!object->fault == -16) {
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        detRegisteredImfileRow *object;

        object = detRegisteredImfileRowAlloc(-64, -32, "a string", "a string", 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, 64.64, "a string", "a string", -16    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!object->det_id == -64) {
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
        if (!object->fault == -16) {
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        detCorrectedExpRow *object;

        object = detCorrectedExpRowAlloc(-64, -64, "a string", -64, "a string", "a string", "a string", -16    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!object->det_id == -64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->exp_id == -64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (strncmp(object->uri, "a string", MAX_STRING_LENGTH)) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->corr_id == -64) {
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
        if (!object->fault == -16) {
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        detCorrectedImfileRow *object;

        object = detCorrectedImfileRowAlloc(-64, -64, "a string", "a string", "a string", -16    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!object->det_id == -64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->exp_id == -64) {
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
        if (!object->fault == -16) {
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        magicRunRow     *object;

        object = magicRunRowAlloc(-64, -64, "a string", "a string", "a string", "a string", "a string", "0001-01-01T00:00:00Z", -16    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!object->magic_id == -64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->exp_id == -64) {
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
        if (!object->fault == -16) {
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        magicInputSkyfileRow *object;

        object = magicInputSkyfileRowAlloc(-64, -64, "a string"    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!object->magic_id == -64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->diff_id == -64) {
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
        magicTreeRow    *object;

        object = magicTreeRowAlloc(-64, "a string", "a string"    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!object->magic_id == -64) {
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
        magicNodeResultRow *object;

        object = magicNodeResultRowAlloc(-64, "a string", "a string", -16    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!object->magic_id == -64) {
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
        if (!object->fault == -16) {
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        magicMaskRow    *object;

        object = magicMaskRowAlloc(-64, "a string", -32, -16    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!object->magic_id == -64) {
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
        if (!object->fault == -16) {
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        calDBRow        *object;

        object = calDBRowAlloc(-64, "a string", "a string"    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!object->cal_id == -64) {
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
        calRunRow       *object;

        object = calRunRowAlloc(-64, "a string", "a string", "a string"    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!object->cal_id == -64) {
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
        flatcorrRunRow  *object;

        object = flatcorrRunRowAlloc(-64, "a string", "a string", "a string", "a string", "a string", "a string", "a string", "a string", -16    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!object->corr_id == -64) {
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
        if (!object->fault == -16) {
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        flatcorrChipLinkRow *object;

        object = flatcorrChipLinkRowAlloc(-64, -64    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!object->corr_id == -64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->chip_id == -64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        flatcorrCamLinkRow *object;

        object = flatcorrCamLinkRowAlloc(-64, -64, -64    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!object->corr_id == -64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->chip_id == -64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->cam_id == -64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }

        psFree(object);
    }

    {
        pstampDataStoreRow *object;

        object = pstampDataStoreRowAlloc(-64, "a string", "a string", "a string", "a string"    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!object->ds_id == -64) {
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
        pstampProjectRow *object;

        object = pstampProjectRowAlloc(-64, "a string", "a string", "a string", "a string", "a string", "a string", true    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!object->proj_id == -64) {
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
        pstampRequestRow *object;

        object = pstampRequestRowAlloc(-64, -64, "a string", "a string", "a string", "a string", "a string", -32    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!object->req_id == -64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->ds_id == -64) {
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
        pstampJobRow    *object;

        object = pstampJobRowAlloc(-64, -64, "a string", "a string", "a string", -32, "a string", -64, "a string", "a string"    );

        if (!object) {
            exit(EXIT_FAILURE);
        }

        if (!object->job_id == -64) {
            psFree(object);
            exit(EXIT_FAILURE);
        }
        if (!object->req_id == -64) {
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
        if (!object->exp_id == -64) {
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
