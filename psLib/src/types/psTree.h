#ifndef PS_TREE_H
#define PS_TREE_H

#include <psError.h>
#include <psVector.h>

/// An array of coordinates for the tree
///
/// The coordinate for point i, dimension j is psTreeCoordArray->F64[i][j].  We have a "raw" vector which
/// points into the operational vector, as is done for psImage.  This keeps all the memory together, reducing
/// fragmentation.  Currently supports only F64 type, but could be expanded to support multiple types (to save
/// memory) by changing the F64 member into a union.
typedef struct {
    psF64 **F64;                        ///< Coordinates
    psU8 *raw;                          // Raw vector
} psTreeCoordArray;

/// Type of tree
///
/// Specifies how distances are measured
typedef enum {
    PS_TREE_EUCLIDEAN,                  // d^2 = dx^2 + dy^2 + ...
    PS_TREE_SPHERICAL,                  // sin(dist/2)^2 = sin(dphi/2)^2 + cos(phi1)cos(phi2)sin(dlambda/2)^2
} psTreeType;

/// A simple kd-tree implementation
///
/// This parent tree structure contains all the main information for the tree.  We choose this instead of
/// including everything within the nodes to avoid memory fragmentation.
typedef struct {
    int dim;                            ///< Dimensionality
    int maxLeafContents;                ///< Maximum number of points on a leaf
    psTreeType type;                    ///< Type of tree
    long numNodes;                      ///< Number of nodes
    long numData;                       ///< Number of data points
    struct psTreeNode **nodes;          ///< Array of nodes
    psTreeCoordArray *min, *max;        ///< Bounding box: minimum and maximum coordinates
    double *division;                   ///< Division for each node
    struct psTreeNode *root;            ///< Convenience pointer to root node of tree
    psTreeCoordArray *data;             ///< Data: coordinates
    long *contents;                     ///< Indices into the data
} psTree;

/// A node of the kd-tree
///
/// Most of the information is stored in the parent tree (to reduce memory fragmentation).  All the pointers
/// are views only; the pointers to the nodes are contained within the parent tree's "nodes" element.
typedef struct psTreeNode {
    long index;                         ///< Index of node
    int divideDim;                      ///< Dimension in which to divide
    int num;                            ///< Number of points in node
    long *contents;                     ///< Contents of node; view only
    psTree *tree;                       ///< Parent tree; view only
    struct psTreeNode *up;              ///< Node higher up; view only
    struct psTreeNode *left, *right;    ///< Nodes below, to the left and right; views only
} psTreeNode;

/// Assertion for psTree
#define PS_ASSERT_TREE_NON_NULL(TREE, RETVAL) \
if (!(TREE) || !(TREE)->nodes || !(TREE)->min || !(TREE)->max || !(TREE)->division || !(TREE)->root || \
    !(TREE)->data || !(TREE)->contents) { \
    psError(PS_ERR_UNEXPECTED_NULL, true, "Tree %s or one of its components is NULL.", #TREE); \
    return RETVAL; \
}

/// Allocator for psTreeCoordArray
psTreeCoordArray *psTreeCoordArrayAlloc(long size, ///< Size of array
                                        int dim ///< Dimensionality
                                        );

/// Allocator for psTreeNode
psTreeNode *psTreeNodeAlloc(long index  ///< Index of node
                            );

/// Allocator for psTree
psTree *psTreeAlloc(int dim,            ///< Dimensionality
                    int maxLeafContents,///< Maximum number of points on a leaf
                    psTreeType type,    ///< Type of tree
                    long numNodes       ///< Number of nodes in tree
                    );

/// Plant (create) a tree
///
/// This is the main startup function.
psTree *psTreePlant(int dim,            ///< Dimensionality
                    int maxLeafContents,///< Maximum number of points on a leaf
                    psTreeType type,    ///< Type of tree
                    ...                 ///< psVector for each coordinate
                    );

/// Return the leaf node containing given coordinates
///
/// Returns a view to the pointer.
psTreeNode *psTreeLeaf(const psTree *tree, ///< Tree
                       const psVector *coords ///< Coordinates of interest
                       );

/// Return the index of the nearest neighbour to given coordinates
long psTreeNearest(const psTree *tree,  ///< Tree
                   const psVector *coords ///< Coordinates of interest
                   );

/// Return the index of the nearest neighbour to given coordinates, but within some radius
long psTreeNearestWithin(const psTree *tree,  ///< Tree
                         const psVector *coords, ///< Coordinates of interest
                         double radius  ///< Radius of interest
                         );

/// Return the number of points within some radius of given coordinates
long psTreeWithin(const psTree *tree,   ///< Tree
                  const psVector *coords, ///< Coordinates of interest
                  double radius         ///< Radius of interest
                  );

/// Return whether there are any points within some radius of given coordinates
bool psTreeWithinAny(const psTree *tree,   ///< Tree
                     const psVector *coords, ///< Coordinates of interest
                     double radius         ///< Radius of interest
                     );

/// Return the index of all points within some radius of given coordinates
psVector *psTreeAllWithin(const psTree *tree,          ///< Tree
                          const psVector *coordinates, ///< Coordinates of interest
                          double radius                ///< Radius of interest
                          );

/// Return the coordinates of a point in the tree, specified by its index
psVector *psTreeCoords(psVector *out,   ///< Output vector, or NULL
                       const psTree *tree, ///< Tree
                       long index       ///< Index of point in tree
                       );

/// Print a representation of the tree
void psTreePrint(FILE *fptr,            ///< File to which to print
                 const psTree *tree     ///< Tree
                 );

#endif
