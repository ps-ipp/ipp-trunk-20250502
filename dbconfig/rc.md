rcDestination METADATA
    dest_id     S64         0       # Primary Key
    name        STR         64
    status_uri  STR         255
    comment     STR         255
    last_fileset STR        255
    dbname      STR         64
    dbhost      STR         64
    state       STR         64      # Key
END

rcDSFileset METADATA
    fs_id       S64        0       # Primary Key
    dist_id     S64        0       # fkey(dist_id) ref distRun(dist_id)
    dest_id     S64        0       # fkey(dest_id) ref rcDestination(dest_id)
    name        STR        255
    state       STR        64
    fault       S16        0
END


rcInterest METADATA
    int_id      S64         0       # Primary Key
    dest_id     S64         0       # fkey(dest_id) ref rcDestination(dest_id)
    target_id   S64         0       # fkey(target_id ref distTarget(target_id)
    state       STR         64
END

rcRun METADATA
    rc_id       S64         0       # Primary Key
    fs_id       S64         0       # fkey(fs_id) ref rcDSFileset(fs_id)
    dest_id     S64         0       # Primary Key fkey(dest_id) ref rcDestination(dest_id)
    state       STR         64
    status_fs_name   STR         64
    registered  UTC         0001-01-01T00:00:00Z
    fault       S16        0
END

