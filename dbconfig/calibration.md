
# table of the DVO databases which get calibrated
calDB METADATA
    cal_id      S64         0       # Primary Key AUTO_INCREMENT
    dvodb       STR         64
    state       STR         64
END

calRun METADATA
    cal_id      S64         0       # Primary Key AUTO_INCREMENT
    region      STR         64      # success or failure
    last_step   STR         64      # fkey(cal_id) ref calDB(cal_id)
    state       STR         64      # 
END

