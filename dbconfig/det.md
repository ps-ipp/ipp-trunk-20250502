detRun METADATA
    det_id      S64         0       # Primary Key AUTO_INCREMENT
    iteration   S32         0       # Key INDEX(det_id, iteration)
    det_type    STR         64      # Key
    mode        STR         64      # Key
    state       STR         64      # Key
    filelevel   STR         64
    workdir     STR         255     # destination for output files
    camera      STR         64
    telescope   STR         64
    exp_type    STR         64      # XXX this should be dropped
    reduction   STR         64      # Reduction clas
    filter      STR         64
    airmass_min F32         0.0
    airmass_max F32         0.0
    exp_time_min F32        0.0
    exp_time_max F32        0.0
    ccd_temp_min F32        0.0
    ccd_temp_max F32        0.0
    posang_min  F64         0.0 
    posang_max  F64         0.0 
    registered  TAI         0001-01-01T00:00:00Z
    time_begin  TAI         0001-01-01T00:00:00Z
    time_end    TAI         0001-01-01T00:00:00Z
    use_begin   TAI         0001-01-01T00:00:00Z
    use_end     TAI         0001-01-01T00:00:00Z
    solang_min  F32         0.0
    solang_max  F32         0.0
    label       STR         64      # key
    ref_det_id  S64         0	    # reference for 'verify' and 'correction' analysis
    ref_iter    S32         0	    # reference for 'verify' and 'correction' analysis
END

detInputExp METADATA
    det_id      S64         0       # Primary Key fkey(det_id) ref detRun(det_id)
    iteration   S32         0       # Primary Key fkey(exp_id) ref rawExp(exp_id)
    exp_id     S64         64       # Primary Key INDEX(det_id, exp_id)
    include     BOOL        f       # INDEX(det_id, iteration)
END

detProcessedImfile METADATA
    det_id      S64         0       # Primary Key fkey(det_id, exp_id) ref detInputExp(det_id, exp_id) 
    exp_id     S64         64       # Primary Key fkey(exp_id, class_id) ref rawImfile(exp_id, class_id)
    class_id    STR         64      # Primary Key INDEX(det_id, class_id)
    uri         STR         255     # INDEX(det_id, exp_id)
    recipe      STR         64
    bg          F64         0.0
    bg_stdev    F64         0.0
    bg_mean_stdev   F64     0.0
    fringe_0    F64         0.0
    fringe_1    F64         0.0
    fringe_2    F64         0.0
    user_1      F64         0.0
    user_2      F64         0.0
    user_3      F64         0.0
    user_4      F64         0.0
    user_5      F64         0.0
    path_base   STR         255
    data_state  STR         64      # full, cleaned, purged (only track end states; request states are in detRunSummary by iteration)
    fault       S16         0       # Key NOT NULL
END

detProcessedExp METADATA
    det_id      S64         0       # Primary Key fkey(det_id, exp_id) ref detInputExp(det_id, exp_id)
    exp_id     S64         64       # Primary Key fkey(det_id, exp_id) ref detProcessedImfile(det_id, exp_id)
    recipe      STR         64
    bg          F64         0.0
    bg_stdev    F64         0.0
    bg_mean_stdev   F64     0.0
    fringe_0    F64         0.0
    fringe_1    F64         0.0
    fringe_2    F64         0.0
    user_1      F64         0.0
    user_2      F64         0.0
    user_3      F64         0.0
    user_4      F64         0.0
    user_5      F64         0.0
    path_base   STR         255
    data_state  STR         64      # full, cleaned, purged (only track end states; request states are in detRunSummary by iteration)
    fault       S16         0       # Key NOT NULL
END

