/* @file ippDiffMode.h
 * @brief some macro defintions for the stages of the pipeline
 * @author Bill Sweeney, IfA
 *
 * Copyright 2010 Institute for Astronomy, University of Hawaii
 */

#ifndef IPP_DIFF_MODE_H
#define IPP_DIFF_MODE_H

typedef enum {
    IPP_DIFF_MODE_UNDEFINED   = 0,
    IPP_DIFF_MODE_WARP_WARP   = 1,
    IPP_DIFF_MODE_WARP_STACK  = 2,
    IPP_DIFF_MODE_STACK_WARP  = 3,
    IPP_DIFF_MODE_STACK_STACK = 4
} ippDiffMode;


#endif
