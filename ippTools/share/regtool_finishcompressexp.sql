select DISTINCT
    exp_id,
    exp_name,
    camera,   
    telescope,
    dateobs,
    exp_tag,
    exp_type,
    filelevel,
    workdir,
    state,
    reduction,    
    dvodb,
    tess_id,
    end_stage,
    filter,
    comment,
    obs_mode,
    obs_group,
    airmass,
    ra,
    decl,
    exp_time,
    sat_pixel_frac,
    bg,
    bg_stdev,
    bg_mean_stdev,
    alt,
    az,
    ccd_temp,
    posang,
    m1_x,
    m1_y,
    m1_z,
    m1_tip,
    m1_tilt,
    m2_x,
    m2_y,
    m2_z,
    m2_tip,
    m2_tilt,
    env_temperature,
    env_humidity,
    env_wind_speed,
    env_wind_dir,
    teltemp_m1,
    teltemp_m1cell,
    teltemp_m2,
    teltemp_spider,
    teltemp_truss,
    teltemp_extra,
    pon_time,
    user_1,
    user_2,
    user_3,
    user_4,
    user_5,
    object,
    sun_angle,
    sun_alt,
    moon_angle,
    moon_alt,
    moon_phase,
    hostname,
    fault,
    epoch,
    magicked
FROM
    (SELECT V.*,
     count(class_id) AS complete
     FROM 
     	  (SELECT rawExp.*,count(newImfile.tmp_class_id) AS total
	   FROM rawExp
	   LEFT JOIN newImfile ON rawExp.exp_id = newImfile.exp_id
	   WHERE (rawExp.state = 'goto_compressed' OR rawExp.state = 'goto_lossy') 
	   GROUP BY rawExp.exp_id) AS V
     JOIN rawImfile ON V.exp_id = rawImfile.exp_id
     WHERE ((V.state = 'goto_compressed' AND rawImfile.data_state = 'compressed') OR
     	    (V.state = 'goto_lossy' AND rawImfile.data_state = 'lossy'))
     GROUP BY V.exp_id) as foo
WHERE (foo.complete = foo.total)
	