#
# detStackedImfile does not depend on detProcessedExp. detProcesedExp is purely
# FYI values and can be calculated in parellel
#
detStackedImfile METADATA
    det_id      S64         0       # Primary Key fkey(det_id, iteration) ref detInputExp(det_id, iteration)
    iteration   S32         0       # Primary Key fkey(det_id, class_id) ref detProcessedImfile(det_id, class_id)
    class_id    STR         64      # Primary Key
    uri         STR         255
    # XXX missing path_base
    recipe      STR         64
    bg          F64         0.0
    bg_stdev    F64         0.0
    bg_mean_stdev   F64     0.0
    user_1      F64         0.0
    user_2      F64         0.0
    user_3      F64         0.0
    user_4      F64         0.0
    user_5      F64         0.0
    #   XXX does it make sense to 'clean' the stacked imfiled? (EAM: yes)
    data_state  STR         64      # full, cleaned, purged (only track end states; request states are in detRunSummary by iteration)
    fault       S16         0       # Key NOT NULL
END

detNormalizedStatImfile METADATA
    det_id      S64         0       # Primary Key fkey(det_id, iteration) ref detInputExp(det_id, iteration)
    iteration   S32         0       # Primary Key fkey(det_id, iteration, class_id) ref detStackedImfile(det_id, iteration, class_id)
    class_id    STR         64      # Primary Key
    norm        F32         0.0
    data_state  STR         64      # full, cleaned, purged (only track end states; request states are in detRunSummary by iteration)
    fault       S16         0       # Key NOT NULL
END

## XXX this must match in fields with detRegisteredImfile
detNormalizedImfile METADATA
    det_id      S64         0       # Primary Key fkey(det_id) ref detInputExp(det_id)
    iteration   S32         0       # Primary Key fkey(det_id, iteration, class_id) ref detNormalizedStatImfile(det_id, iteration, class_id)
    class_id    STR         64      # Primary Key INDEX(det_id, iteration)
    uri         STR         255
    bg          F64         0.0
    bg_stdev    F64         0.0
    bg_mean_stdev   F64     0.0
    user_1      F64         0.0
    user_2      F64         0.0
    user_3      F64         0.0
    user_4      F64         0.0
    user_5      F64         0.0
    path_base   STR         255
    data_state  STR         64      # full, cleaned, purged (only track end states; request states are in detRunSummary by iteration)
    fault       S16         0       # Key NOT NULL
END

detNormalizedExp METADATA
    det_id      S64         0       # Primary Key fkey(det_id, iteration) ref detInputExp(det_id, iteration)
    iteration   S32         0       # Primary Key fkey(det_id, iteration) ref detNormalizedImfile(det_id, iteration)
    recipe      STR         64
    bg          F64         0.0
    bg_stdev    F64         0.0
    bg_mean_stdev   F64     0.0
    user_1      F64         0.0
    user_2      F64         0.0
    user_3      F64         0.0
    user_4      F64         0.0
    user_5      F64         0.0
    path_base   STR         255
    data_state  STR         64      # full, cleaned, purged (only track end states; request states are in detRunSummary by iteration)
    fault       S16         0       # Key NOT NULL
END

detResidImfile METADATA
    det_id      S64         0       # Primary Key fkey(det_id, iteration, exp_id) ref detInputExp(det_id, iteration, exp_id)
    iteration   S32         0       # Primary Key fkey(det_id, exp_id, class_id) ref detProcessedImfile(det_id, exp_id, class_id)
    ref_det_id  S64         0	    # detrend master actually applied (same as above for 'master', but not for 'verify')
    ref_iter    S32         0	    # detrend master actually applied (same as above for 'master', but not for 'verify')
    exp_id      S64         64      # Primary Key fkey(det_id, iteration) ref detNormalizedExp(det_id, iteration)
    class_id    STR         64      # Primary Key INDEX(det_id, iteration, exp_id)
    uri         STR         255
    recipe      STR         64
    bg          F64         0.0
    bg_stdev    F64         0.0
    bg_mean_stdev   F64     0.0
    bg_skewness F64         0.0
    bg_kurtosis F64         0.0
    bin_stdev   F64         0.0
    fringe_0    F64         0.0
    fringe_1    F64         0.0
    fringe_2    F64         0.0
    fringe_resid_0  F64     0.0
    fringe_resid_1  F64     0.0
    fringe_resid_2  F64     0.0
    user_1      F64         0.0
    user_2      F64         0.0
    user_3      F64         0.0
    user_4      F64         0.0
    user_5      F64         0.0
    path_base   STR         255
    data_state  STR         64      # full, cleaned, purged (only track end states; request states are in detRunSummary by iteration)
    fault       S16         0       # Key NOT NULL
