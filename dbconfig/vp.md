vpRun METADATA
    vp_id      S64           0      # Primary Key AUTO_INCREMENT
    exp_id      S64         64      # Key INDEX(vp_id, exp_id) fkey (exp_id) ref rawExp(exp_id)
    state       STR         64      # Key
    label       STR         64      # Key
    data_group  STR         64      # Key
    workdir     STR         255 
    note        STR         255
    dest_id     S64         0
    outroot     STR         255
    dtime_script F32        0.0
    hostname    STR         64
    fault       S16         0       # Key NOT NULL
END

vpProcessedCell METADATA
    vp_id           S64     0       # Primary Key fkey (vp_id) ref vpRun(vp_id)
    class_id        STR     64      # Primary Key
    cell_id         STR     64      # Primary Key
    dtime_photom    F32     0.0
    quality         S16     0
    path_base       STR     255
    fault       S16         0       # Key NOT NULL
END
