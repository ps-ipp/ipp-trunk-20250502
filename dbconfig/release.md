survey METADATA
    surveyID    S32      0   # Primary Key
    surveyName  STR     16
    description STR     255
END

ippRelease METADATA
    rel_id      S32     0   # Primary Key AUTO_INCREMENT
    surveyID    S32     0   # fkey(surveyID) ref survey(surveyID)
    release_name STR    64
    release_state STR   16
    dataRelease S32     0
    priority    S32     0
    dvodb       STR     255
    ubercal_file STR    255
    accessLevelMin S32  0
END

relExp  METADATA
    relexp_id   S64     0   # Primary Key AUTO_INCREMENT
    rel_id      S32     0   # fkey(rel_id) ref ippRelease(rel_id)
    exp_id      S64     0
    chip_id     S64     0
    cam_id      S64     0
    group_id    S32     0
    state       STR     16
    flags       U32     0
    zpt_obs     F32     0
    zpt_stdev   F32     0
    mcal        F32     0
    ubercal_dist S32    0
    path_base   STR     255
    fault       S16     0
    registered  TAI     NULL
    time_stamp  UTC     NULL
END

relStack METADATA
    relstack_id S64     0   # Primary Key AUTO_INCREMENT
    rel_id      S32     0   # fkey(rel_id) ref ippRelease(rel_id)
    stack_id    S64     0   # fkey(stack_id) ref stackRun(stack_id)
    skycal_id   S64     0 
    skycell_id  STR     64
    tess_id     STR     64
    filter      STR     64
    state       STR     16
    flags       U32     0
    stack_type  STR     16
    zpt_obs     F32     0
    zpt_stdev   F32     0
    mjd_obs     U32     0
    path_base   STR     255
    fault       S16     0
    registered  TAI     NULL
    time_stamp  TAI     NULL
END

relGroup METADATA
    group_id    S32     0   # Primary Key AUTO_INCREMENT
    rel_id      S32     0
    group_type  STR     16
    lap_id      S64     0
    group_name  STR     16
    state       STR     16
    label       STR     64
    exp_list_path STR   255
    fault       S16     0
    registered  TAI     NULL
END
