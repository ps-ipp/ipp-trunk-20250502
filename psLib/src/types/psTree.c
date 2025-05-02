#include <stdio.h>
#include <string.h>

#include "psAssert.h"
#include "psAbort.h"
#include "psMemory.h"
#include "psType.h"
#include "psArray.h"
#include "psVector.h"
#include "psImage.h"
#include "psList.h"
#include "psSort.h"

#include "psTree.h"

//#define INPUT_CHECK                   // Check inputs for functions that may be in a tight loop?


// XXX Upgrades:
// * Could allow different types of coordinates (saving memory)
// * Could make tree "growable" by including a pointer to a new tree within each node.
// * Rebalancing the tree ("pruning"?) is done by collecting all the points and re-planting a new tree.

static void treeCoordArrayFree(psTreeCoordArray *array)
{
    psFree(array->F64);
    psFree(array->raw);
}

static void treeNodeFree(psTreeNode *node)
{
    // All contents are views only
    return;
}

static void treeFree(psTree *tree)
{
    for (long i = 0; i < tree->numNodes; i++) {
        psFree(tree->nodes[i]);
    }
    psFree(tree->nodes);
    psFree(tree->min);
    psFree(tree->max);
    psFree(tree->division);
    // 'root' is a view only
    psFree(tree->data);
    psFree(tree->contents);
}

psTreeCoordArray *psTreeCoordArrayAlloc(long size, int dim)
{
    psAssert(size > 0, "Size (%ld) must be positive", size);
    psAssert(dim > 0, "Dimension (%d) must be positive", dim);

    psTreeCoordArray *array = psAlloc(sizeof(psTreeCoordArray)); // Array to return
    psMemSetDeallocator(array, (psFreeFunc)treeCoordArrayFree);

    array->raw = psAlloc(dim * size * sizeof(psF64));
    array->F64 = psAlloc(size * sizeof(psF64*));

    psF64 *ptr = (psF64*)array->raw;    // Pointer into raw vector
    for (long i = 0; i < size; i++, ptr += dim) {
        array->F64[i] = ptr;
    }

    return array;
}

psTreeNode *psTreeNodeAlloc(long index)
{
    psAssert(index >= 0, "Index (%ld) must be non-negative", index);

    psTreeNode *node = psAlloc(sizeof(psTreeNode)); // Node to return
    psMemSetDeallocator(node, (psFreeFunc)treeNodeFree);

    node->index = index;
    node->divideDim = -1;
    node->num = 0;
    node->contents = NULL;
    node->tree = NULL;
    node->up = NULL;
    node->left = NULL;
    node->right = NULL;

    return node;
}

psTree *psTreeAlloc(int dim, int maxLeafContents, psTreeType type, long numNodes)
{
    psAssert(dim > 0, "Dimensionality (%d) must be positive", dim);
    psAssert(maxLeafContents > 0, "Maximum number per leaf (%d) must be positive", maxLeafContents);
    psAssert(numNodes > 0, "Number of nodes (%ld) must be positive", numNodes);

    psTree *tree = psAlloc(sizeof(psTree)); // Tree to return
    psMemSetDeallocator(tree, (psFreeFunc)treeFree);

    tree->dim = dim;
    tree->maxLeafContents = maxLeafContents;
    tree->type = type;

    tree->numNodes = numNodes;
    tree->nodes = psAlloc(numNodes * sizeof(psTreeNode*));
    for (long i = 0; i < numNodes; i++) {
        tree->nodes[i] = psTreeNodeAlloc(i);
    }
    tree->root = tree->nodes[0];

    tree->min = psTreeCoordArrayAlloc(numNodes, dim);
    tree->max = psTreeCoordArrayAlloc(numNodes, dim);

    tree->division = psAlloc(numNodes * sizeof(double));

    tree->numData = 0;
    tree->data = NULL;

    return tree;
}


