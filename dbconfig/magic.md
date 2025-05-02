# $Id: magic.md,v 1.13 2008-12-13 20:17:34 bills Exp $

### Fault in magicRun indicates that the processing tree failed
magicRun METADATA
    magic_id    S64         0       # Primary Key AUTO_INCREMENT
    exp_id      S64         0       # Key
    diff_id     S64         0       # Key
    inverse     BOOL	    FALSE	    
    state       STR         64      # Key
    workdir     STR         255
    workdir_state STR       255     # Key
    label       STR         64      # key
    data_group  STR         64
    dvodb       STR         255
    registered  TAI         NULL
    fault       S16         0       # Key
    note        STR         255
END

### This is left over from when diffs were composed of a single skycell
### When we're not too busy, it should be deleted in favour of:
### magicRun JOIN diffSkyfile USING(diff_id) WHERE diffSkyfile.fault = 0 AND diffSkyfile.quality = 0
magicInputSkyfile METADATA
    magic_id    S64         0       # Primary Key fkey(magic_id) ref magicRun(magic_id)
    node        STR         64      #
END

magicTree METADATA
    magic_id    S64         0       # Key fkey(magic_id) ref magicRun(magic_id)
    node        STR         64      # Key INDEX(magic_id, node)
    dep         STR         64      # Key
END

magicNodeResult METADATA
    magic_id    S64         0       # Primary Key fkey(magic_id) ref magicRun(magic_id)
    node        STR         64      # Primary Key fkey(magic_id, node) ref magicTree(magic_id, node)
    path_base   STR         255
    fault       S16         0       # Key
END

magicMask METADATA
    magic_id    S64         0       # Primary Key fkey(magic_id) ref magicRun(magic_id)
    uri         STR         255
    path_base   STR         255
    streaks     S32         0
    fault       S16         0       # Key
END

magicDSRun METADATA
    magic_ds_id S64         0       # Primary Key
    magic_id    S64         0       # Primary Key fkey(magic_id) ref magicRun(magic_id)
    inv_magic_id S64        0      # Primary Key fkey(inverse_magic_id) ref magicRun(magic_id)
    state       STR         0       # Key
    stage       STR         64
    stage_id    S64         0
    cam_id      S64         0
    label       STR         64      # key
    data_group  STR         64      # key
    outroot     STR         255
    recoveryroot    STR     255
    re_place    BOOL        f
    remove      BOOL        f
    fault       S16         0       # Key
    note        STR         255
END

magicDSFile METADATA
    magic_ds_id S64         0       # Primary Key fkey(magic_ds_id) ref magicDSRun(magic_ds_id)
    component   STR         64
    backup_path_base  STR         255
    recovery_path_base STR        255
    streak_frac F32         0
    nondiff_frac F32         0
    run_time    F32         0
    fault       S16         0
    data_state  STR        64     # Key
END
