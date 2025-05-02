/* @file ippStages.h
 * @brief some macro defintions for the stages of the pipeline
 * @author Bill Sweeney, IfA
 *
 * Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#ifndef IPP_STAGES_H
#define IPP_STAGES_H

typedef enum {
    IPP_STAGE_NONE = -1,
    IPP_STAGE_RAW = 0,
    IPP_STAGE_CHIP,
    IPP_STAGE_CHIP_BG,
    IPP_STAGE_CAMERA,
    IPP_STAGE_FAKE,
    IPP_STAGE_WARP,
    IPP_STAGE_WARP_BG,
    IPP_STAGE_DIFF,
    IPP_STAGE_STACK,
} ippStage;

/** return the ippStage represented by a string
 * @return the corresponding value or IPP_STAGE_NONE if invalid
 */
ippStage ippStringToStage(const psString stageString);

#endif
