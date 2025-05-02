/** Functions that describe the various stages of the IPP
 *  @author Bill Sweeney, IfA
 **/

#include <pslib.h>
#include "ippStages.h"
#include <string.h>

ippStage ippStringToStage(const psString stageString)
{
    PS_ASSERT_PTR_NON_NULL(stageString, IPP_STAGE_NONE);

    if (!strcmp(stageString, "raw")) {
        return IPP_STAGE_RAW;
    } else if (!strcmp(stageString, "chip")) {
        return IPP_STAGE_CHIP;
    } else if (!strcmp(stageString, "chip_bg")) {
        return IPP_STAGE_CHIP_BG;
    } else if (!strcmp(stageString, "camera")) {
        return IPP_STAGE_CAMERA;
    } else if (!strcmp(stageString, "warp")) {
        return IPP_STAGE_WARP;
    } else if (!strcmp(stageString, "warp_bg")) {
        return IPP_STAGE_WARP_BG;
    } else if (!strcmp(stageString, "fake")) {
        return IPP_STAGE_FAKE;
    } else if (!strcmp(stageString, "diff")) {
        return IPP_STAGE_DIFF;
    } else if (!strcmp(stageString, "stack")) {
        return IPP_STAGE_STACK;
    } else {
        psError(PS_ERR_PROGRAMMING, true, "%s is not a valid IPP stage", stageString);
        return IPP_STAGE_NONE;
    }
}