psTree *psTreePlant(int dim, int maxLeafContents, psTreeType type, ...)
{
    PS_ASSERT_INT_POSITIVE(dim, NULL);
    PS_ASSERT_INT_POSITIVE(maxLeafContents, NULL);

    // Parse coordinate list
    va_list args;                       // Variable argument list
    va_start(args, type);
    psArray *coords = psArrayAlloc(dim); // Array of coordinates
    long numData = 0;                   // Number of data points
    for (int i = 0; i < dim; i++) {
        psVector *data = va_arg(args, psVector*);
        if (i == 0) {
            numData = data->n;
        } else if (data->n != numData) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Input vectors have differing sizes: %ld vs %ld",
                    data->n, numData);
            psFree(coords);
            va_end(args);
            return NULL;
        }
        coords->data[i] = psMemIncrRefCounter(data);
    }
    va_end(args);

    long numNodes;
    {
        // From equation 21.2.2 in Numerical Recipes vol 3, p1102.
        long pow2;                      // Smallest power of two >= numData
        for (pow2 = 1; pow2 < numData; pow2 <<= 1);
        numNodes = PS_MAX(PS_MIN(pow2 - 1, 2 * numData - pow2 / 2 - 1), 1);
    }

    psTree *tree = psTreeAlloc(dim, maxLeafContents, type, numNodes);
    tree->data = psTreeCoordArrayAlloc(numData, dim);
    tree->numData = numData;
    psF64 **data = tree->data->F64;     // Dereference data

    // Copy data into the tree
#define PSTREE_COPY_DATA_CASE(TYPE) \
    case PS_TYPE_##TYPE: { \
        for (long j = 0; j < numData; j++) { \
            data[j][i] = values->data.TYPE[j]; \
        } \
        break; \
    }

    for (int i = 0; i < dim; i++) {
        psVector *values = coords->data[i]; // Vector of values for this dimension
        switch (values->type.type) {
            PSTREE_COPY_DATA_CASE(U8);
            PSTREE_COPY_DATA_CASE(U16);
            PSTREE_COPY_DATA_CASE(U32);
            PSTREE_COPY_DATA_CASE(U64);
            PSTREE_COPY_DATA_CASE(S8);
            PSTREE_COPY_DATA_CASE(S16);
            PSTREE_COPY_DATA_CASE(S32);
            PSTREE_COPY_DATA_CASE(S64);
            PSTREE_COPY_DATA_CASE(F32);
            PSTREE_COPY_DATA_CASE(F64);
          default:
            psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Unsupported vector type for dimension %d: %x",
                    i, values->type.type);
            psFree(tree);
            psFree(coords);
        }
    }
    psFree(coords);

    long *contents = tree->contents = psAlloc(numData * sizeof(long)); // Contents of tree
    for (long i = 0; i < numData; i++) {
        contents[i] = i;
    }

    psTreeNode *root = tree->root;      // Root of the tree
    root->tree = tree;
    root->contents = contents;
    root->num = numData;
    root->divideDim = 0;
    for (int i = 0; i < dim; i++) {
        tree->min->F64[0][i] = -INFINITY;
        tree->max->F64[0][i] = INFINITY;
    }

    if (numData <= maxLeafContents) {
        // Don't need to do any more work
        return tree;
    }

    psArray *work = psArrayAlloc(numNodes); // Work queue
    work->data[0] = root;
    long workNum = 1;

