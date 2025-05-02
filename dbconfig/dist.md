distTarget METADATA
    target_id   S64        0       # Primary Key
    dist_group  STR        64
    filter      STR        64
    stage       STR        64
    clean       BOOL       f
    state       STR        64
    comment     STR        255
END

distRun METADATA
    dist_id     S64         0       # Primary Key
    target_id   S64         0       # fkey(target_id) ref distTarget(target_id)
    stage       STR         64
    stage_id    S64         0
    magic_ds_id S64         0
    label       STR         64      # Key
    outroot     STR         255
    outdir      STR         255
    clean       BOOL        f
    no_magic    BOOL        f
    alternate   BOOL        f
    state       STR         64      # Key
    time_stamp  UTC         0001-01-01T00:00:00Z
    fault       S16         0
    data_group  STR         64
    note        STR         255
END

distComponent METADATA
    dist_id     S64         0       # Primary Key fkey(dist_id) ref distRun(dist_id)
    component   STR         64      # Key
    bytes       S32         0
    md5sum      STR         32
    state       STR         64      # Key
    outdir      STR         255
    name        STR         255
    fault       S16         0
END

