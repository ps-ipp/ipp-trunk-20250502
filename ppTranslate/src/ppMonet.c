#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <zlib.h>
#include <pslib.h>

#include "ppTranslateVersion.h"
#include "ppMonet.h"

int main(int argc, char *argv[])
{
    psLibInit(NULL);

    ppMonetArguments *args = ppMonetArgumentsParse(argc, argv); // Parsed arguments
    if (!args) {
        psErrorStackPrint(stderr, "Error parsing arguments");
        exit(PS_EXIT_CONFIG_ERROR);
    }

    psTrace("ppMonet.read", 1, "Reading input detections\n");

    psFits *fits = psFitsOpen(args->input, "r"); // FITS file
    if (!fits) {
        psError(PS_ERR_IO, false, "Unable to open input %s", args->input);
        exit(PS_EXIT_SYS_ERROR);
    }

    // Set up output file
    FILE *file = fopen(args->output, "w"); // File handle
    if (!file) {
        psErrorStackPrint(stderr, "Error opening file %s", args->output);
        exit(PS_EXIT_SYS_ERROR);
    }
    gzFile gz = gzdopen(fileno(file), "w"); // Gzip file handle
    gzprintf(gz, "# Pan-STARRS IPP detections for Dave Monet\n");
    gzprintf(gz, "#\n");
    gzprintf(gz, "# Translated from %s\n", args->input);
    psString source = ppTranslateSource(), version = ppTranslateVersion();
    gzprintf(gz, "# S/W source = %s\n", source);
    gzprintf(gz, "# S/W version = %s\n", version);
    gzprintf(gz, "#\n");
    psFree(source);
    psFree(version);
    gzprintf(gz, "# exp_name = %s\n", args->exp_name);
    gzprintf(gz, "# exp_id = %" PRId64 "\n", args->exp_id);
    gzprintf(gz, "# chip_id = %" PRId64 "\n", args->chip_id);
    gzprintf(gz, "# cam_id = %" PRId64 "\n", args->cam_id);
    gzprintf(gz, "# ZP = %f\n", args->zp);
    gzprintf(gz, "# ZP error = %f\n", args->zpErr);
    gzprintf(gz, "# Astrometry rms = %f\n", args->rmsAstrom);

    psFree(args);

    psMetadata *header = psFitsReadHeader(NULL, fits); // Primary header
    if (!header) {
        psError(PS_ERR_IO, false, "Unable to read header");
        exit(PS_EXIT_SYS_ERROR);
    }

    double raBoresight = psMetadataLookupF64(NULL, header, "RA");
    double decBoresight = psMetadataLookupF64(NULL, header, "DEC");
    psString filter = psMemIncrRefCounter(psMetadataLookupStr(NULL, header, "FILTERID"));
    float airmass = psMetadataLookupF32(NULL, header, "AIRMASS");
    float exptime = psMetadataLookupF32(NULL, header, "EXPTIME");
    double posangle = RAD_TO_DEG(psMetadataLookupF64(NULL, header, "POSANGLE"));
    double rotangle = psMetadataLookupF64(NULL, header, "ROTANGLE");
    double alt = psMetadataLookupF64(NULL, header, "ALT");
    double az = psMetadataLookupF64(NULL, header, "AZ");
    double mjd = psMetadataLookupF64(NULL, header, "MJD-OBS");

    gzprintf(gz, "# RA = %.8lf\n", raBoresight);
    gzprintf(gz, "# Dec = %.8lf\n", decBoresight);
    gzprintf(gz, "# Filter = %s\n", filter);
    gzprintf(gz, "# Airmass = %f\n", airmass);
    gzprintf(gz, "# ExpTime = %f\n", exptime);
    gzprintf(gz, "# PA = %lf\n", posangle);
    gzprintf(gz, "# Rotator = %lf\n", rotangle);
    gzprintf(gz, "# Alt = %.8lf\n", alt);
    gzprintf(gz, "# Az = %.8lf\n", az);
    gzprintf(gz, "# MJD = %.6lf\n", mjd);
    gzprintf(gz, "#\n");
    gzprintf(gz, "# chip , x , y , RA , Dec , Mag , MagErr , Width , Flags\n");

    psFree(header);

    int numHDU = psFitsGetSize(fits);   // Number of HDUs
    for (int i = 1; i < numHDU; i++) {
        if (!psFitsMoveExtNum(fits, i, false)) {
            psError(PS_ERR_IO, false, "Unable to move to HDU %d", i);
            exit(PS_EXIT_SYS_ERROR);
        }

        if (psFitsGetExtType(fits) != PS_FITS_TYPE_BINARY_TABLE) {
            psTrace("ppMonet", 1, "Skipping extension %d: not a binary table", i);
            continue;
        }

        psMetadata *header = psFitsReadHeader(NULL, fits); // Primary header
        if (!header) {
            psError(PS_ERR_IO, false, "Unable to read header %d\n", i);
            exit(PS_EXIT_SYS_ERROR);
            return false;
        }

        const char *extname = psMetadataLookupStr(NULL, header, "EXTNAME");
        const char *exttype = extname + strlen(extname) - strlen(EXT_TYPE);
        if (strcmp(exttype, EXT_TYPE) != 0) {
            psTrace("ppMonet", 1, "Skipping extension %d (%s): not correct type", i, extname);
            psFree(header);
            continue;
        }

        long size = psFitsTableSize(fits); // Size of table
        if (size <= 0) {
            psErrorStackPrint(stderr, "Unable to determine size of table %d", i);
            psFree(header);
            exit(PS_EXIT_SYS_ERROR);
        }

        psString chipName = psStringNCopy(extname, strlen(extname) - strlen(EXT_TYPE)); // Name of chip
        psFree(header);

        psTrace("ppMonet.read", 3, "Reading %ld rows from %s\n", size, extname);
        psArray *table = psFitsReadTable(fits); // Table of interest
        if (!table) {
            psError(PS_ERR_IO, false, "Unable to read table %d", i);
            psFree(chipName);
            exit(PS_EXIT_SYS_ERROR);
        }

        psTrace("ppMonet.read", 3, "Writing %ld rows for %s\n", size, chipName);
        for (long j = 0; j < size; j++) {
            psMetadata *row = table->data[j]; // Row of interest

            float x = psMetadataLookupF32(NULL, row, "X_PSF");
            float y = psMetadataLookupF32(NULL, row, "Y_PSF");
            double ra = psMetadataLookupF64(NULL, row, "RA_PSF");
            double dec = psMetadataLookupF64(NULL, row, "DEC_PSF");
            float mag = psMetadataLookupF32(NULL, row, "PSF_INST_MAG");
            float magErr = psMetadataLookupF32(NULL, row, "PSF_INST_MAG_SIG");
            float major = psMetadataLookupF32(NULL, row, "PSF_MAJOR");
            float minor = psMetadataLookupF32(NULL, row, "PSF_MINOR");
            psU32 flags = psMetadataLookupU32(NULL, row, "FLAGS");

            float width = (float) (0.5 * (major + minor));

            gzprintf(gz, "%s , %.2f , %.2f , %.8lf , %.8lf , %.3f , %.3f , %.2f , %#08x\n",
                     chipName, x, y, ra, dec, mag, magErr, width, flags);
        }
        psFree(table);
        psFree(chipName);
    }
    gzclose(gz);
    psFitsClose(fits);
    psFree(filter);

    psTrace("ppMonet.read", 1, "Done reading input detections\n");

    psLibFinalize();

    return PS_EXIT_SUCCESS;
}
