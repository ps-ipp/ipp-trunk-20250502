dqstatsRun METADATA
    dqstats_id     S64          0     # Primary Key AUTO_INCREMENT
    state          STR          64
    registered     TAI          NULL
    label          STR          64    # Key
    fault          S16          0     # Key NOT NULL
END

dqstatsContent METADATA
    dqstats_id     S64          0     # Key fkey(dqstats_id) ref dqstatsRun(dqstats_id)
    exp_id         S64          0     # Key fkey(exp_id) ref rawExp(exp_id)
    chip_id        S64          0     # Key fkey(chip_id) ref chipRun(chip_id)
    cam_id         S64          0     # Key fkey(cam_id) ref camRun(cam_id)
    warp_id        S64          0     # Key fkey(warp_id) ref warpRun(warp_id)
    invalid        BOOL         false 
END
