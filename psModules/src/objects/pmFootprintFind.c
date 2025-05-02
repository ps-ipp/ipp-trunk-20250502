/* @file  pmFootprintFind.c
 * find footprints in an image (fast on large scale images)
 *
 * @author RHL, Princeton & IfA; EAM, IfA
 *
 * @version $Revision: 1.8 $ $Name: not supported by cvs2svn $
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

// XXX EAM : why use WSPAN in here rather than pmSpan?
// XXX WES : can't use pmSpan because does not have an id
typedef struct {                        /* run-length code for part of object*/
   int id;                              /* ID for object */
   int y;                               /* Row wherein WSPAN dwells */
   int x0, x1;                          /* inclusive range of columns */
} WSPAN;

/*
 * comparison function for qsort; sort by ID then row
 */
static int
compar(const void *va, const void *vb)
{
   const WSPAN *a = va;
   const WSPAN *b = vb;

   if(a->id < b->id) {
      return(-1);
   } else if(a->id > b->id) {
      return(1);
   } else {
      return(a->y - b->y);
   }
}

/*
 * Follow a chain of aliases, returning the final resolved value.
 */
static int
resolve_alias(const int *aliases,       /* list of aliases */
              int id)                   /* alias to look up */
{
   int resolved = id;                   /* resolved alias */

   while(id != aliases[id]) {
      resolved = id = aliases[id];
   }

   return(resolved);
}

/*
 * Go through an image, finding sets of connected pixels above threshold
 * and assembling them into pmFootprints;  the resulting set of objects
 * is returned as a psArray
 */
