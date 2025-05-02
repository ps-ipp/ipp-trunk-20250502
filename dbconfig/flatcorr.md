
# table of flat-field correction runs
flatcorrRun METADATA
    corr_id     S64         0       # Primary Key AUTO_INCREMENT	
    det_type    STR         64      # type of output detrend correction (eg, FLATCORR, FLATTEST)
    dvodb       STR         64
    camera      STR         64
    telescope   STR         64
    epoch       UTC         0001-01-01T00:00:00Z
    filter      STR         64
    state       STR         64
    make_corr   BOOL        t
    workdir     STR         255 
    label       STR         64
    reduction   STR         64
    region      STR         64
    hostname    STR         64
    fault       S16         0       # Key NOT NULL
END

# table of Exposure-level data used for each flat-field corrction run
flatcorrChipLink METADATA
    corr_id     S64         0       # Primary Key fkey(corr_id) ref flatcorrRun(corr_id)
    chip_id     S64         0       # Primary Key fkey(chip_id) ref chipRun(chip_id)
    include     BOOL        t
END

# table of Exposure-level data used for each flat-field corrction run
flatcorrCamLink METADATA
    corr_id     S64         0       # Primary Key fkey(corr_id) ref flatcorrRun(corr_id)
    chip_id     S64         0       # Primary Key fkey(chip_id) ref chipRun(chip_id)
    cam_id      S64         0       # Primary Key fkey(chip_id) ref chipRun(chip_id)
    include     BOOL        t
END

# table of exposures sent to addstar
flatcorrAddstarLink METADATA
    corr_id     S64         0       # Primary Key fkey(corr_id) ref flatcorrRun(corr_id)
    cam_id      S64         0       # Primary Key fkey(chip_id) ref chipRun(chip_id)
    add_id      S64         0       # Primary Key fkey(chip_id) ref chipRun(chip_id)
    include     BOOL        t
END

