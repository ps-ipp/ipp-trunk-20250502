# XXX rename these 'addstarRun', etc... (add_id -> addstar_id)
addRun METADATA
    add_id          S64     0       # Primary Key AUTO_INCREMENT
    stage 	    STR	    64      # what the stage is (warp, cam, diff,
    stage_id          S64     0       # Key INDEX(add_id,cam_id) fkey(cam_id)    ref camRun(cam_id)
    stage_extra1    S32	    0       
    state           STR     64      # Key
    workdir         STR     255
    workdir_state   STR     64
    reduction       STR	    64
    label           STR	    64
    data_group      STR     64      # Key
    dvodb	    STR	    255
    note            STR     255
    image_only      BOOL    f
    minidvodb	    BOOL    f
    minidvodb_group STR	    64
    minidvodb_name  STR     64
    minidvodb_host  STR     64
END

addProcessedExp METADATA
    add_id          S64     0       # Primary Key AUTO_INCREMENT
    dtime_addstar   F32     0.0
    path_base	    STR	    255
    dvodb_path      STR	    255
    fault           S16     0       # Key NOT NULL
END

addMask METADATA
    label           STR    64       # Primary Key
END