psArray *
pmFootprintsFind(const psImage *img,    // image to search
                 const float threshold, // Threshold
                 const int npixMin)     // minimum number of pixels in an acceptable pmFootprint
{
   int i0;                              /* initial value of i */
   int id;                              /* object ID */
   int in_span;                         /* object ID of current WSPAN */
   int nspan = 0;                       /* number of spans */
   int nobj = 0;                        /* number of objects found */
   int x0 = 0;                          /* unpacked from a WSPAN */
   int *tmp;                            /* used in swapping idc/idp */

   assert(img != NULL);

   psImage *floatImg = img->type.type == PS_TYPE_F32 ? psMemIncrRefCounter((psImage*)img) :
       psImageCopy(NULL, img, PS_TYPE_F32); // Floating-point version of image; casting away const

   const int row0 = img->row0;
   const int col0 = img->col0;
   const int numRows = img->numRows;
   const int numCols = img->numCols;
/*
 * Storage for arrays that identify objects by ID. We want to be able to
 * refer to idp[-1] and idp[numCols], hence the (numCols + 2)
 */
   int *id_s = psAlloc(2*(numCols + 2)*sizeof(int));
   memset(id_s, '\0', 2*(numCols + 2)*sizeof(int)); assert(id_s[0] == 0);
   int *idc = id_s + 1;                 // object IDs in current/
   int *idp = idc + (numCols + 2);      //                       previous row

   int size_aliases = 1 + numRows/20;   // size of aliases[] array
   int *aliases = psAlloc(size_aliases*sizeof(int)); // aliases for object IDs

   int size_spans = 1 + numRows/20;     // size of spans[] array
   WSPAN *spans = psAlloc(size_spans*sizeof(WSPAN)); // row:x0,x1 for objects
/*
 * Go through image identifying objects
 */
   for (int i = 0; i < numRows; i++) {
       int j;
       tmp = idc; idc = idp; idp = tmp;  /* swap ID pointers */
       memset(idc, '\0', numCols*sizeof(int));

       in_span = 0;                      /* not in a span */
       int id_last_connection = 0;
       for (j = 0; j < numCols; j++) {
           double pixVal = floatImg->data.F32[i][j]; // Value of pixel
           // If pixVal is less than threshold and we are working on a, span end it.
           if (pixVal < threshold) {
               // below threshold. If in a span close it out
               if (in_span) {
                   if(nspan >= size_spans) {
                       size_spans *= 2;
                       spans = psRealloc(spans, size_spans*sizeof(WSPAN));
                   }
                   spans[nspan].id = in_span;
                   spans[nspan].y = i;
                   spans[nspan].x0 = x0;
                   spans[nspan].x1 = j - 1;

                   nspan++;

                   in_span = 0;
                   id_last_connection = 0;
               }
           } else {                       /* a pixel to fix */
               // Above theshold. There are 5 choices for the id of this pixel based on whether they are
               // part of a span (non-zero)
               // This diagram shows the priority which we check
               //       
               //                       col
               //                   j-1   j   j+1
               // row i              1    5 
               // row i + 1          2    3    4
               //
               // In case 4 we have a pixel that is not connected to the left are connecting with 
               // an existing span so need to identify whether it is connected
               // to the same footprint as the current span (if we are in one)
               if(idc[j - 1] != 0) {
                   id = idc[j - 1];
               } else if(idp[j - 1] != 0) {
                   id = idp[j - 1];
               } else if(idp[j] != 0) {
                   id = idp[j];
               } else if(idp[j + 1] != 0) {
                   id = idp[j + 1];
               } else {
                   id = ++nobj;

                   if(id >= size_aliases) {
                       size_aliases *= 2;
                       aliases = psRealloc(aliases, size_aliases*sizeof(int));
                   }
                   aliases[id] = id;
               }

               idc[j] = id;
               if(!in_span) {
                   x0 = j; in_span = id;
               }
               /*
                * Do we need to merge ID numbers? If so, make suitable entries in aliases[]
                */
               if (idp[j + 1] != 0 && idp[j + 1] != id && idp[j + 1] != id_last_connection) {
                   int resolved_lower_right = resolve_alias(aliases, idp[j + 1]);
                   int resolved_current = resolve_alias(aliases, id);
                   aliases[resolved_lower_right] = resolved_current;

                   // now we choose the id to continue to use to set pixels in the current span.
                   // We choose the higher value because future alias resolutions will be faster
                   // since the alias chain goes from lower ids to higher. This is about 4 times
                   // faster for complex footprints.
                   if (resolved_current <= idp[j + 1]) {
                       idc[j] = id = idp[j + 1];
                       id_last_connection = 0;
                   } else {
                       idc[j] = id = resolved_current;
                       id_last_connection = idp[j + 1];
                   }
               }
           }
       }

       if(in_span) {
           if(nspan >= size_spans) {
               size_spans *= 2;
               spans = psRealloc(spans, size_spans*sizeof(WSPAN));
           }

           assert(nspan < size_spans);    /* we checked for space above */
           spans[nspan].id = in_span;
           spans[nspan].y = i;
           spans[nspan].x0 = x0;
           spans[nspan].x1 = j - 1;

           nspan++;
       }
   }
   psFree(floatImg);

   psFree(id_s);
   /*
    * Resolve aliases; first alias chains, then the IDs in the spans
    */
   for (int i = 0; i < nspan; i++) {
       spans[i].id = resolve_alias(aliases, spans[i].id);
   }

   psFree(aliases);
   /*
    * Sort spans by ID, so we can sweep through them once
    * XXX replace with a psLib sort call
    */
   if(nspan > 0) {
       qsort(spans, nspan, sizeof(WSPAN), compar);
   }
   /*
    * Build pmFootprints from the spans
    */
   psArray *footprints = psArrayAlloc(nobj);
   int n = 0;                   // number of pmFootprints

   if(nspan > 0) {
       id = spans[0].id;
       i0 = 0;
       for (int i = 0; i <= nspan; i++) {        /* nspan + 1 to catch last object */
           if(i == nspan || spans[i].id != id) {
               pmFootprint *fp = pmFootprintAlloc(i - i0, img);

               for(; i0 < i; i0++) {
                   pmFootprintAddSpan(fp, spans[i0].y + row0,
                                      spans[i0].x0 + col0, spans[i0].x1 + col0);
               }

               if (fp->npix < npixMin) {
                   psFree(fp);
               } else {
                   footprints->data[n++] = fp;
               }
           }

           id = spans[i].id;
       }
   }

   footprints = psArrayRealloc(footprints, n);
   footprints->n = n;
   /*
    * clean up
    */
   psFree(spans);

   return footprints;
}