// Compare points (in a single dimension) by their index
#define PSTREE_SELECT_COMPARE(A,B) (data[contents[A]][divideDim] < data[contents[B]][divideDim])
// Swap points by their index
#define PSTREE_SELECT_SWAP(TYPE,A,B) { \
    if (A != B) { \
        TYPE temp = contents[A]; \
        contents[A] = contents[B]; \
        contents[B] = temp; \
    } \
}

    for (long workIndex = 0, nextIndex = 0; workIndex < workNum; workIndex++) {
        psTreeNode *node = work->data[workIndex]; // Node to be worked on
        long num = node->num;           // Number of points in node
        long *contents = node->contents;// Indices of points in node
        psF64 **data = tree->data->F64; // Dereference the values
        long middle = num / 2;          // Index of middle point
        int divideDim = node->divideDim;// Dimension in which to divide
        long nodeIndex = node->index;   // Index of node

        PSSELECT(num, middle, PSTREE_SELECT_COMPARE, PSTREE_SELECT_SWAP, long);

        psF64 *nodeMin = tree->min->F64[nodeIndex]; // Minimum bounds for current node
        psF64 *nodeMax = tree->max->F64[nodeIndex]; // Maximum bounds for current node

        // Divide to the left and right
        double division = tree->division[nodeIndex] = data[contents[middle]][divideDim];

        long leftIndex = ++nextIndex, rightIndex = ++nextIndex; // Indices for left and right nodes
        psTreeNode *left = node->left = tree->nodes[leftIndex]; // Node to the left
        psTreeNode *right = node->right = tree->nodes[rightIndex]; // Node to the right
        long numLeft = middle, numRight = num - middle; // Number in each node

        for (int i = 0; i < dim; i++) {
            tree->min->F64[leftIndex][i] = nodeMin[i];
            tree->min->F64[rightIndex][i] = (i == divideDim ? division : nodeMin[i]);
            tree->max->F64[leftIndex][i] = (i == divideDim ? division : nodeMax[i]);
            tree->max->F64[rightIndex][i] = nodeMax[i];
        }

        left->num = numLeft;
        right->num = numRight;

        left->contents = contents;
        right->contents = contents + middle;

#if 0
        // Check contents
        for (int i = 0; i < numLeft; i++) {
            long index = left->contents[i]; // Index of point
            for (int j = 0; j < dim; j++) {
                if (data[index][j] < tree->min->F64[leftIndex][j] ||
                    data[index][j] > tree->max->F64[leftIndex][j]) {
                    psWarning("Bad division");
                }
            }
        }
        for (int i = 0; i < numRight; i++) {
            long index = right->contents[i]; // Index of point
            for (int j = 0; j < dim; j++) {
                if (data[index][j] < tree->min->F64[rightIndex][j] ||
                    data[index][j] > tree->max->F64[rightIndex][j]) {
                    psWarning("Bad division");
                }
            }
        }
#endif

        divideDim++;
        if (divideDim == dim) {
            divideDim = 0;
        }
        left->divideDim = right->divideDim = divideDim;

        left->up = right->up = node;
        left->tree = right->tree = tree;

        if (numLeft > maxLeafContents) {
            work->data[workNum++] = left;
        }
        if (numRight > maxLeafContents) {
            work->data[workNum++] = right;
        }

        work->data[workIndex] = NULL;
    }
    psFree(work);

    return tree;
}

// Print a representation of the node; called recursively
static void treeNodePrint(FILE *fptr,   // File pointer to which to print
                          const psTreeNode *node, // Node to print
                          int level     // Level at which to print
    )
{
    long index = node->index;           // Index of node
    psTree *tree = node->tree;          // Parent tree
    int dim = tree->dim;                // Dimensions

    for (int i = 0; i < level; i++) {
        fprintf(fptr, " ");
    }
    fprintf(fptr, "%ld: (", index);
    for (int i = 0; i < dim; i++) {
        fprintf(fptr, "%lf", tree->min->F64[index][i]);
        if (i < dim - 1) {
            fprintf(fptr, ",");
        }
    }
    fprintf(fptr, ") --> (");
    for (int i = 0; i < dim; i++) {
        fprintf(fptr, "%lf", tree->max->F64[index][i]);
        if (i < dim - 1) {
            fprintf(fptr, ",");
        }
    }
    fprintf(fptr, ")");
    level++;

    if (node->left && node->right) {
        fprintf(fptr, " ==> %lf\n", tree->division[index]);
        treeNodePrint(fptr, node->left, level);
        treeNodePrint(fptr, node->right, level);
        return;
    }
    fprintf(fptr, "\n");
    for (int i = 0; i < level; i++) {
        fprintf(fptr, " ");
    }
    for (int i = 0; i < node->num; i++) {
        fprintf(fptr, "%ld=(",node->contents[i]);
        psF64 *coords = tree->data->F64[node->contents[i]]; // Coordinates
        for (int j = 0; j < dim; j++) {
            fprintf(fptr, "%lf", coords[j]);
            if (j < dim - 1) {
                fprintf(fptr, ",");
            }
        }
        fprintf(fptr, ") ");
    }
    fprintf(fptr, "\n");

    return;
}


