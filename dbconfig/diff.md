# $Id: diff.md,v 1.16 2009-02-05 19:37:44 bills Exp $

diffRun METADATA
    diff_id     S64         0       # Primary Key AUTO_INCREMENT
    state       STR         64      # Key
    workdir     STR         255
    label       STR         64      # Key
    data_group  STR         64
    dist_group  STR         64
    reduction   STR         64      # Reduction class
    dvodb       STR         255
    registered  TAI         NULL
    tess_id     STR         64      # Key
    bothways    BOOL	    f
    exposure	BOOL	    f
    magicked    S64         0
    software_ver    STR         16
    maskfrac_npix F32       0.0
    maskfrac_static F32	    0.0
    maskfrac_dynamic F32    0.0
    maskfrac_magic  F32     0.0
    maskfrac_advisory F32   0.0
    diff_mode   S16         0
    note        STR         255
END

#
# Diff Sky Cells Mode
#

diffInputSkyfile METADATA
    diff_id         S64         0       # Primary Key fkey(diff_id) ref diffRun(diff_id)
    skycell_id      STR         64      # Primary Key
    warp1           S64         0       # fkey(warp1, skycell_id, tess_id) ref warpSkyfile(warp_id, skycell_id, tess_id)
    stack1          S64         0       # fkey(stack1) ref stackSumSkyfile(stack_id) 
    warp2           S64         0       # fkey(warp2, skycell_id, tess_id) ref warpSkyfile(warp_id, skycell_id, tess_id)
    stack2          S64         0       # fkey(stack2) ref stackSumSkyfile(stack_id) 
    tess_id         STR         64      # Key
    diff_skyfile_id S64         0       # Primary Key
END

diffSkyfile METADATA
    diff_id      S64        0       # Primary Key fkey(diff_id) ref diffRun(diff_id)
    skycell_id   STR        64      # 
    path_base    STR        255
    data_state   STR        64
    bg           F64        0.0
    bg_stdev     F64        0.0
    stamps_num   S32        0
    stamps_mean  F32        0.0
    stamps_rms   F32        0.0
    norm         F32        0.0
    bg_diff      F32        0.0
    kernel_x     F32        0.0
    kernel_y     F32        0.0
    kernel_xx    F32        0.0
    kernel_xy    F32        0.0
    kernel_yy    F32        0.0
    deconv_max   F32        0.0
    sources      S32        0
    dtime_diff   F32        0.0
    dtime_match  F32        0.0
    dtime_phot   F32        0.0
    dtime_script F32        0.0
    hostname     STR        64
    good_frac    F32        0.0     # Key
    fault        S16        0       # Key
    quality      S16        0
    magicked    S64         0
    software_ver     STR        16
    maskfrac_npix S32       0
    maskfrac_static F32	    0.0
    maskfrac_dynamic F32    0.0
    maskfrac_magic  F32     0.0
    maskfrac_advisory F32   0.0
END

diffSummary METADATA
    diff_id	    S64      0       # Primary Key fkey(diff_id) ref diffRun(diff_id)
    projection_cell STR	    32	    # Primary Key
    path_base 	    STR	    255
END