
fullForceRun METADATA
    ff_id           S64         0
    skycal_id       S64         0
    sources_path_base STR       255
    state           STR 	64
    workdir         STR         255
    label           STR 	64
    data_group      STR 	64
    dist_group      STR 	64
    note            STR 	255
    reduction       STR 	64
    registered      TAI         NULL
END

fullForceInput METADATA
    ff_id           S64         0
    warp_id         S64         0
END

fullForceResult METADATA
    ff_id           S64         0 
    warp_id         S64         0
    path_base       STR         255
    dtime_script    F32         0.0
    quality         S16         0
    hostname        STR 	64
    software_ver    STR 	16
    fault           S16         0
END

fullForceSummary METADATA
    ff_id           S64         0 
    path_base       STR         255
    dtime_script    F32         0.0
    quality         S16         0
    hostname        STR 	64
    software_ver    STR 	16
    fault           S16         0
END