void psTreePrint(FILE *fptr, const psTree *tree)
{
    PS_ASSERT_TREE_NON_NULL(tree, );

    treeNodePrint(fptr, tree->root, 0);
    return;
}

// Given an arbitrary point, find the appropriate leaf
psTreeNode *psTreeLeaf(const psTree *tree, const psVector *coords)
{
#ifdef INPUT_CHECK // Might be in a tight loop
    PS_ASSERT_TREE_NON_NULL(tree, NULL);
    PS_ASSERT_VECTOR_NON_NULL(coords, NULL);
    PS_ASSERT_VECTOR_SIZE(coords, (long)tree->dim, NULL);
#endif

#define TREE_LEAF_CASE(TYPE) \
  case PS_TYPE_##TYPE: \
      for (; node->left; node = (coords->data.TYPE[node->divideDim] < tree->division[node->index]) ? \
               node->left : node->right); \
      break;

    psTreeNode *node = tree->root;      // Node of interest
    switch (coords->type.type) {
        TREE_LEAF_CASE(U8);
        TREE_LEAF_CASE(U16);
        TREE_LEAF_CASE(U32);
        TREE_LEAF_CASE(U64);
        TREE_LEAF_CASE(S8);
        TREE_LEAF_CASE(S16);
        TREE_LEAF_CASE(S32);
        TREE_LEAF_CASE(S64);
        TREE_LEAF_CASE(F32);
        TREE_LEAF_CASE(F64);
      default:
        psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Unsupported type: %x", coords->type.type);
        return NULL;
    }
    return node;
}

// Returns the square of the distance between a point on the tree and an aribtary point
static inline double treeContentDistance(const psTree *tree, // Tree of interest
                                         long index, // Index of point on tree
                                         const psVector *coords // Coordinates of interest
    )
{
    int dim = tree->dim;                // Dimensionality
    switch (tree->type) {
      case PS_TREE_EUCLIDEAN:
        switch (dim) {
          case 2:
            return PS_SQR(coords->data.F64[0] - tree->data->F64[index][0]) +
                PS_SQR(coords->data.F64[1] - tree->data->F64[index][1]);
          case 3:
            return PS_SQR(coords->data.F64[0] - tree->data->F64[index][0]) +
                PS_SQR(coords->data.F64[1] - tree->data->F64[index][1]) +
                PS_SQR(coords->data.F64[2] - tree->data->F64[index][2]);
          default: {
              double distance2 = 0.0;             // Distance of interest
              for (int i = 0; i < dim; i++) {
                  distance2 += PS_SQR(coords->data.F64[i] - tree->data->F64[index][i]);
              }
              return distance2;
          }
        }
        break;
      case PS_TREE_SPHERICAL:
        switch (dim) {
          case 2: {
              // Haversine formula, modulo a factor of 1/2
              double dphi = coords->data.F64[1] - tree->data->F64[index][1];
              double haverPhi = 1.0 - cos(dphi);
              double dlambda = coords->data.F64[0] - tree->data->F64[index][0];
              double haverLambda = 1.0 - cos(dlambda);
              return haverPhi + cos(coords->data.F64[1]) * cos(tree->data->F64[index][1]) * haverLambda;
          }
          default:
            psAbort("Spherical distances not supported for more than 2 dimensions");
        }
      default:
        psAbort("Unrecognised type: %x", tree->type);
    }

    return NAN;
}

