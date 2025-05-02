# paired with rawImfile
rawExp METADATA
    exp_id      S64         64      # Primary Key fkey(exp_id) ref newExp(exp_id)
    exp_name    STR         64      # Key
    camera      STR         64
    telescope   STR         64
    dateobs     UTC         0001-01-01T00:00:00Z
    exp_tag     STR         255
    exp_type    STR         64
    filelevel   STR         64
    workdir     STR         255     # destination for output files
    state       STR         64      # Key
    reduction   STR         64      # Reduction class
    dvodb       STR         255
    tess_id     STR         64
    end_stage   STR         64      # Key
    filter      STR         64
    comment     STR         80
    obs_mode    STR         64      # data usage goal (eg, survey name, engineering, etc)
    obs_group   STR         64      # identifier for data block (eg, observation sequence)
    airmass     F32         0.0
    ra          F64         0.0
    decl        F64         0.0
    exp_time    F32         0.0
    sat_pixel_frac F32      0.0
    bg          F64         0.0
    bg_stdev    F64         0.0
    bg_mean_stdev   F64     0.0
    alt         F64         0.0
    az          F64         0.0
    ccd_temp    F32         0.0
    posang      F64         0.0 
    m1_x        F32         0.0
    m1_y        F32         0.0
    m1_z        F32         0.0
    m1_tip      F32         0.0
    m1_tilt     F32         0.0
    m2_x        F32         0.0
    m2_y        F32         0.0
    m2_z        F32         0.0
    m2_tip      F32         0.0
    m2_tilt     F32         0.0
    env_temperature F32     0.0
    env_humidity    F32     0.0
    env_wind_speed  F32     0.0
    env_wind_dir    F32     0.0
    teltemp_m1      F32     0.0
    teltemp_m1cell  F32     0.0
    teltemp_m2      F32     0.0
    teltemp_spider  F32     0.0
    teltemp_truss   F32     0.0
    teltemp_extra   F32     0.0
    pon_time        F32     0.0
    user_1      F64         0.0
    user_2      F64         0.0
    user_3      F64         0.0
    user_4      F64         0.0
    user_5      F64         0.0
    object      STR         64
    sun_angle   F32         0.0
    sun_alt     F32         0.0
    moon_angle  F32         0.0
    moon_alt    F32         0.0
    moon_phase  F32         0.0
    hostname    STR         64
    fault       S16         0       # Key NOT NULL
    epoch       UTC         0001-01-01T00:00:00Z
    magicked    S64         0
END

rawImfile METADATA
    exp_id      S64         64      # Primary Key fkey(exp_id, tmp_class_id) ref newImfile(exp_id, tmp_class_id)
    exp_name    STR         64      # UINDEX(exp_id, tmp_class_id)
    camera      STR         64
    telescope   STR         64
    dateobs     UTC         0001-01-01T00:00:00Z
    tmp_class_id    STR     64      # Key
    class_id    STR         64      # Primary Key
    uri         STR         255
    data_state  STR         64      # Key
    exp_type    STR         64
# This field is used to set the per exp filelevel. Thus the values for all of
# the imfiles in an exposure need to be sanity checked to make sure that this
# value is in argeement.
    filelevel   STR         64 
    filter      STR         64
    comment     STR         80
    obs_mode    STR         64      # data usage goal (eg, survey name, engineering, etc)
    obs_group   STR         64      # identifier for data block (eg, observation sequence)
    airmass     F32         0.0
    ra          F64         0.0
    decl        F64         0.0
    exp_time    F32         0.0
    sat_pixel_frac F32      0.0
    bg          F64         0.0
    bg_stdev    F64         0.0
    bg_mean_stdev   F64     0.0
    alt         F64         0.0
    az          F64         0.0
    ccd_temp    F32         0.0
    posang      F64         0.0 
    m1_x        F32         0.0
    m1_y        F32         0.0
    m1_z        F32         0.0
    m1_tip      F32         0.0
    m1_tilt     F32         0.0
    m2_x        F32         0.0
    m2_y        F32         0.0
    m2_z        F32         0.0
    m2_tip      F32         0.0
    m2_tilt     F32         0.0
    env_temperature F32     0.0
    env_humidity    F32     0.0
    env_wind_speed  F32     0.0
    env_wind_dir    F32     0.0
    teltemp_m1      F32     0.0
    teltemp_m1cell  F32     0.0
    teltemp_m2      F32     0.0
    teltemp_spider  F32     0.0
    teltemp_truss   F32     0.0
    teltemp_extra   F32     0.0
    pon_time        F32     0.0
    user_1      F64         0.0
    user_2      F64         0.0
    user_3      F64         0.0
    user_4      F64         0.0
    user_5      F64         0.0
    object      STR         64
    sun_angle   F32         0.0
    sun_alt     F32         0.0
    moon_angle  F32         0.0
    moon_alt    F32         0.0
    moon_phase  F32         0.0
    ignored	BOOL        f
    hostname    STR         64
    fault       S16         0       # Key NOT NULL
    quality     S16         0
    epoch       UTC         0001-01-01T00:00:00Z
    magicked    S64         0
    bytes       S32         0
    md5sum      STR         32
    burntool_state  S16     0
    video_cells BOOL        f
END
