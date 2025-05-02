#ifndef PM_SOURCE_GROUPS_H
#define PM_SOURCE_GROUPS_H

/// Groups of sources
///
/// We divide up the sources for threading.
typedef struct {
    int Xo, Yo;                         // Offset
    int Nx, Ny;                         // Size of cells
    int Cx, Cy;                         // Number of cells in x and y
    psArray *groups;                    // Cell groups
} pmSourceGroups;

/// Allocate the source groups
pmSourceGroups *pmSourceGroupsAlloc(
    const pmReadout *readout,           // Readout on which the sources are defined
    int nThreads                        // Number of threads
    );


/// Return the group and cell indices given x,y coordinates
bool pmSourceGroupsCoordToCell(
    int *group,                         // Group number, returned
    int *cell,                          // Cell number, returned
    float x, float y,                   // Coordinates
    const pmSourceGroups *groups        // Groups
    );

/// Populate the source groups
bool pmSourceGroupsPopulate(
    pmSourceGroups *groups,              // Source groups to populate
    const psVector *x, const psVector *y // Coordinates of sources
    );


/// Generate source groups from an array of sources
pmSourceGroups *pmSourceGroupsFromSources(
    const pmReadout *readout,           // Readout on which the sources are defined
    const psArray *sources,             // Array of sources
    int nThreads                        // Number of threads
    );

/// Generate source groups from vectors with source positions
pmSourceGroups *pmSourceGroupsFromVectors(
    const pmReadout *readout,             // Readout on which the sources are defined
    const psVector *x, const psVector *y, // Coordinates of sources
    int nThreads                          // Number of threads
    );

#endif
