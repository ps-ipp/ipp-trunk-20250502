/*
 * pxio.c
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

#ifdef HAVB_CONFIG_H
#include <config.h>
#endif

#include <pslib.h>

#include "pxtools.h"
#include "pxtree.h"

static void pxNodeFree(pxNode *node)
{
    psFree(node->name);
    psFree(node->parent);
    psFree(node->children);
    psFree(node->data);
}

pxNode *pxNodeAlloc(const char *name)
{
    PS_ASSERT_PTR_NON_NULL(name, NULL);

    pxNode *node = psAlloc(sizeof(pxNode));

    node->name = psStringCopy(name);
    node->parent = NULL;
    node->children = psListAlloc(NULL);
    node->data = NULL;

    psMemSetDeallocator(node, (psFreeFunc) pxNodeFree);

    return node;
}

pxNode *pxNodeAddParent(pxNode *node, pxNode *parent)
{
    // add this node to it's parent's children list
    psListAdd(parent->children, PS_LIST_TAIL, node);
    // and set the parent pointer
    node->parent = psMemIncrRefCounter(parent);

    return node;
}

pxNode *pxNodeAddChild(pxNode *node, pxNode *child)
{
    // add the child node to this nodes list of children
    psListAdd(node->children, PS_LIST_TAIL, child);
    // if the child already has a parent, release the ref count
    if (child->parent) {
        psFree(child->parent);
    }
    // [re]parent the child
    child->parent = psMemIncrRefCounter(node);

    return node;
}

pxNode *pxNodeAddData(pxNode *node, psPtr data)
{
    node->data = psMemIncrRefCounter(data);

    return node;
}

void pxNodePrint(FILE *stream, pxNode *node)
{
    fprintf(stream, "node name: %s\n", node->name);
    fprintf(stream, "node parent's name: %s\n", node->parent
                                              ?  node->parent->name
                                              : NULL);
    psListIterator *iter = psListIteratorAlloc(node->children, 0, false);
    pxNode *child = NULL;
    while ((child = psListGetAndIncrement(iter))) {
        fprintf(stream, "node child's name: %s\n", child->name);
    }
    psFree(iter);
}


static bool printNodes(void *arg, pxNode *node)
{
    pxNodePrint((FILE *)arg, node);
    fprintf((FILE *)arg, "\n\n");

    return true;
}


void pxTreePrint(FILE *stream, pxNode *root)
{
    pxTreeCrawl(root, printNodes, (void *)stream);
}

// func() returning false means decend no futher along this branch

bool pxTreeCrawl(pxNode *node, pxNodeFunc func, void *arg)
{
    // if func() returns false, stop
    if (!func(arg, node)) {
        return false;
    }

    psListIterator *iter = psListIteratorAlloc(node->children, 0, false);
    pxNode *child = NULL;
    while ((child = psListGetAndIncrement(iter))) {
        pxTreeCrawl(child, func, arg);
    }
    psFree(iter);

    return true;
}

bool pxNodeFuncCountDependants(void *arg, pxNode *node)
{
    (*(int *)arg)++;
    return true;
}

bool pxNodeHasChildren(pxNode *node)
{
    int children = psListLength(node->children);

    return children ? true : false;
}

bool pxNodeHasGrandChildren(pxNode *node)
{
    // find out how many nodes there are lower in the tree
    // subtract the parent node from the count so we have just a tally of
    // decendants
    int nodes = -1;
    pxTreeCrawl(node, pxNodeFuncCountDependants, &nodes);
    psTrace("pxtree", PS_LOG_INFO, "node %s has %d dependants", node->name, nodes);

    // find out how many children this node has
    int children = psListLength(node->children);
    psTrace("pxtree", PS_LOG_INFO, "node %s has %d children", node->name, children);

    if (!children) {
        // no children
        return false;
    }

    if (nodes < children) {
        psAbort("you can't have fewer children than decendants!");
    }

    if (nodes == children) {
        // no grandchildren
        return false;
    }

    return true;
}

pxNode *pxTreeFromMetadata(psMetadata *md)
{
    psHash *forest = psHashAlloc(10);

    psMetadataIterator *iter = psMetadataIteratorAlloc(md, 0, NULL);
    psMetadataItem *item = NULL;
    while ((item = psMetadataGetAndIncrement(iter))) {
        if (!(item->type == PS_DATA_STRING)) {
            continue;
        }

        // add this node to the forest and hopefully it'll get attached to the
        // root tree
        pxTreeBuilder(forest, item->name, item->data.str, NULL);
    }
    psFree(iter);

    pxNode *root = psHashLookup(forest, "root");
    psFree(forest);

    return psMemIncrRefCounter(root);
}

psHash *pxTreeBuilder(psHash *forest,
                      const char *nodeName,
                      const char *childName,
                      psPtr data)
{
    // try to find a node with this name
    pxNode *node = psHashLookup(forest, nodeName);
    if (!node) {
        // create a new node with this name
        node = pxNodeAlloc(nodeName);
        if (data) {
            pxNodeAddData(node, data);
        }
        // add it to the hash of nodes
        psHashAdd(forest, nodeName, node);
        // node may be used below because it will still have a ref count of
        // 1 from being on the hash, this is equivalent to the "view" we
        // get from a hashlookup
        psFree(node);
    } else if (!node->data) {
        pxNodeAddData(node, data);
    }

    // does this node declare a child?
    if (childName) {
        // try to find a node with this name
        pxNode *child = psHashLookup(forest, childName);
        if (!child ) {
            // create a new node with this name
            child = pxNodeAlloc(childName);
            // add it to the hash of nodes
            psHashAdd(forest, childName, child);
            // child may be used below because it will still have a ref
            // count of 1 from being on the hash, this is equivalent to the
            // "view" we get from a hashlookup
            psFree(child);
        }
        pxNodeAddChild(node, child);
    }

    return forest;
}
