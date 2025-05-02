fakeRun METADATA
    fake_id     S64         0       # Primary Key AUTO_INCREMENT
    cam_id      S64         0       # Key INDEX(fake_id, cam_id) fkey (cam_id) ref camRun(cam_id)
    state       STR         64      # Key
    workdir     STR         255 
    label       STR         64      # Key
    data_group  STR         64      # Key
    dist_group  STR         64      # Key
    reduction   STR         64      # Reduction class
    expgroup    STR         64      # Key
    dvodb       STR         255
    tess_id     STR         64
    end_stage   STR         64      # Key
    epoch       UTC         0001-01-01T00:00:00Z
    note        STR         255
END

fakeProcessedImfile METADATA
    fake_id         S64     0       # Primary Key fkey (fake_id) ref fakeRun(fake_id)
    exp_id          S64     64      # Primary Key fkey (exp_id, class_id) ref rawImfile(exp_id, class_id)
    class_id        STR     64      # Primary Key
    uri             STR     255
    dtime_fake      F32     0.0
    dtime_script    F32     0.0
    hostname        STR     64
    path_base       STR     255
    data_state      STR     64      # Key
    fault           S16     0       # Key NOT NULL
    epoch           UTC         0001-01-01T00:00:00Z
END

fakeMask METADATA
    label       STR         64      # Primary Key
END