END

detResidExp METADATA
    det_id      S64         0       # Primary Key fkey(det_id, iteration, exp_id) ref detInputExp(det_id, iteration, exp_id)
    iteration   S32         0       # Primary Key fkey(det_id, iteration, exp_id) ref detResidImfile(det_id, iteration, exp_id)
    exp_id     S64         64       # Primary Key INDEX(det_id, iteration)
    recipe      STR         64
    bg          F64         0.0
    bg_stdev    F64         0.0
    bg_mean_stdev   F64     0.0
    bg_skewness F64         0.0
    bg_kurtosis F64         0.0
    bin_stdev   F64         0.0
    fringe_0    F64         0.0
    fringe_1    F64         0.0
    fringe_2    F64         0.0
    fringe_resid_0  F64     0.0
    fringe_resid_1  F64     0.0
    fringe_resid_2  F64     0.0
    user_1      F64         0.0
    user_2      F64         0.0
    user_3      F64         0.0
    user_4      F64         0.0
    user_5      F64         0.0
    path_base   STR         255
    data_state  STR         64      # full, cleaned, purged (only track end states; request states are in detRunSummary by iteration)
    accept      BOOL        f
    fault       S16         0       # Key NOT NULL
END

# XXX this probably should have been defined at the start of a new iteration, not at the end.
detRunSummary METADATA
    det_id      S64         0       # Primary Key fkey(det_id, iteration) ref detInputExp(det_id, iteration)
    iteration   S32         0       # Primary Key fkey(det_id, iteration) ref detResidExp(det_id, iteration)
    data_state  STR         64      # full, goto_cleaned, cleaned, goto_full, goto_purged, purged
    bg          F64         0.0
    bg_stdev    F64         0.0
    bg_mean_stdev   F64     0.0
    accept      BOOL        f
    fault       S16         0       # Key NOT NULL
END

#
# Note: This table needs to stay more or less identical to detNormalizedImfile.
# It only exists as a seperate entity so they it can have different fkeys
#
detRegisteredImfile METADATA
    det_id      S64         0       # Primary Key fkey(det_id, iteration) ref detRun(det_id, iteration)
    iteration   S32         0       # Primary Key
    class_id    STR         64      # Primary Key
    uri         STR         255
    bg          F64         0.0
    bg_stdev    F64         0.0
    bg_mean_stdev   F64     0.0
    user_1      F64         0.0
    user_2      F64         0.0
    user_3      F64         0.0
    user_4      F64         0.0
    user_5      F64         0.0
    path_base   STR         255
    data_state  STR         64      # full, cleaned, purged (only track end states; request states are in detRunSummary by iteration)
    fault       S16         0       # Key NOT NULL
END

detCorrectedExp METADATA
    det_id      S64         0       # Primary Key fkey(det_id, exp_id) ref detInputExp(det_id, exp_id) 
    exp_id      S64         64      # Primary Key fkey(exp_id, class_id) ref rawImfile(exp_id, class_id)
    uri         STR         255     # INDEX(det_id, exp_id)
    corr_id     S64         0
    corr_type   STR         64
    recipe      STR         64
    path_base   STR         255
    fault       S16         0       # Key NOT NULL
END

detCorrectedImfile METADATA
    det_id      S64         0       # Primary Key fkey(det_id, exp_id) ref detInputExp(det_id, exp_id) 
    exp_id     S64         64       # Primary Key fkey(exp_id, class_id) ref rawImfile(exp_id, class_id)
    class_id    STR         64      # Primary Key INDEX(det_id, class_id)
    uri         STR         255     # INDEX(det_id, exp_id)
    path_base   STR         255
    fault       S16         0       # Key NOT NULL
END
