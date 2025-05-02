#ifndef PM_SOURCE_MATCH_H
#define PM_SOURCE_MATCH_H

/// Mask values for matched sources
typedef enum {
    PM_SOURCE_MATCH_MASK_PHOT = 0x01,   // Source was rejected from photometry fit
    PM_SOURCE_MATCH_MASK_ASTRO = 0x02,     // Source was rejected from astrometry fit
} pmSourceMatchMask;

/// Matched sources
///
/// A match between sources on different images.  The index back to the image and source are provided.
/// The magnitudes are pulled out, for convenience.
typedef struct {
    int num;                            // Number of matches
    psVector *image;                    // Image index
    psVector *index;                    // Source index for image
    psVector *mag;                      // Magnitudes
    psVector *magErr;                   // Magnitude errors
    psVector *x, *y;                    // Positions
    psVector *mask;                     // Mask for measurements
} pmSourceMatch;


// Assert that the source match and its components are non-NULL
#define PM_ASSERT_SOURCE_MATCH_NON_NULL(MATCH, RVAL) { \
    if (!MATCH) { \
        psError(PS_ERR_UNEXPECTED_NULL, true, "Source match %s is NULL.", #MATCH); \
        return RVAL; \
    } \
    PS_ASSERT_VECTOR_NON_NULL((MATCH)->image, RVAL); \
    PS_ASSERT_VECTOR_NON_NULL((MATCH)->index, RVAL); \
    PS_ASSERT_VECTOR_NON_NULL((MATCH)->mag, RVAL); \
    PS_ASSERT_VECTOR_NON_NULL((MATCH)->magErr, RVAL); \
    PS_ASSERT_VECTOR_NON_NULL((MATCH)->x, RVAL); \
    PS_ASSERT_VECTOR_NON_NULL((MATCH)->y, RVAL); \
    PS_ASSERT_VECTOR_SIZE((MATCH)->image, (MATCH)->num, RVAL); \
    PS_ASSERT_VECTOR_SIZE((MATCH)->index, (MATCH)->num, RVAL); \
    PS_ASSERT_VECTOR_SIZE((MATCH)->mag, (MATCH)->num, RVAL); \
    PS_ASSERT_VECTOR_SIZE((MATCH)->magErr, (MATCH)->num, RVAL); \
    PS_ASSERT_VECTOR_SIZE((MATCH)->x, (MATCH)->num, RVAL); \
    PS_ASSERT_VECTOR_SIZE((MATCH)->y, (MATCH)->num, RVAL); \
}

/// Allocator for pmSourceMatch
pmSourceMatch *pmSourceMatchAlloc(void);

/// Add a source to a match
void pmSourceMatchAdd(pmSourceMatch *match, // Match data
                      float mag, float magErr, // Magnitude and error
                      float x, float y,        // Position
                      int image, // Image index
                      int index // Source index
                      );



/// Match sources between different images
///
/// Returns an array of psSourceMatch
psArray *pmSourceMatchSources(const psArray *sourceArrays, ///< Array of arrays of sources on each image
                              float radius, ///< Matching radius
                              bool cullSingles ///< Cull "matches" with only a single source?
                              );

/// Merge two source lists
///
/// Sources are pulled from the lists into a new list, with no effort made to adjust them.
psArray *pmSourceMatchMerge(psArray *sourceArrays, ///< Array of arrays of sources on each image
                            float radius, ///< Matching radius
                            bool cullSingles ///< Cull "matches" with only a single source?
    );

/// Perform relative photometry to calibrate images
psVector *pmSourceMatchRelphot(const psArray *matches, // Array of matches
			       psArray *matchedSources, // Array of average sources
                               const psVector *zp, // Zero points for each image (including airmass term)
                               float tol, // Relative tolerance for convergence
                               int iter1, // Number of iterations for pass 1
                               float rej1, // Limit on rejection between iterations for pass 1
                               float sys1, // Systematic error in measurements for pass 1
                               int iter2, // Number of iterations for pass 2
                               float rej2, // Limit on rejection between iterations for pass 2
                               float sys2, // Systematic error in measurements for pass 2
                               float rejLimit, // Limit on rejection between iterations
                               int transIter, // Clipping iterations for transparency
                               float transClip, // Clipping level for transparency
                               float photoLevel // Level at which we declare image is photometric
    );

/// Perform relative astrometry to calibrate images
psArray *pmSourceMatchRelastro(const psArray *matches, // Array of matches
                               int numImages,          // Number of images
                               float tol, // Relative tolerance for convergence
                               int iter1, // Number of iterations for pass 1
                               float rej1, // Limit on rejection between iterations for pass 1
                               int iter2, // Number of iterations for pass 2
                               float rej2, // Limit on rejection between iterations for pass 2
                               float rejLimit // Limit on rejection between iterations
    );

#endif
