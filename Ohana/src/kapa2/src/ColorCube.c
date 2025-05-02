# include "Ximage.h"

/* these functions are used to generate an appropriately-sampled color map for a given image.
   we assume that only a limited number of colors can be allocated (start with, eg, 1024, grow to
   64k or so).  the RGB image is constructed from the 32bit images, each with their own zero and
   scale.  We want to allocate set of color cells which sample the actual colors available in the
   image data.  We generate a 3D histogram of the actual data values.  The histogram is generated
   using a k-d tree. */

// the k-d tree is built of a bunch of nodes, each of which represents a cubic region in the 3D
// color space.  each node may be occupied, or may point to a set of 8 children, subdividing the
// cube into equal cubes.

// NOTE : nothing in this file specifies a relationship between x,y,z and red,blue,green

// set of operations needed:

// allocate empty, bottom-level node
CCNode *CCNodeAlloc () {

  CCNode *node;
  int ix, iy, iz;

  ALLOCATE (node, CCNode, 1);
  node->count = 0;
  node->pixel = 0;
  node->bottom = TRUE;
  for (ix = 0; ix < 2; ix++) {
    for (iy = 0; iy < 2; iy++) {
      for (iz = 0; iz < 2; iz++) {
	node->sub[ix][iy][iz] = NULL;
      }
    }
  }
  return node;
}

// given a coordinate and a containing node, find the corresponding child node (if any)
CCNode *CCFindChild (CCNode *node, float x, float y, float z) {

  int ix, iy, iz;

  assert (x >= node->min[CC_X]);
  assert (y >= node->min[CC_Y]);
  assert (z >= node->min[CC_Z]);

  assert (x <= node->max[CC_X]);
  assert (y <= node->max[CC_Y]);
  assert (z <= node->max[CC_Z]);

  if (node->bottom) return NULL; // already at the bottom

  ix = (x < node->mid[CC_X]) ? 0 : 1;
  iy = (y < node->mid[CC_Y]) ? 0 : 1;
  iz = (z < node->mid[CC_Z]) ? 0 : 1;
  
  return node->sub[ix][iy][iz];
}

// given a coordinate and a containing node, find the corresponding child node (if any)
int CCSplitNode (CCNode *node) {

  CCNode *child;
  int ix, iy, iz;

  // trying to split an already split node
  if (!node->bottom) return FALSE;

  node->bottom = FALSE;
  for (ix = 0; ix < 2; ix++) {
    for (iy = 0; iy < 2; iy++) {
      for (iz = 0; iz < 2; iz++) {
	child = CCNodeAlloc();
	child->min[CC_X] = (ix == 0) ? node->min[CC_X] : node->mid[CC_X];
	child->min[CC_Y] = (iy == 0) ? node->min[CC_Y] : node->mid[CC_Y];
	child->min[CC_Z] = (iz == 0) ? node->min[CC_Z] : node->mid[CC_Z];
	child->max[CC_X] = (ix == 0) ? node->mid[CC_X] : node->max[CC_X];
	child->max[CC_Y] = (iy == 0) ? node->mid[CC_Y] : node->max[CC_Y];
	child->max[CC_Z] = (iz == 0) ? node->mid[CC_Z] : node->max[CC_Z];

	child->mid[CC_X] = 0.5*(child->min[CC_X] + child->max[CC_X]);
	child->mid[CC_Y] = 0.5*(child->min[CC_Y] + child->max[CC_Y]);
	child->mid[CC_Z] = 0.5*(child->min[CC_Z] + child->max[CC_Z]);

	node->sub[ix][iy][iz] = child;
      }
    }
  }
  return TRUE;
}

int CCSplitNodeIterate (CCNode *node, int current, int max) {

  int ix, iy, iz;

  if (current == max) return TRUE;
  current ++;

  CCSplitNode (node);
  for (ix = 0; ix < 2; ix++) {
    for (iy = 0; iy < 2; iy++) {
      for (iz = 0; iz < 2; iz++) {
	CCSplitNodeIterate (node->sub[ix][iy][iz], current, max);
      }
    }
  }
  return TRUE;
}

// allocate empty, bottom-level node
void CCNodeFree (CCNode *node) {

  int ix, iy, iz;

  for (ix = 0; ix < 2; ix++) {
    for (iy = 0; iy < 2; iy++) {
      for (iz = 0; iz < 2; iz++) {
	if (node->sub[ix][iy][iz]) CCNodeFree (node->sub[ix][iy][iz]);
      }
    }
  }
  free (node);
  return;
}

// given a coordinate and a containing node, find the corresponding child node (if any)
CCNode *CCFindBottom (CCNode *top, float x, float y, float z) {

  CCNode *node;
  CCNode *next;

  assert (x >= top->min[CC_X]);
  assert (y >= top->min[CC_Y]);
  assert (z >= top->min[CC_Z]);

  assert (x <= top->max[CC_X]);
  assert (y <= top->max[CC_Y]);
  assert (z <= top->max[CC_Z]);

  node = top;
  next = CCFindChild (node, x, y, z);
  while (next != NULL) {
    node = next;
    next = CCFindChild (node, x, y, z);
  }

  return node;
}

// iterate over the CCNode tree to generate a single, 1d vector of all data values
// XXX this function is specific to the color histogram concept
int CCNodeExtractCounts (CCNode *node, float **values, int *nvalues, int *NVALUES) {

  int ix, iy, iz;

  if (*values == NULL) {
    nvalues[0] = 0;
    NVALUES[0] = 32;
    ALLOCATE (*values, float, NVALUES[0]);
  }

  if (node->bottom) {
    values[0][nvalues[0]] = node->count;
    nvalues[0] ++;
    if (nvalues[0] >= NVALUES[0]) {
      NVALUES[0] += 32;
      REALLOCATE (*values, float, NVALUES[0]);
    }
    return TRUE;
  }
  
  for (ix = 0; ix < 2; ix++) {
    for (iy = 0; iy < 2; iy++) {
      for (iz = 0; iz < 2; iz++) {
	CCNodeExtractCounts (node->sub[ix][iy][iz], values, nvalues, NVALUES);
      }
    }
  }
  return TRUE;
}

// iterate over the CCNode tree and subdivide any bottom level nodes with value > minValue
// XXX this function is specific to the color histogram concept
int CCNodeDivideLimit (CCNode *node, float minValue) {

  int ix, iy, iz;

  if (node->bottom) {
    if (node->count > minValue) {
      CCSplitNode (node);
    }
    return TRUE;
  }
  
  for (ix = 0; ix < 2; ix++) {
    for (iy = 0; iy < 2; iy++) {
      for (iz = 0; iz < 2; iz++) {
	CCNodeDivideLimit (node->sub[ix][iy][iz], minValue);
      }
    }
  }
  return TRUE;
}

// iterate over the CCNode tree and subdivide any bottom level nodes with value > minValue
// XXX this function is specific to the color histogram concept
int CCNodeInitCounts (CCNode *node, float value) {

  int ix, iy, iz;

  if (node->bottom) {
    node->count = value;
    return TRUE;
  }
  
  for (ix = 0; ix < 2; ix++) {
    for (iy = 0; iy < 2; iy++) {
      for (iz = 0; iz < 2; iz++) {
	CCNodeInitCounts (node->sub[ix][iy][iz], value);
      }
    }
  }
  return TRUE;
}

