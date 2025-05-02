remoteRun METADATA
    remote_id      S64         0    # Primary Key AUTO_INCREMENT
    state          STR         64
    stage          STR         64
    label          STR         64
    path_base      STR         255
    job_id         S64         0
    last_poll      TAI         NULL # DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
    fault          S16         0    # Key
END

remoteComponent METADATA
    remote_id      S64         0    # Primary Key fkey (remote_id) ref remoteRun(remote_id)
    stage_id       S64         0    # Key
    jobs           S32         0    # Key
    state          STR         64
    path_base      STR         255
END
