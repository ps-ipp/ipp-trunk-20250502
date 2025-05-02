lapSequence METADATA
    seq_id         S64         0    # Primary Key AUTO_INCREMENT
    name           STR         64   # Key
    description    STR         255
END

lapRun METADATA
    lap_id         S64         0    # Primary Key AUTO_INCREMENT
    seq_id         S64         0    # Key fkey (seq_id) ref lapSequence(seq_id)
    tess_id        STR         64
    projection_cell STR        64   # Key
    filter         STR         64   # Key
    state          STR         64   # Key
    label          STR         64   # Key
    dist_group     STR         64 
    registered     TAI	       NULL 
    fault          S16	       0    # Key
    quick_sass_id  S64         0    # fkey(quick_sass_id) ref stackAssociation(sass_id)
    final_sass_id  S64         0    # fkey(final_sass_id) ref stackAssociation(sass_id)
END

lapExp METADATA
    lap_id         S64         0    # Primary Key fkey (lap_id) ref lapRun(lap_id)
    exp_id         S64         0    # Primary Key fkey (exp_id) ref rawExp(exp_id)
    chip_id        S64         0    # Key fkey (exp_id, chip_id) ref chipRun(exp_id, chip_id)
    pair_id        S64         0    # Key fkey (pair_id) ref chipRun(chip_id)
    private        BOOL        f    
    pairwise       BOOL        f    
    active         BOOL        f
    data_state     STR         64   # Key
END

lapGroup METADATA
    seq_id          S64 0
    tess_id         STR 64
    projection_cell STR 64
    state           STR 64
    label           STR 64
    registered      TAI	NULL 
    fault           S16 0
end
