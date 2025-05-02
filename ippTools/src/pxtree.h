/*
 * pxtree.h
 *
 * Copyright (C) 2007  Joshua Hoblitt
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * program; see the file COPYING. If not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef PXTREE_H
#define PXTREE_H 1

#include <stdio.h>
#include <stdbool.h>

#include <pslib.h>


typedef struct pxNode {
  char *name;
  struct pxNode *parent;
  psList  *children;
  void *data;
} pxNode;

typedef bool (*pxNodeFunc)(void *arg, pxNode *node);

pxNode *pxNodeAlloc(
    const char *name
) PS_ATTR_MALLOC;

pxNode *pxNodeAddParent(pxNode *node, pxNode *parent);
pxNode *pxNodeAddChild(pxNode *node, pxNode *child);
pxNode *pxNodeAddData(pxNode *node, psPtr data);

void pxTreePrint(FILE *stream, pxNode *root);

void pxNodePrint(
    FILE *stream,
    pxNode  *node
);

bool pxTreeCrawl(
    pxNode  *node,
    pxNodeFunc func,
    void    *arg
);

bool pxNodeFuncCountDependants(void *arg, pxNode *node);

bool pxNodeHasChildren(pxNode *node);

bool pxNodeHasGrandChildren(pxNode *node);

pxNode *pxTreeFromMetadata(psMetadata *md);

psHash *pxTreeBuilder(psHash *forest,
                      const char *nodeName,
                      const char *childName,
                      psPtr data);

#endif // PXTREE_H