// Returns the square of the distance between a leaf and an arbitrary point
static inline double treeBranchDistance(const psTree *tree, long index, const psVector *coords)
{
    int dim = tree->dim;                // Dimensionality
    double distance = 0.0;              // Distance to box
    switch (tree->type) {
      case PS_TREE_EUCLIDEAN:
        for (int i = 0; i < dim; i++) {
            double minDiff = tree->min->F64[index][i] - coords->data.F64[i];
            if (minDiff > 0) {
                distance += PS_SQR(minDiff);
                continue;
            }
            double maxDiff = coords->data.F64[i] - tree->max->F64[index][i];
            if (maxDiff > 0) {
                distance += PS_SQR(maxDiff);
                continue;
            }
        }
        break;
      case PS_TREE_SPHERICAL: {
          double ra = coords->data.F64[0], dec = coords->data.F64[1];                 // Coords of interest
          double raMin = tree->min->F64[index][0], raMax = tree->max->F64[index][0]; // RA bounds
          double decMin = tree->min->F64[index][1], decMax = tree->max->F64[index][1]; // Dec bounds

          // Haversine formula, modulo a factor of 1/2
          // This seems to deal with the wrap at RA=0=2pi, probably ascending the tree and descending on the
          // other side of the wrap.
          if (ra < raMin || ra > raMax) {
              double ra1 = cos(ra - raMin), ra2 = cos(ra - raMax); // Options for RA distance
              double raDist = PS_MAX(ra1, ra2);                    // Will give the smallest distance
              double dec0 = (fabs(dec - decMin) < fabs(dec - decMax)) ? decMin : decMax; // Closest Dec limit
              distance += cos(dec0) * cos(dec) * (1.0 - raDist);
          }

          if (dec < decMin || dec > decMax) {
              double dec1 = cos(dec - decMin), dec2 = cos(dec - decMax); // Options for Dec distance
              distance += 1.0 - PS_MAX(dec1, dec2);
          }

          break;
      }
      default:
        psAbort("Unrecognised type: %x", tree->type);
    }
    return distance;
}

// Returns whether the designated leaf contains an arbitrary point
static inline bool treeBranchInside(const psTree *tree, // Tree of interest
                                    long index, // Index of leaf on tree
                                    const psVector *coords // Coordinates of interest
    )
{
    int dim = tree->dim;                // Dimensionality
    for (int i = 0; i < dim; i++) {
        if (tree->min->F64[index][i] > coords->data.F64[i]) {
            return false;
        }
        if (tree->max->F64[index][i] < coords->data.F64[i]) {
            return false;
        }
    }
    return true;
}

// Search a leaf node for a closer point
static inline void treeLeafSearchNearest(long *bestIndex, // Index of closest point
                                         double *bestDistance, // Distance to closest point
                                         const psTree *tree, // Tree of interest
                                         const psTreeNode *leaf, // Leaf to search
                                         const psVector *coords // Coordinates of interest
    )
{
    for (int i = 0; i < leaf->num; i++) {
        long index = leaf->contents[i]; // Index of point
        double distance = treeContentDistance(tree, index, coords); // Distance to point
        if (distance < *bestDistance) {
            *bestIndex = index;
            *bestDistance = distance;
        }
    }
    return;
}

