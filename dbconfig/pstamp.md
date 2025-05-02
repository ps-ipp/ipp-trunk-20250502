pstampDataStore METADATA
    ds_id       S64         0       # Primary Key AUTO_INCREMENT
    state       STR         64
    lastFileset STR         64
    timestamp   UTC         0001-01-01T00:00:00Z
    label       STR         64
    outProduct  STR         64
    uri         STR         255
    pollInterval S32        0
    need_magic  S32         0
END

pstampProject METADATA
    proj_id     S64         0       # Primary Key AUTO_INCREMENT
    name        STR         64      # UNIQUE
    state       STR         64
    dbname      STR         64
    dvodb       STR         64
    camera      STR         64
    telescope   STR         64
    need_magic  BOOL        f
END

pstampRequest   METADATA
    req_id      S64         0       # Primary Key AUTO_INCREMENT
    ds_id       S64         0
    state       STR         64
    name        STR         64      # UNIQUE
    reqType     STR         16
    label       STR         64
    outProduct  STR         64
    uri         STR         255
    outdir      STR         255
    username    STR         255
    proj_id     S64         0
    registered  UTC         0001-01-01T00:00:00Z
    timestamp   UTC         0001-01-01T00:00:00Z
    fault       S32         0
END

pstampJob       METADATA
    job_id      S64         0       # Primary Key AUTO_INCREMENT
    req_id      S64         0       # Primary Key fkey(req_id) ref pstampRequest(req_id)
    rownum      STR         64
    state       STR         64
    jobType     STR         16
    fault       S32         0
    exp_id      S64         0
    outputBase  STR         255
    options     S64         64
    dep_id      S64         0
    fault_count S32         0
    parent_id   S64         0
    is_parent   BOOL        f
END

pstampDependent       METADATA
    dep_id      S64         0       # Primary Key AUTO_INCREMENT
    state       STR         64
    stage       STR         64
    stage_id    S64         0
    component   STR         64
    imagedb     STR         64
    rlabel      STR         64
    need_magic  BOOL        f
    outdir      STR         255
    fault       S16         0
    fault_count S32         0
END

pstampWebRequest        METADATA
    num         S64         0    # Primary Key AUTO_INCREMENT
END

pstampFile              METADATA
    file_id     S64         0   # Primary Key AUTO_INCREMENT
    job_id      S64         0
    path        STR         255
END

pstampUserDomain        METADATA
    domainName      STR     64
    accessLevel     S32     0
    defaultProduct  STR     255
    defaultLabel    STR     64
END

pstampUser              METADATA
    userName        STR     64  # Primary Key (userName, domainName)
    domainName      STR     64  
    accessLevel     S32     0
    defaultProduct  STR     255
    defaultLabel    STR     64
END

pstampAccessLevel       METADATA
    proj_id         S64     0   # Primary Key (proj_id, accessLevel) fkey(proj_id) 
    accessLevel     S32     0
    mjd_min         F32     0
    mjd_max         F32     0
END

