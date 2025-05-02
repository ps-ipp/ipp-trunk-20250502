# include "skycells.h"
# include <libgen.h>

void skycells_to_mdc(FILE *mdcfile, int simple, char * tess_id, FITS_DB *db) {
    // Convert FITS_DB to images structure
    off_t NdbImages;
    Image *dbImages = gfits_table_get_Image(&db->ftable, &NdbImages, &db->scaledValue, &db->nativeOrder);

    // If tess_id was not supplied assume that it is the basename of the CATDIR
    if (tess_id == NULL) {
        // basename modifies string so make a copy
        char *catdir = strdup(CATDIR);
        tess_id = basename(catdir);
    }

    CoordTransform *celestial_to_galactic = InitTransform(COORD_CELESTIAL, COORD_GALACTIC);

    if (!simple) {
        fprintf(mdcfile, "Skycell MULTI\n");
    }

    int i;
    for (i = 0; i < NdbImages; i++) {
        Image *skycell = &dbImages[i];
        double xCenter = skycell->NX / 2.;
        double yCenter = skycell->NY / 2.;
        double raCenter, decCenter;

        if (!XY_to_RD(&raCenter, &decCenter, xCenter, yCenter, &skycell->coords)) {
            fprintf(stderr, "failed to transform center of image %d\n", (int) i);
            exit(1);
        }

        double glonCenter;
        double glatCenter;
        ApplyTransform(&glonCenter, &glatCenter, raCenter, decCenter, celestial_to_galactic);

        // width and height of skycell in degrees
        double width  = skycell->coords.cdelt1 * skycell->NX;
        double height = skycell->coords.cdelt2 * skycell->NY;

        if (simple) {
            fprintf(mdcfile, "%s %s %f %f %f %f %f %f\n", tess_id, skycell->name, raCenter, decCenter, glonCenter, glatCenter, width, height);
        } else {
            fprintf(mdcfile, "\nSkycell METADATA\n");
            fprintf(mdcfile, "    tess_id\tSTR\t%s\n", tess_id);
            fprintf(mdcfile, "    skycell_id\tSTR\t%s\n", skycell->name);
            fprintf(mdcfile, "    radeg\tF32\t%f\n", raCenter);
            fprintf(mdcfile, "    decdeg\tF32\t%f\n", decCenter);
            fprintf(mdcfile, "    glong\tF32\t%f\n", glonCenter);
            fprintf(mdcfile, "    glat\tF32\t%f\n", glatCenter);
            fprintf(mdcfile, "    width\tF32\t%f\n", width);
            fprintf(mdcfile, "    height\tF32\t%f\n", height);
            fprintf(mdcfile, "END\n");
        }

    }
}
