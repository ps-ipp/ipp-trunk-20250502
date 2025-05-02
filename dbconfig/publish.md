# Tables for publishing data to a science client

publishClient    METADATA 
    client_id    S64         0       # Primary Key AUTO_INCREMENT
    active       BOOL        TRUE
    product      STR         64
    stage        STR         64
    magicked	 BOOL	     TRUE
    workdir      STR         255
    comment      STR         255
    name	 STR	     64
    output_format S16	     0
END              
                 
publishRun       METADATA
    pub_id       S64         0       # Primary Key AUTO_INCREMENT
    client_id    S64         0
    stage_id     S64         0
    label        STR         64
    state        STR         64
END              
                 
publishDone      METADATA
    pub_id       S64         0       # Primary Key
    path_base    STR         255
    hostname     STR         64
    dtime_script F32         0.0
    fault        S16         0
END