// Return the index of the nearest neighbour to given coordinates, within some distance measure
// This is the engine for psTreeNearest() and psTreeNearestWithin()
static inline long treeNearestWithin(const psTree *tree, // Tree
                                     const psVector *coordinates, // Coordinates of interest
                                     double bestDistance // Distance measure to best point
    )
{
#ifdef INPUT_CHECK // Might be in a tight loop
    PS_ASSERT_TREE_NON_NULL(tree, -1);
    PS_ASSERT_VECTOR_NON_NULL(coordinates, -1);
    PS_ASSERT_VECTOR_SIZE(coordinates, (long)tree->dim, -1);
#endif

    psVector *coords = (coordinates->type.type == PS_TYPE_F64 ? psMemIncrRefCounter((psVector*)coordinates) :
                        psVectorCopy(NULL, coordinates, PS_TYPE_F64)); // F64 version of coordinates

    // Find the closest point in the leaf that contains the point of interest
    psTreeNode *leaf = psTreeLeaf(tree, coords); // Leaf containing the point of interest
    long bestIndex = -1;                // Index of best point
    treeLeafSearchNearest(&bestIndex, &bestDistance, tree, leaf, coords);

    // Work up from the leaf containing the point of interest, and for each in turn, search down the other
    // branch.  Every time we search down a branch, we first check to see if the current best distance rules
    // out that branch.  By having the check within the work queue loop, we allow the best distance to evolve
    // (smaller), which means our elimination test goes up against the smallest possible distance.
    psArray *work = psArrayAlloc(tree->numNodes); // Work queue
    while (leaf->up) {
        psTreeNode *up = leaf->up;      // Parent node

        long workIndex = 0;
        work->data[workIndex] = (leaf == up->left ? up->right : up->left); // The other node
        while (workIndex >= 0) {
            psTreeNode *node = work->data[workIndex];
            if (treeBranchDistance(tree, node->index, coords) > bestDistance) {
                // No need to investigate
                work->data[workIndex] = NULL;
                workIndex--;
                continue;
            }
            if (node->left) {
                // Branch node
                work->data[workIndex] = node->right;
                work->data[++workIndex] = node->left;
                continue;
            }
            // Leaf node
            treeLeafSearchNearest(&bestIndex, &bestDistance, tree, node, coords);
            work->data[workIndex] = NULL;
            workIndex--;
        }

        leaf = up;
    }
    psFree(work);
    psFree(coords);

    return bestIndex;
}

// Given an arbitrary point, return the index of the nearest neighbour
long psTreeNearest(const psTree *tree, const psVector *coords)
{
    return treeNearestWithin(tree, coords, INFINITY);
}


// Convert a radius to our internal "distance measure"
// Often, we're given a search radius, but for efficiency reasons, we don't use that internally.
static double treeRadiusToDistance(const psTree *tree, double radius)
{
    switch (tree->type) {
      case PS_TREE_EUCLIDEAN:
        // Using the square of the distance as the distance measure
        return PS_SQR(radius);
      case PS_TREE_SPHERICAL:
        // Using Haversine formula, modulo a factor of 1/2
        return 1.0 - cos(radius);
      default:
        psAbort("Unrecognised type: %x", tree->type);
    }
}


long psTreeNearestWithin(const psTree *tree, const psVector *coords, double radius)
{
    return treeNearestWithin(tree, coords, treeRadiusToDistance(tree, radius));
}


// Search a leaf node for points within distance
static inline long treeLeafSearchWithin(double distance, // Distance to search
                                        const psTree *tree, // Tree of interest
                                        const psTreeNode *leaf, // Leaf to search
                                        const psVector *coords // Coordinates of interest
    )
{
    long num = 0;                       // Number within circle
    for (int i = 0; i < leaf->num; i++) {
        long index = leaf->contents[i]; // Index of point
        if (treeContentDistance(tree, index, coords) < distance) {
            num++;
        }
    }
    return num;
}

