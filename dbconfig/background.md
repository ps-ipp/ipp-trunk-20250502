# Tables to support background replacement

# Background replacement on a chipRun
chipBackgroundRun METADATA
    chip_bg_id          S64     0
    chip_id             S64     0
    cam_id              S64     0
    state               STR     64
    workdir             STR     255
    label               STR     64
    data_group          STR     64
    dist_group          STR     64
    reduction	        STR	64
    note                STR     255
    registered          TAI     NULL
    magicked            S64     0
END

# Results of background replacement from chipBackgroundRun
chipBackgroundImfile METADATA
    chip_bg_id          S64     0
    class_id            STR     64
    path_base           STR     255
    data_state          STR     64
    magicked            S64     0
    dtime_script        F32     0.0
    hostname            STR     64
    quality             S16     0
    fault               S16     0
    software_ver        STR     16
    bg                  F64     0.0
    bg_stdev            F64     0.0
    maskfrac_npix       F32     0.0
    maskfrac_static     F32	0.0
    maskfrac_dynamic    F32     0.0
    maskfrac_magic      F32     0.0
    maskfrac_advisory   F32     0.0
END

# Background replacement on a warpRun (utilising chipBackgroundRun)
warpBackgroundRun METADATA
    warp_bg_id          S64     0
    warp_id             S64     0
    chip_bg_id          S64     0
    state               STR     64
    workdir             STR     255
    label               STR     64
    data_group          STR     64
    dist_group          STR     64
    reduction	        STR	64
    note                STR     255
    registered          TAI     NULL
    magicked            S64     0
END

# Results of background replacement from warpBackgroundRun
warpBackgroundSkyfile METADATA
    warp_bg_id          S64     0
    skycell_id          STR     64
    path_base           STR     255
    data_state          STR     64
    magicked            S64     0
    dtime_script        F32     0.0
    hostname            STR     64
    quality             S16     0
    fault               S16     0
    software_ver        STR     16
    bg                  F64     0.0
    bg_stdev            F64     0.0
    maskfrac_npix       F32     0.0
    maskfrac_static     F32	0.0
    maskfrac_dynamic    F32     0.0
    maskfrac_magic      F32     0.0
    maskfrac_advisory   F32     0.0
END
