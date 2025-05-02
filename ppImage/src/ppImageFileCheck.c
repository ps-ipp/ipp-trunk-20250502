#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ppImage.h"

void ppImageFileCheck (pmConfig *config) {

    // add the output names to the output-type files
    psMetadataItem *item = NULL;
    psMetadataIterator *iter = psMetadataIteratorAlloc (config->files, PS_LIST_HEAD, NULL);
    while ((item = psMetadataGetAndIncrement (iter)) != NULL) {
        pmFPAfile *file = item->data.V;
        pmFPA *fpa = file->fpa;
        fprintf (stderr, "file %s\n", file->name);
        if (!fpa) {
            fprintf (stderr, "  has no FPA\n");
            continue;
        }
        if (fpa->hdu) {
            if (fpa->hdu->images) fprintf (stderr, "  (%d,%d) images\n", -1, -1);
            if (fpa->hdu->variances) fprintf (stderr, "  (%d,%d) variances\n", -1, -1);
            if (fpa->hdu->masks) fprintf (stderr, "  (%d,%d) masks\n", -1, -1);
            if (fpa->hdu->header) fprintf (stderr, "  (%d,%d) header\n", -1, -1);
        } else {
            // fprintf (stderr, "  has no fpa data (%d,%d)\n", -1, -1);
        }
        for (int i = 0; i < fpa->chips->n; i++) {
            pmChip *chip = fpa->chips->data[i];
            if (chip->hdu) {
                if (chip->hdu->images) fprintf (stderr, "  (%d,%d) images\n", i, -1);
                if (chip->hdu->variances) fprintf (stderr, "  (%d,%d) variances\n", i, -1);
                if (chip->hdu->masks) fprintf (stderr, "  (%d,%d) masks\n", i, -1);
                if (chip->hdu->header) fprintf (stderr, "  (%d,%d) header\n", i, -1);
            } else {
                // fprintf (stderr, "  has no chip data (%d,%d)\n", i, -1);
            }
            for (int j = 0; j < chip->cells->n; j++) {
                pmCell *cell = chip->cells->data[j];
                if (cell->hdu) {
                    if (cell->hdu->images) fprintf (stderr, "  (%d,%d) images\n", i, j);
                    if (cell->hdu->variances) fprintf (stderr, "  (%d,%d) variances\n", i, j);
                    if (cell->hdu->masks) fprintf (stderr, "  (%d,%d) masks\n", i, j);
                    if (cell->hdu->header) fprintf (stderr, "  (%d,%d) header\n", i, j);
                } else {
                    // fprintf (stderr, "  has no cell data (%d,%d)\n", i, j);
                }
                for (int k = 0; k < cell->readouts->n; k++) {
                    pmReadout *readout = cell->readouts->data[k];
                    if (readout) {
                        if (readout->image) fprintf (stderr, "  (%d,%d,%d) image\n", i, j, k);
                        if (readout->variance) fprintf (stderr, "  (%d,%d,%d) variance\n", i, j, k);
                        if (readout->mask) fprintf (stderr, "  (%d,%d,%d) masks\n", i, j, k);
                    }
                }
            }
        }
    }
    psFree (iter);
}
