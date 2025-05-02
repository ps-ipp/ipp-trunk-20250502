			
diffPhotRun METADATA
    diff_phot_id    S64         0       # Primary Key AUTO_INCREMENT
    diff_id	    S64		0	# Key
    state           STR         64      # Key
    workdir         STR         255
    label           STR         64      # Key
    data_group      STR         64      # Key
    reduction       STR         64
    registered      TAI         NULL
    note            STR         255
    magicked        S64         0
END

diffPhotSkyfile METADATA
    diff_phot_id     S64        0       # Primary Key
    skycell_id       STR        64      # Primary Key
    path_base        STR        255
    dtime_script     F32        0.0
    hostname         STR        64
    fault            S16        0       # Key
    quality          S16        0       # Key
    software_ver     STR        16
    magicked         S64        0
END