// Given an arbitrary point and a matching radius, return the number of points within that radius
long psTreeWithin(const psTree *tree, const psVector *coordinates, double radius)
{
#ifdef INPUT_CHECK // Might be in a tight loop
    PS_ASSERT_TREE_NON_NULL(tree, -1);
    PS_ASSERT_VECTOR_NON_NULL(coordinates, -1);
    PS_ASSERT_VECTOR_SIZE(coordinates, (long)tree->dim, -1);
#endif

    psVector *coords = (coordinates->type.type == PS_TYPE_F64 ? psMemIncrRefCounter((psVector*)coordinates) :
                        psVectorCopy(NULL, coordinates, PS_TYPE_F64)); // F64 version of coordinates

    double distance = treeRadiusToDistance(tree, radius); // Distance measure
    long num = 0;                       // Number of points in circle

    // This is essentially the same as psTreeNearest

    // Find the closest point in the leaf that contains the point of interest
    psTreeNode *leaf = psTreeLeaf(tree, coords); // Leaf containing the point of interest
    num += treeLeafSearchWithin(distance, tree, leaf, coords);

    psArray *work = psArrayAlloc(tree->numNodes); // Work queue
    while (leaf->up) {
        psTreeNode *up = leaf->up;      // Parent node

        long workIndex = 0;
        work->data[workIndex] = (leaf == up->left ? up->right : up->left); // The other node
        while (workIndex >= 0) {
            psTreeNode *node = work->data[workIndex];
            if (treeBranchDistance(tree, node->index, coords) > distance) {
                // No need to investigate
                work->data[workIndex] = NULL;
                workIndex--;
                continue;
            }
            if (node->left) {
                // Branch node
                work->data[workIndex] = node->right;
                work->data[++workIndex] = node->left;
                continue;
            }
            // Leaf node
            num += treeLeafSearchWithin(distance, tree, node, coords);
            work->data[workIndex] = NULL;
            workIndex--;
        }

        leaf = up;
    }
    psFree(work);
    psFree(coords);

    return num;
}

// Search a leaf node for points within distance
static inline void treeLeafSearchAllWithin(psVector *result,       // Result vector
                                          double distance, // Distance to search
                                          const psTree *tree, // Tree of interest
                                          const psTreeNode *leaf, // Leaf to search
                                          const psVector *coords // Coordinates of interest
    )
{
    for (int i = 0; i < leaf->num; i++) {
        psS64 index = leaf->contents[i]; // Index of point
        if (treeContentDistance(tree, index, coords) < distance) {
            psVectorAppend(result, index);
        }
    }
    return;
}

// Given an arbitrary point and a matching radius, return the index of all points within that radius
psVector *psTreeAllWithin(const psTree *tree, const psVector *coordinates, double radius)
{
#ifdef INPUT_CHECK // Might be in a tight loop
    PS_ASSERT_TREE_NON_NULL(tree, NULL);
    PS_ASSERT_VECTOR_NON_NULL(coordinates, NULL);
    PS_ASSERT_VECTOR_SIZE(coordinates, (long)tree->dim, NULL);
#endif

    psVector *coords = (coordinates->type.type == PS_TYPE_F64 ? psMemIncrRefCounter((psVector*)coordinates) :
                        psVectorCopy(NULL, coordinates, PS_TYPE_F64)); // F64 version of coordinates

    double distance = treeRadiusToDistance(tree, radius); // Distance measure

    psVector *result = psVectorAllocEmpty(4, PS_TYPE_S64); // Indices of points within match radius

    // This is essentially the same as psTreeNearest, except pruning based on the search radius

    // Find the closest point in the leaf that contains the point of interest
    psTreeNode *leaf = psTreeLeaf(tree, coords); // Leaf containing the point of interest
    treeLeafSearchAllWithin(result, distance, tree, leaf, coords);

    psArray *work = psArrayAlloc(tree->numNodes); // Work queue
    while (leaf->up) {
        psTreeNode *up = leaf->up;      // Parent node

        long workIndex = 0;
        work->data[workIndex] = (leaf == up->left ? up->right : up->left); // The other node
        while (workIndex >= 0) {
            psTreeNode *node = work->data[workIndex];
            if (treeBranchDistance(tree, node->index, coords) > distance) {
                // No need to investigate
                work->data[workIndex] = NULL;
                workIndex--;
                continue;
            }
            if (node->left) {
                // Branch node
                work->data[workIndex] = node->right;
                work->data[++workIndex] = node->left;
                continue;
            }
            // Leaf node
            treeLeafSearchAllWithin(result, distance, tree, node, coords);
            work->data[workIndex] = NULL;
            workIndex--;
        }

        leaf = up;
    }
    psFree(work);
    psFree(coords);

    return result;
}

