/* @file  pmFootprintArraysMerge.c
 *
 * @author RHL, Princeton & IfA; EAM, IfA
 *
 * @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
 * @date $Date: 2008-12-08 02:51:14 $
 * Copyright 2006 Institute for Astronomy, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <strings.h>
#include <pslib.h>
#include "pmSpan.h"
#include "pmFootprintSpans.h"
#include "pmFootprint.h"
#include "pmPeaks.h"

/*
 * Merge together two psArrays of pmFootprints neither of which is damaged.
 *
 * The returned psArray may contain elements of the inital psArrays (with
 * their reference counters suitable incremented)
 */
psArray *pmFootprintArraysMerge(const psArray *footprints1, // one set of footprints
                                const psArray *footprints2, // the other set
                                const int includePeaks // which peaks to set? 0x1 => footprints1, 0x2 => 2
    )
{
    if (!footprints1 && !footprints2) {
        // No footprints in merged array
        return psArrayAllocEmpty(0);
    }

    assert(!footprints1 || footprints1->n == 0 || pmFootprintTest(footprints1->data[0]));
    assert(!footprints2 || footprints2->n == 0 || pmFootprintTest(footprints2->data[0]));

    if (!footprints1 || footprints1->n == 0 || !footprints2 || footprints2->n == 0) {
        // nothing to do but put copies on merged
        const psArray *old = (!footprints1 || footprints1->n == 0) ? footprints2 : footprints1;

        psArray *merged = psArrayAllocEmpty(old->n);
        for (int i = 0; i < old->n; i++) {
            psArrayAdd(merged, 1, old->data[i]);
        }

        return merged;
    }
    /*
     * We have real work to do as some pmFootprints in footprints2 may overlap
     * with footprints1
     */
    {
        pmFootprint *fp1 = footprints1->data[0];
        pmFootprint *fp2 = footprints2->data[0];
        if (fp1->region.x0 != fp2->region.x0 ||
            fp1->region.x1 != fp2->region.x1 ||
            fp1->region.y0 != fp2->region.y0 ||
            fp1->region.y1 != fp2->region.y1) {
            psError(PS_ERR_BAD_PARAMETER_SIZE, true,
                    "The two pmFootprint arrays correspnond to different-sized regions");
            return NULL;
        }
    }
    /*
     * We'll insert first one set of footprints then the other into an image, then
     * extract a footprint from the result --- this is magically what we want.
     */
    psImage *idImage = pmSetFootprintArrayIDs(footprints1, true);
    pmSetFootprintArrayIDsForImage(idImage, footprints2, true);

    psArray *merged = pmFootprintsFind(idImage, 0.5, 1);
    assert (merged != NULL);
    psFree(idImage);
    /*
     * Now assign the peaks appropriately.  We could do this more efficiently
     * using idImage (which we just freed), but this is easy and probably fast enough
     */
    if (includePeaks & 0x1) {
        psArray *peaks = pmFootprintArrayToPeaks(footprints1);
        pmFootprintsAssignPeaks(merged, peaks);
        psFree(peaks);
    }

    if (includePeaks & 0x2) {
        psArray *peaks = pmFootprintArrayToPeaks(footprints2);
        pmFootprintsAssignPeaks(merged, peaks);
        psFree(peaks);
    }

    return merged;
}
