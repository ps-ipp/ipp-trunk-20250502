# $Id: warp.md,v 1.21 2009-02-05 19:37:44 bills Exp $

#
# We have at least 3 different run types
# a) single epoch differing
# b) magic run operates on "complete" exposures
# c) multiple epoch differing/stacking operations
#

# define a new warprun for a single skycell
warpRun METADATA
    warp_id     S64         0       # Primary Key AUTO_INCREMENT
    fake_id     S64         0       # Key INDEX(warp_id, fake_id) fkey(fake_id) ref camProcessedExp(fake_id)
    mode        STR         64      # Key
    state       STR         64      # Key
    workdir     STR         255
    workdir_state STR       64      # Key
    label       STR         64      # key
    data_group  STR         64
    dist_group  STR         64
    dvodb       STR         255
    tess_id     STR         64
    reduction	STR	    64
    end_stage   STR         64      # Key
    registered  TAI         NULL
    magicked    S64         0
    software_ver    STR         16
    maskfrac_npix F32       0.0
    maskfrac_static F32	    0.0
    maskfrac_dynamic F32    0.0
    maskfrac_magic  F32     0.0
    maskfrac_advisory F32   0.0
    note        STR         255
END

warpSkyCellMap METADATA
    warp_id     S64         0       # Primary Key fkey(warp_id) ref warpRun(warp_id)
    skycell_id  STR         64      # Primary Key
    tess_id     STR         64      # Primary Key
    class_id    STR         64      # Primary Key
    fault       S16         0       # Key
END

warpImfile METADATA
    warp_id         S64     0       # Primary Key fkey(warp_id) ref warpRun(warp_id)
    skycell_id      STR     64      # Primary Key
    warp_skyfile_id S64     0       # Primary Key AUTO_INCREMENT
END

warpSkyfile METADATA
    warp_id        S64      0       # Primary Key fkey(warp_id, skycell_id, tess_id) ref warpSkyCellMap(warp_id, skycell_id, tess_id)
    skycell_id     STR      64      # Primary Key
    tess_id        STR      64      # Primary Key
    uri            STR      255
    path_base      STR      255
    data_state     STR      64      # Key
    bg             F64      0.0
    bg_stdev       F64      0.0
    dtime_warp     F32      0.0
    dtime_script   F32      0.0
    hostname       STR      64
    good_frac      F32      0.0     # Key
    xmin           S32      0
    xmax           S32      0
    ymin           S32      0
    ymax           S32      0
    fault          S16      0       # Key
    quality        S16      0
    magicked       S64      0
    software_ver    STR         16
    maskfrac_npix S32       0
    maskfrac_static F32	    0.0
    maskfrac_dynamic F32    0.0
    maskfrac_magic  F32     0.0
    maskfrac_advisory F32   0.0
    background_model  S16   0
END

warpMask METADATA
    label       STR         64      # Primary Key
END

warpSummary METADATA
    warp_id	   S64      0       # Primary Key fkey(warp_id) ref warpRun(warp_id)
    projection_cell  STR	    32	    # Primary Key
    path_base 	   STR	    255
END