// Search a leaf node for any points within distance measure
static inline bool treeLeafSearchWithinAny(double distance, // Distance to search
                                           const psTree *tree, // Tree of interest
                                           const psTreeNode *leaf, // Leaf to search
                                           const psVector *coords // Coordinates of interest
    )
{
    for (int i = 0; i < leaf->num; i++) {
        long index = leaf->contents[i]; // Index of point
        if (treeContentDistance(tree, index, coords) < distance) {
            return true;
        }
    }
    return false;
}

// Given an arbitrary point and a matching radius, return whether there are any points within that radius
bool psTreeWithinAny(const psTree *tree, const psVector *coordinates, double radius)
{
#ifdef INPUT_CHECK // Might be in a tight loop
    PS_ASSERT_TREE_NON_NULL(tree, false);
    PS_ASSERT_VECTOR_NON_NULL(coordinates, false);
    PS_ASSERT_VECTOR_SIZE(coordinates, (long)tree->dim, false);
#endif

    psVector *coords = (coordinates->type.type == PS_TYPE_F64 ? psMemIncrRefCounter((psVector*)coordinates) :
                        psVectorCopy(NULL, coordinates, PS_TYPE_F64)); // F64 version of coordinates

    double distance = treeRadiusToDistance(tree, radius); // Distance measure

    // This is essentially the same as psTreeWithin, except we can bail as soon as we find something

    // Find the closest point in the leaf that contains the point of interest
    psTreeNode *leaf = psTreeLeaf(tree, coords); // Leaf containing the point of interest
    if (treeLeafSearchWithinAny(distance, tree, leaf, coords)) {
        psFree(coords);
        return true;
    }

    psArray *work = psArrayAlloc(tree->numNodes); // Work queue
    while (leaf->up) {
        psTreeNode *up = leaf->up;      // Parent node

        long workIndex = 0;
        work->data[workIndex] = (leaf == up->left ? up->right : up->left); // The other node
        while (workIndex >= 0) {
            psTreeNode *node = work->data[workIndex];
            if (treeBranchDistance(tree, node->index, coords) > distance) {
                // No need to investigate
                work->data[workIndex] = NULL;
                workIndex--;
                continue;
            }
            if (node->left) {
                // Branch node
                work->data[workIndex] = node->right;
                work->data[++workIndex] = node->left;
                continue;
            }
            // Leaf node
            if (treeLeafSearchWithinAny(distance, tree, node, coords)) {
                // Clear out the work queue
                memset(work->data, 0, (workIndex + 1) * sizeof(void));
                psFree(work);
                psFree(coords);
                return true;
            }
            work->data[workIndex] = NULL;
            workIndex--;
        }

        leaf = up;
    }
    psFree(work);
    psFree(coords);

    return false;
}


psVector *psTreeCoords(psVector *out, const psTree *tree, long index)
{
#ifdef INPUT_CHECK // Might be in a tight loop
    PS_ASSERT_TREE_NON_NULL(tree, NULL);
    PS_ASSERT_INT_NONNEGATIVE(index, NULL);
    PS_ASSERT_INT_LESS_THAN_OR_EQUAL(index, tree->numData, NULL);
#endif

    int dim = tree->dim;                // Dimensions

    out = psVectorRecycle(out, dim, PS_TYPE_F64);
    memcpy(out->data.F64, tree->data->F64[index], dim * PSELEMTYPE_SIZEOF(PS_TYPE_F64));
    return out;
}
