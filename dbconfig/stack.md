# $Id: stack.md,v 1.14 2008-10-07 00:01:28 price Exp $

stackRun METADATA
    stack_id    S64         0       # Primary Key AUTO_INCREMENT
    state       STR         64      # Key
    workdir     STR         255
    label       STR         64      # Key
    data_group  STR         64      # Key
    dist_group  STR         64      # Key
    reduction   STR         64      # Reduction class
    dvodb       STR         255
    registered  TAI         NULL
    skycell_id  STR         64      # Key
    tess_id     STR         64      # Key
    filter      STR         64
    software_ver    STR         16
    note        STR         255
END

stackInputSkyfile METADATA
    stack_id    S64         0       # Primary Key fkey(stack_id) ref stackRun(stack_id)
    warp_id     S64         0       # Primary Key fkey(warp_id) ref warpSkyfile(warp_id)
END

stackSumSkyfile METADATA
    stack_id           S64    0       # Primary Key fkey(stack_id) ref stackRun(stack_id)
    uri                STR    255
    path_base          STR    255
    bg                 F64    0.0
    bg_stdev           F64    0.0
    dtime_stack        F32    0.0 # Key
    dtime_match_mean   F32    0.0 # Key
    dtime_match_stdev  F32    0.0 # Key
    dtime_convolve     F32    0.0 # Key
    dtime_initial      F32    0.0 # Key
    dtime_reject       F32    0.0 # Key
    dtime_final        F32    0.0 # Key
    dtime_phot         F32    0.0 # Key
    dtime_script       F32    0.0
    match_mean         F32    0.0
    match_stdev        F32    0.0
    match_rms          F32    0.0
    stamps_mean        F32    0.0
    stamps_stdev       F32    0.0
    stamps_min         S32    0
    reject_images      S32    0
    reject_pix_mean    F32    0.0
    reject_pix_stdev   F32    0.0
    sources            S32    0
    hostname           STR    64
    good_frac          F32    0.0     # Key
    mjd_obs            F64    0.0
    fault              S16    0       # Key
    software_ver    STR         16
    background_model   S16    0
    quality            S16    0
END

stackSummary METADATA
    sass_id	   S64      0       # Primary Key fkey(sass_id) ref stackAssociation(sass_id)
    projection_cell  STR	    32	    # Primary Key
    path_base 	   STR	    255
END

stackAssociation METADATA
    sass_id        S64      0       # Primary Key AUTO_INCREMENT
    data_group     STR      64      # Key
    projection_cell  STR      64      # Key
    tess_id        STR      64      # Key
    filter         STR      64
END

stackAssociationMap METADATA
    sass_id        S64      0       # Primary Key fkey(sass_id) ref stackAssociation(sass_id)
    stack_id       S64      0       # Primary Key fkey(stack_id) ref stackRun(stack_id)
END

stackExternalCamera METADATA
    ext_camera_id  S64      0       # Primary Key AUTO_INCREMENT
    ext_camera     STR     64
END

stackExternalStack METADATA
    stack_id       S64      0       # Primary Key
    ext_stack_id   S64      0
    ext_camera_id  S64      0       # fkey(ext_camera_id) ref stackExternalCamera(ext_camera_id)
END
