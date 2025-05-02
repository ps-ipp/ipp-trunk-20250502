newExp METADATA
    exp_id      S64         0       # Primary Key AUTO_INCREMENT
    summit_id   S64         0       # Key 
    tmp_exp_name STR        64      # Key
    tmp_camera    STR       64      # Key
    tmp_telescope STR       64      # Key
    state       STR         64      # Key
    workdir     STR         255     # destination for output files
    workdir_state STR       64      # key
    reduction   STR         64      # Reduction class
    dvodb       STR         255
    tess_id     STR         64
    end_stage   STR         64      # Key
    label       STR         64      # Key
    epoch       UTC         0001-01-01T00:00:00Z
END

# class needs to be carried here so it can go into rawImfile and be normalized
# from there
newImfile METADATA
    exp_id      S64         64      # Primary Key fkey(exp_id) ref newExp(exp_id)
    tmp_class_id STR        64      # Primary Key
    uri         STR         255
    epoch       UTC         0001-01-01T00:00:00Z
    bytes       S32         0
    md5sum      STR         32
END

