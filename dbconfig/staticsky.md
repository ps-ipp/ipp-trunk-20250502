# Tables for static sky analysis
# XXX add tess_id and skycell_id here?

staticskyRun METADATA
    sky_id      S64         0       # Primary Key AUTO_INCREMENT
    state       STR         64      # Key
    workdir     STR         255
    label       STR         64      # Key
    data_group  STR         64      # Key
    dist_group  STR         64      # Key
    reduction   STR         64      # Reduction class
    registered  TAI         NULL
    note        STR         255
END

staticskyInput METADATA
    sky_id      S64         0       # Primary Key fkey(sky_id) ref staticskyRun(sky_id)
    stack_id    S64         0       # Primary Key fkey(stack_id) ref stackSumSkyfile(stack_id)
END

staticskyResult METADATA
    sky_id             S64    0       # Primary Key fkey(sky_id) ref staticskyRun(sky_id)
    path_base          STR    255
    dtime_phot         F32    0.0
    dtime_script       F32    0.0
    sources            S32    0
    num_inputs         S32    0
    hostname           STR    64
    good_frac          F32    0.0     # Key
    fault              S16    0       # Key
    quality            S16    0	      # Key
END

skycalRun METADATA
    skycal_id     S64         0       # Primary Key AUTO_INCREMENT
    sky_id      S64         0       # fkey(sky_id) ref staticskyRun(sky_id)
    stack_id    S64         0       # fkey(stack_id) ref stackRun(stack_id)
    state       STR         64      # Key
    workdir     STR         255
    label       STR         64      # Key
    data_group  STR         64      # Key
    dist_group  STR         64      # Key
    reduction   STR         64      # Reduction class
    registered  TAI         NULL
    note        STR         255
END

skycalResult METADATA
    skycal_id        S64    0       # Primary Key fkey(skycal_id) ref skycalRun(skycal_id)
    path_base      STR    255
    dtime_script   F32    0.0
    dtime_astrom   F32    0.0
    sigma_ra       F32    0.0
    sigma_dec      F32    0.0
    n_astrom       S32    0
    n_detections   S32    0
    n_extended     S32    0
    n_forced       S32    0
    zpt_obs        F32    0.0
    zpt_stdev      F32    0.0
    fwhm_major     F32    0.0
    fwhm_minor     F32    0.0
    quality        S16    0	  # Key
    software_ver   STR    16
    hostname       STR    64
    fault          S16    0       # Key
END
