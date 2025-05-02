CREATE TABLE dbversion (
    schema_version VARCHAR(64),
    updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE pzDataStore (
    camera VARCHAR(64),
    telescope VARCHAR(64),
    uri VARCHAR(255),
    epoch TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    use_compress SMALLINT,
    PRIMARY KEY(camera, telescope)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE summitExp (
    summit_id BIGINT NOT NULL AUTO_INCREMENT,
    exp_name VARCHAR(64),
    camera VARCHAR(64),
    telescope VARCHAR(64),
    dateobs DATETIME,
    exp_type VARCHAR(64),
    uri VARCHAR(255),
    imfiles INT,
    fault SMALLINT NOT NULL,
    epoch TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY(summit_id),
    KEY(exp_name, camera, telescope),
    KEY(fault)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE summitImfile (
    summit_id BIGINT,
    exp_name VARCHAR(64),
    camera VARCHAR(64),
    telescope VARCHAR(64),
    file_id VARCHAR(64),
    bytes INT,
    md5sum VARCHAR(32),
    class VARCHAR(64),
    class_id VARCHAR(64),
    uri VARCHAR(255),
    epoch TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY(summit_id, class, class_id),
    KEY(exp_name, camera, telescope, class, class_id),
    KEY(file_id),
    FOREIGN KEY(summit_id) REFERENCES summitExp(summit_id),
    FOREIGN KEY(exp_name, camera, telescope) REFERENCES summitExp(exp_name, camera, telescope)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE pzDownloadExp (
    summit_id BIGINT,
    exp_name VARCHAR(64),
    camera VARCHAR(64),
    telescope VARCHAR(64),
    state VARCHAR(64),
    epoch TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY(summit_id),
    KEY(exp_name, camera, telescope),
    KEY(state),
    FOREIGN KEY(summit_id) REFERENCES summitExp(summit_id),
    FOREIGN KEY(exp_name, camera, telescope) REFERENCES summitExp(exp_name, camera, telescope)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE pzDownloadImfile (
    summit_id BIGINT,
    exp_name VARCHAR(64),
    camera VARCHAR(64),
    telescope VARCHAR(64),
    class VARCHAR(64),
    class_id VARCHAR(64),
    uri VARCHAR(255),
    fault SMALLINT NOT NULL,
    epoch TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    hostname VARCHAR(64),
    bytes INT,
    md5sum VARCHAR(32),
    PRIMARY KEY(summit_id, class, class_id),
    KEY(exp_name, camera, telescope, class, class_id),
    KEY(fault),
    FOREIGN KEY(summit_id) REFERENCES pzDownloadExp(summit_id),
    FOREIGN KEY(summit_id, class, class_id) REFERENCES summitImfile(summit_id, class, class_id),
    FOREIGN KEY (exp_name, camera, telescope) REFERENCES pzDownloadExp(exp_name, camera, telescope),
    FOREIGN KEY(exp_name, camera, telescope, class, class_id) REFERENCES summitImfile(exp_name, camera, telescope, class, class_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE newExp (
    exp_id BIGINT AUTO_INCREMENT,
    summit_id BIGINT,
    tmp_exp_name VARCHAR(64),
    tmp_camera VARCHAR(64),
    tmp_telescope VARCHAR(64),
    state VARCHAR(64),
    workdir VARCHAR(255),
    workdir_state VARCHAR(64),
    reduction VARCHAR(64),
    dvodb VARCHAR(255),
    tess_id VARCHAR(64),
    end_stage VARCHAR(64),
    label VARCHAR(64),
    epoch TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY(exp_id),
    KEY(exp_id),
    KEY(summit_id),
    KEY(tmp_exp_name),
    KEY(tmp_camera),
    KEY(tmp_telescope),
    KEY(state),
    KEY(workdir_state),
    KEY(end_stage),
    KEY(label)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE newImfile (
    exp_id BIGINT,
    tmp_class_id VARCHAR(64),
    uri VARCHAR(255),
    epoch TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    bytes INT,
    md5sum VARCHAR(32),
    PRIMARY KEY(exp_id, tmp_class_id),
    FOREIGN KEY(exp_id) REFERENCES newExp(exp_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE rawExp (
    exp_id BIGINT,
    exp_name VARCHAR(64),
    camera VARCHAR(64),
    telescope VARCHAR(64),
    dateobs DATETIME,
    exp_tag VARCHAR(255),
    exp_type VARCHAR(64),
    filelevel VARCHAR(64),
    workdir VARCHAR(255),
    state VARCHAR(64),
    reduction VARCHAR(64),
    dvodb VARCHAR(255),
    tess_id VARCHAR(64),
    end_stage VARCHAR(64),
    filter VARCHAR(64),
    comment VARCHAR(80),
    obs_mode VARCHAR(64),
    obs_group VARCHAR(64),
    airmass FLOAT,
    ra DOUBLE,
    decl DOUBLE,
    exp_time FLOAT,
    sat_pixel_frac FLOAT,
    bg DOUBLE,
    bg_stdev DOUBLE,
    bg_mean_stdev DOUBLE,
    alt DOUBLE,
    az DOUBLE,
    ccd_temp FLOAT,
    posang DOUBLE,
    m1_x FLOAT,
    m1_y FLOAT,
    m1_z FLOAT,
    m1_tip FLOAT,
    m1_tilt FLOAT,
    m2_x FLOAT,
    m2_y FLOAT,
    m2_z FLOAT,
    m2_tip FLOAT,
    m2_tilt FLOAT,
    env_temperature FLOAT,
    env_humidity FLOAT,
    env_wind_speed FLOAT,
    env_wind_dir FLOAT,
    teltemp_m1 FLOAT,
    teltemp_m1cell FLOAT,
    teltemp_m2 FLOAT,
    teltemp_spider FLOAT,
    teltemp_truss FLOAT,
    teltemp_extra FLOAT,
    pon_time FLOAT,
    user_1 DOUBLE,
    user_2 DOUBLE,
    user_3 DOUBLE,
    user_4 DOUBLE,
    user_5 DOUBLE,
    object VARCHAR(64),
    sun_angle FLOAT,
    sun_alt FLOAT,
    moon_angle FLOAT,
    moon_alt FLOAT,
    moon_phase FLOAT,
    hostname VARCHAR(64),
    fault SMALLINT NOT NULL,
    epoch TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    magicked BIGINT,
    PRIMARY KEY(exp_id),
    KEY(end_stage),
    KEY(fault),
    KEY(state),
    KEY(exp_name),
    FOREIGN KEY(exp_id) REFERENCES newExp(exp_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE rawImfile (
    exp_id BIGINT,
    exp_name VARCHAR(64),
    camera VARCHAR(64),
    telescope VARCHAR(64),
    dateobs DATETIME,
    tmp_class_id VARCHAR(64),
    class_id VARCHAR(64),
    uri VARCHAR(255),
    data_state VARCHAR(64),
    exp_type VARCHAR(64),
    filelevel VARCHAR(64),
    filter VARCHAR(64),
    comment VARCHAR(80),
    obs_mode VARCHAR(64),
    obs_group VARCHAR(64),
    airmass FLOAT,
    ra DOUBLE,
    decl DOUBLE,
    exp_time FLOAT,
    sat_pixel_frac FLOAT,
    bg DOUBLE,
    bg_stdev DOUBLE,
    bg_mean_stdev DOUBLE,
    alt DOUBLE,
    az DOUBLE,
    ccd_temp FLOAT,
    posang DOUBLE,
    m1_x FLOAT,
    m1_y FLOAT,
    m1_z FLOAT,
    m1_tip FLOAT,
    m1_tilt FLOAT,
    m2_x FLOAT,
    m2_y FLOAT,
    m2_z FLOAT,
    m2_tip FLOAT,
    m2_tilt FLOAT,
    env_temperature FLOAT,
    env_humidity FLOAT,
    env_wind_speed FLOAT,
    env_wind_dir FLOAT,
    teltemp_m1 FLOAT,
    teltemp_m1cell FLOAT,
    teltemp_m2 FLOAT,
    teltemp_spider FLOAT,
    teltemp_truss FLOAT,
    teltemp_extra FLOAT,
    pon_time FLOAT,
    user_1 DOUBLE,
    user_2 DOUBLE,
    user_3 DOUBLE,
    user_4 DOUBLE,
    user_5 DOUBLE,
    object VARCHAR(64),
    sun_angle FLOAT,
    sun_alt FLOAT,
    moon_angle FLOAT,
    moon_alt FLOAT,
    moon_phase FLOAT,
    ignored TINYINT DEFAULT 0,
    hostname VARCHAR(64),
    quality SMALLINT NOT NULL DEFAULT 0,
    fault SMALLINT NOT NULL,
    epoch TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    raw_image_id BIGINT AUTO_INCREMENT,
    magicked BIGINT,
    bytes INT,
    md5sum VARCHAR(32),
    burntool_state SMALLINT,
    video_cells TINYINT DEFAULT 0,
    PRIMARY KEY(exp_id, class_id),
    KEY(tmp_class_id),
    KEY(fault),
    KEY(raw_image_id),
    KEY(quality),
    KEY(dateobs),
    UNIQUE KEY(exp_id, tmp_class_id),
    FOREIGN KEY(exp_id, tmp_class_id) REFERENCES newImfile(exp_id, tmp_class_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE chipRun (
    chip_id BIGINT AUTO_INCREMENT,
    exp_id BIGINT,
    state VARCHAR(64),
    workdir VARCHAR(255),
    workdir_state VARCHAR(64),
    label VARCHAR(64),
    data_group VARCHAR(64),
    dist_group VARCHAR(64),
    reduction VARCHAR(64),
    expgroup VARCHAR(64),
    dvodb VARCHAR(255),
    tess_id VARCHAR(64),
    end_stage VARCHAR(64),
    magicked BIGINT,
    software_ver VARCHAR(16),
    maskfrac_npix FLOAT,
    maskfrac_static FLOAT,
    maskfrac_dynamic FLOAT,
    maskfrac_magic FLOAT,
    maskfrac_advisory FLOAT,
    update_mode SMALLINT DEFAULT 0,
    note VARCHAR(255),
    PRIMARY KEY(chip_id),
    KEY(chip_id), KEY(exp_id),
    KEY(state),
    KEY(workdir_state),
    KEY(label),
    KEY(data_group),
    KEY(dist_group),
    KEY(expgroup),
    KEY(end_stage),
    INDEX(chip_id, exp_id),
    FOREIGN KEY(exp_id) REFERENCES rawExp(exp_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE chipImfile (
    chip_id BIGINT,
    class_id VARCHAR(64),
    chip_imfile_id BIGINT AUTO_INCREMENT,
    PRIMARY KEY(chip_id, class_id),
    KEY(chip_imfile_id),
    FOREIGN KEY(chip_id) REFERENCES chipRun(chip_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE chipProcessedImfile (
    chip_id BIGINT,
    exp_id BIGINT,
    class_id VARCHAR(64),
    data_state VARCHAR(64),
    uri VARCHAR(255),
    bg FLOAT,
    bg_stdev FLOAT,
    bg_mean_stdev FLOAT,
    bias FLOAT,
    bias_stdev FLOAT,
    fringe_0 FLOAT,
    fringe_1 FLOAT,
    fringe_2 FLOAT,
    ap_resid FLOAT,
    ap_resid_stdev FLOAT,
    fwhm_major FLOAT,
    fwhm_major_lq FLOAT,
    fwhm_major_uq FLOAT,
    fwhm_minor FLOAT,
    fwhm_minor_lq FLOAT,
    fwhm_minor_uq FLOAT,
    iq_fwhm_major FLOAT,
    iq_fwhm_major_err FLOAT,
    iq_fwhm_minor FLOAT,
    iq_fwhm_minor_err FLOAT,
    iq_m2 FLOAT,
    iq_m2_err FLOAT,
    iq_m2_lq FLOAT,
    iq_m2_uq FLOAT,
    iq_m2c FLOAT,
    iq_m2c_err FLOAT,
    iq_m2c_lq FLOAT,
    iq_m2c_uq FLOAT,
    iq_m2s FLOAT,
    iq_m2s_err FLOAT,
    iq_m2s_lq FLOAT,
    iq_m2s_uq FLOAT,
    iq_m3 FLOAT,
    iq_m3_err FLOAT,
    iq_m3_lq FLOAT,
    iq_m3_uq FLOAT,
    iq_m4 FLOAT,
    iq_m4_err FLOAT,
    iq_m4_lq FLOAT,
    iq_m4_uq FLOAT,
    dtime_detrend FLOAT,
    dtime_photom FLOAT,
    dtime_total FLOAT,
    dtime_script FLOAT,
    hostname VARCHAR(64),
    n_stars INT,
    n_psfstars INT,
    n_iqstars INT,
    n_extended INT,
    n_cr INT,
    path_base VARCHAR(255),
    quality SMALLINT NOT NULL DEFAULT 0,
    fault SMALLINT NOT NULL,
    magicked BIGINT,
    software_ver VARCHAR(16),
    maskfrac_npix INT,
    maskfrac_static FLOAT,
    maskfrac_dynamic FLOAT,
    maskfrac_magic FLOAT,
    maskfrac_advisory FLOAT,
    deteff_magref FLOAT,
    PRIMARY KEY(chip_id, exp_id, class_id),
    KEY(data_state),
    KEY(fault),
    KEY(quality),
    KEY(data_state),
    FOREIGN KEY(chip_id, exp_id) REFERENCES chipRun(chip_id, exp_id),
    FOREIGN KEY(exp_id, class_id) REFERENCES rawImfile(exp_id, class_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE chipMask (
    label VARCHAR(64),
    PRIMARY KEY(label)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE camRun (
    cam_id BIGINT AUTO_INCREMENT,
    chip_id BIGINT,
    state VARCHAR(64),
    workdir VARCHAR(255),
    workdir_state VARCHAR(64),
    label VARCHAR(64),
    data_group VARCHAR(64),
    dist_group VARCHAR(64),
    reduction VARCHAR(64),
    expgroup VARCHAR(64),
    dvodb VARCHAR(255),
    tess_id VARCHAR(64),
    end_stage VARCHAR(64),
    magicked BIGINT,
    software_ver VARCHAR(16),
    maskfrac_ref_npix FLOAT,
    maskfrac_ref_static FLOAT,
    maskfrac_ref_dynamic FLOAT,
    maskfrac_ref_magic FLOAT,
    maskfrac_ref_advisory FLOAT,
    maskfrac_max_npix FLOAT,
    maskfrac_max_static FLOAT,
    maskfrac_max_dynamic FLOAT,
    maskfrac_max_magic FLOAT,
    maskfrac_max_advisory FLOAT,
    note VARCHAR(255),
    PRIMARY KEY(cam_id),
    KEY(cam_id),
    KEY(chip_id),
    KEY(state),
    KEY(workdir_state),
    KEY(label),
    KEY(data_group),
    KEY(dist_group),
    KEY(expgroup),
    KEY(end_stage),
    INDEX(cam_id, chip_id),
    FOREIGN KEY(chip_id) REFERENCES chipRun(chip_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE camProcessedExp (
    cam_id BIGINT,
    uri VARCHAR(255),
    bg FLOAT,
    bg_stdev FLOAT,
    bg_mean_stdev FLOAT,
    bias FLOAT,
    bias_stdev FLOAT,
    fringe_0 FLOAT,
    fringe_1 FLOAT,
    fringe_2 FLOAT,
    sigma_ra FLOAT,
    sigma_dec FLOAT,
    ap_resid FLOAT,
    ap_resid_stdev FLOAT,
    zpt_obs FLOAT,
    zpt_stdev FLOAT,
    zpt_lq FLOAT,
    zpt_uq FLOAT,
    fwhm_major FLOAT,
    fwhm_major_lq FLOAT,
    fwhm_major_uq FLOAT,
    fwhm_minor FLOAT,
    fwhm_minor_lq FLOAT,
    fwhm_minor_uq FLOAT,
    iq_fwhm_major FLOAT,
    iq_fwhm_major_err FLOAT,
    iq_fwhm_minor FLOAT,
    iq_fwhm_minor_err FLOAT,
    iq_m2 FLOAT,
    iq_m2_err FLOAT,
    iq_m2_lq FLOAT,
    iq_m2_uq FLOAT,
    iq_m2c FLOAT,
    iq_m2c_err FLOAT,
    iq_m2c_lq FLOAT,
    iq_m2c_uq FLOAT,
    iq_m2s FLOAT,
    iq_m2s_err FLOAT,
    iq_m2s_lq FLOAT,
    iq_m2s_uq FLOAT,
    iq_m3 FLOAT,
    iq_m3_err FLOAT,
    iq_m3_lq FLOAT,
    iq_m3_uq FLOAT,
    iq_m4 FLOAT,
    iq_m4_err FLOAT,
    iq_m4_lq FLOAT,
    iq_m4_uq FLOAT,
    dtime_script FLOAT,
    dtime_astrom FLOAT,
    dtime_addstar FLOAT,
    hostname VARCHAR(64),
    n_stars INT,
    n_psfstars INT,
    n_iqstars INT,
    n_extended INT,
    n_cr INT,
    n_astrom INT,
    path_base VARCHAR(255),
    fault SMALLINT NOT NULL,
    software_ver VARCHAR(16),
    maskfrac_ref_npix INT,
    maskfrac_ref_static FLOAT,
    maskfrac_ref_dynamic FLOAT,
    maskfrac_ref_magic FLOAT,
    maskfrac_ref_advisory FLOAT,
    maskfrac_max_npix INT,
    maskfrac_max_static FLOAT,
    maskfrac_max_dynamic FLOAT,
    maskfrac_max_magic FLOAT,
    maskfrac_max_advisory FLOAT,
    deteff FLOAT,
    deteff_err FLOAT,
    deteff_lq FLOAT,
    deteff_uq FLOAT,
    quality SMALLINT NOT NULL DEFAULT 0,
    background_model SMALLINT,
    astrom_chips BIGINT,
    ast_r0 FLOAT,
    ast_d0 FLOAT,
    ast_t0 FLOAT,
    ast_s0 FLOAT,
    ast_rs FLOAT,
    ast_ds FLOAT,
    PRIMARY KEY(cam_id),
    KEY(fault),
    KEY(quality),
    FOREIGN KEY(cam_id) REFERENCES camRun(cam_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE camMask (
    label VARCHAR(64),
    PRIMARY KEY(label)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE addRun (
    add_id BIGINT AUTO_INCREMENT,
    stage VARCHAR(64),
    stage_id BIGINT,
    stage_extra1 INT,
    state VARCHAR(64),
    workdir VARCHAR(255),
    workdir_state VARCHAR(64),
    reduction VARCHAR(64),
    label VARCHAR(64),
    data_group VARCHAR(64),
    dvodb VARCHAR(255),
    note  VARCHAR(255),
    image_only TINYINT,
    minidvodb TINYINT, 
    minidvodb_group VARCHAR(64),
    minidvodb_name VARCHAR(64),
    PRIMARY KEY(add_id),
    KEY(add_id),
    KEY(stage_id),
    KEY(stage),
    KEY(state),
    KEY(workdir_state),
    KEY(label),
    KEY(data_group)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE addProcessedExp (
    add_id BIGINT AUTO_INCREMENT,
    dtime_addstar FLOAT,
    path_base VARCHAR(255),
    dvodb_path VARCHAR(255),
    fault SMALLINT NOT NULL,
    PRIMARY KEY(add_id),
    FOREIGN KEY(add_id) REFERENCES addRun(add_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE addMask (
    label VARCHAR(64),
    PRIMARY KEY(label)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE fakeRun (
    fake_id BIGINT AUTO_INCREMENT,
    cam_id BIGINT,
    state VARCHAR(64),
    workdir VARCHAR(255),
    label VARCHAR(64),
    data_group VARCHAR(64),
    dist_group VARCHAR(64),
    reduction VARCHAR(64),
    expgroup VARCHAR(64),
    dvodb VARCHAR(255),
    tess_id VARCHAR(64),
    end_stage VARCHAR(64),
    epoch TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    note VARCHAR(255),
    PRIMARY KEY(fake_id),
    KEY(cam_id),
    KEY(state),
    KEY(label),
    KEY(data_group),
    KEY(dist_group),
    KEY(expgroup),
    KEY(end_stage),
    INDEX(fake_id, cam_id),
    FOREIGN KEY(cam_id) REFERENCES camRun(cam_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE fakeProcessedImfile (
    fake_id BIGINT AUTO_INCREMENT,
    exp_id BIGINT,
    class_id VARCHAR(64),
    uri VARCHAR(255),
    dtime_fake FLOAT,
    dtime_script FLOAT,
    hostname VARCHAR(64),
    path_base VARCHAR(255),
    data_state VARCHAR(64),
    fault SMALLINT NOT NULL,
    epoch TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY(fake_id, exp_id, class_id),
    KEY(fault),
    FOREIGN KEY(fake_id) REFERENCES fakeRun(fake_id),
    FOREIGN KEY(exp_id, class_id) REFERENCES rawImfile(exp_id, class_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE fakeMask (
    label VARCHAR(64),
    PRIMARY KEY(label)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE detRun (
    det_id BIGINT AUTO_INCREMENT,
    iteration INT,
    det_type VARCHAR(64),
    mode VARCHAR(64),
    state VARCHAR(64),
    filelevel VARCHAR(64),
    workdir VARCHAR(255),
    camera VARCHAR(64),
    telescope VARCHAR(64),
    exp_type VARCHAR(64),
    reduction VARCHAR(64),
    filter VARCHAR(64),
    airmass_min FLOAT,
    airmass_max FLOAT,
    exp_time_min FLOAT,
    exp_time_max FLOAT,
    ccd_temp_min FLOAT,
    ccd_temp_max FLOAT,
    posang_min DOUBLE,
    posang_max DOUBLE,
    registered DATETIME,
    time_begin DATETIME,
    time_end DATETIME,
    use_begin DATETIME,
    use_end DATETIME,
    solang_min FLOAT,
    solang_max FLOAT,
    label VARCHAR(64),
    ref_det_id BIGINT,
    ref_iter INT,
    PRIMARY KEY(det_id),
    KEY(det_id),
    KEY(iteration),
    KEY(det_type),
    KEY(mode),
    KEY(state),
    KEY(label),
    INDEX(det_id, iteration)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE detInputExp (
    det_id BIGINT,
    iteration INT,
    exp_id BIGINT,
    include TINYINT,
    PRIMARY KEY(det_id, iteration, exp_id),
    INDEX(det_id, exp_id),
    INDEX(det_id, iteration),
    FOREIGN KEY(det_id) REFERENCES detRun(det_id),
    FOREIGN KEY(exp_id) REFERENCES rawExp(exp_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE detProcessedImfile (
    det_id BIGINT,
    exp_id BIGINT,
    class_id VARCHAR(64),
    uri VARCHAR(255),
    recipe VARCHAR(64),
    bg DOUBLE,
    bg_stdev DOUBLE,
    bg_mean_stdev DOUBLE,
    fringe_0 DOUBLE,
    fringe_1 DOUBLE,
    fringe_2 DOUBLE,
    user_1 DOUBLE,
    user_2 DOUBLE,
    user_3 DOUBLE,
    user_4 DOUBLE,
    user_5 DOUBLE,
    path_base VARCHAR(255),
    data_state VARCHAR(64),
    fault SMALLINT NOT NULL,
    PRIMARY KEY(det_id, exp_id, class_id),
    KEY(fault),
    INDEX(det_id, class_id),
    INDEX(det_id, exp_id),
    FOREIGN KEY(det_id, exp_id) REFERENCES detInputExp(det_id, exp_id),
    FOREIGN KEY(exp_id, class_id) REFERENCES rawImfile(exp_id, class_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE detProcessedExp (
    det_id BIGINT,
    exp_id BIGINT,
    recipe VARCHAR(64),
    bg DOUBLE,
    bg_stdev DOUBLE,
    bg_mean_stdev DOUBLE,
    fringe_0 DOUBLE,
    fringe_1 DOUBLE,
    fringe_2 DOUBLE,
    user_1 DOUBLE,
    user_2 DOUBLE,
    user_3 DOUBLE,
    user_4 DOUBLE,
    user_5 DOUBLE,
    path_base VARCHAR(255),
    data_state VARCHAR(64),
    fault SMALLINT NOT NULL,
    PRIMARY KEY(det_id, exp_id),
    KEY(fault),
    FOREIGN KEY(det_id, exp_id) REFERENCES detInputExp(det_id, exp_id),
    FOREIGN KEY(det_id, exp_id) REFERENCES detProcessedImfile(det_id, exp_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE detStackedImfile (
    det_id BIGINT,
    iteration INT,
    class_id VARCHAR(64),
    uri VARCHAR(255),
    recipe VARCHAR(64),
    bg DOUBLE,
    bg_stdev DOUBLE,
    bg_mean_stdev DOUBLE,
    user_1 DOUBLE,
    user_2 DOUBLE,
    user_3 DOUBLE,
    user_4 DOUBLE,
    user_5 DOUBLE,
    data_state VARCHAR(64),
    fault SMALLINT NOT NULL,
    PRIMARY KEY(det_id, iteration, class_id),
    KEY(fault),
    FOREIGN KEY(det_id, iteration) REFERENCES detInputExp(det_id, iteration),
    FOREIGN KEY(det_id, class_id) REFERENCES detProcessedImfile(det_id, class_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE detNormalizedStatImfile (
    det_id BIGINT,
    iteration INT,
    class_id VARCHAR(64),
    norm FLOAT,
    data_state VARCHAR(64),
    fault SMALLINT NOT NULL,
    PRIMARY KEY(det_id, iteration, class_id),
    KEY(fault),
    FOREIGN KEY(det_id, iteration) REFERENCES detInputExp(det_id, iteration),
    FOREIGN KEY(det_id, iteration, class_id) REFERENCES  detStackedImfile(det_id, iteration, class_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE detNormalizedImfile (
    det_id BIGINT,
    iteration INT,
    class_id VARCHAR(64),
    uri VARCHAR(255),
    bg DOUBLE,
    bg_stdev DOUBLE,
    bg_mean_stdev DOUBLE,
    user_1 DOUBLE,
    user_2 DOUBLE,
    user_3 DOUBLE,
    user_4 DOUBLE,
    user_5 DOUBLE,
    path_base VARCHAR(255),
    data_state VARCHAR(64),
    fault SMALLINT NOT NULL,
    PRIMARY KEY(det_id, iteration, class_id),
    KEY(fault),
    INDEX(det_id, iteration),
    FOREIGN KEY(det_id) REFERENCES detInputExp(det_id),
    FOREIGN KEY(det_id, iteration, class_id) REFERENCES detNormalizedStatImfile(det_id, iteration, class_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE detNormalizedExp (
    det_id BIGINT,
    iteration INT,
    recipe VARCHAR(64),
    bg DOUBLE,
    bg_stdev DOUBLE,
    bg_mean_stdev DOUBLE,
    user_1 DOUBLE,
    user_2 DOUBLE,
    user_3 DOUBLE,
    user_4 DOUBLE,
    user_5 DOUBLE,
    path_base VARCHAR(255),
    data_state VARCHAR(64),
    fault SMALLINT NOT NULL,
    PRIMARY KEY(det_id, iteration),
    KEY(fault),
    FOREIGN KEY(det_id, iteration) REFERENCES detInputExp(det_id, iteration),
    FOREIGN KEY(det_id, iteration) REFERENCES detNormalizedImfile(det_id, iteration)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE detResidImfile (
    det_id BIGINT,
    iteration INT,
    ref_det_id BIGINT,
    ref_iter INT,
    exp_id BIGINT,
    class_id VARCHAR(64),
    uri VARCHAR(255),
    recipe VARCHAR(64),
    bg DOUBLE,
    bg_stdev DOUBLE,
    bg_mean_stdev DOUBLE,
    bg_skewness DOUBLE,
    bg_kurtosis DOUBLE,
    bin_stdev DOUBLE,
    fringe_0 DOUBLE,
    fringe_1 DOUBLE,
    fringe_2 DOUBLE,
    fringe_resid_0 DOUBLE,
    fringe_resid_1 DOUBLE,
    fringe_resid_2 DOUBLE,
    user_1 DOUBLE,
    user_2 DOUBLE,
    user_3 DOUBLE,
    user_4 DOUBLE,
    user_5 DOUBLE,
    path_base VARCHAR(255),
    data_state VARCHAR(64),
    fault SMALLINT NOT NULL,
    PRIMARY KEY(det_id, iteration, exp_id, class_id),
    KEY(fault),
    INDEX(det_id, iteration, exp_id),
    FOREIGN KEY (det_id, iteration, exp_id) REFERENCES detInputExp(det_id, iteration, exp_id),
    FOREIGN KEY (det_id, exp_id, class_id) REFERENCES detProcessedImfile(det_id, exp_id, class_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE detResidExp (
       det_id BIGINT,
       iteration INT,
       exp_id BIGINT,
       recipe VARCHAR(64),
       bg DOUBLE,
       bg_stdev DOUBLE,
       bg_mean_stdev DOUBLE,
       bg_skewness DOUBLE,
       bg_kurtosis DOUBLE,
       bin_stdev DOUBLE,
       fringe_0 DOUBLE,
       fringe_1 DOUBLE,
       fringe_2 DOUBLE,
       fringe_resid_0 DOUBLE,
       fringe_resid_1 DOUBLE,
       fringe_resid_2 DOUBLE,
       user_1 DOUBLE,
       user_2 DOUBLE,
       user_3 DOUBLE,
       user_4 DOUBLE,
       user_5 DOUBLE,
       path_base VARCHAR(255),
       data_state VARCHAR(64),
       accept TINYINT,
       fault SMALLINT NOT NULL,
       PRIMARY KEY(det_id, iteration, exp_id),
       KEY(fault),
       INDEX(det_id, iteration),
       FOREIGN KEY(det_id, iteration, exp_id) REFERENCES detInputExp(det_id, iteration, exp_id),
       FOREIGN KEY(det_id, iteration, exp_id) REFERENCES detResidImfile(det_id, iteration, exp_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE detRunSummary (
       det_id BIGINT,
       iteration INT,
       data_state VARCHAR(64),
       bg DOUBLE,
       bg_stdev DOUBLE,
       bg_mean_stdev DOUBLE,
       accept TINYINT,
       fault SMALLINT NOT NULL,
       PRIMARY KEY(det_id, iteration),
       KEY(fault),
       FOREIGN KEY(det_id, iteration) REFERENCES detInputExp(det_id, iteration),
       FOREIGN KEY(det_id, iteration) REFERENCES detResidExp(det_id, iteration)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE detRegisteredImfile (
       det_id BIGINT,
       iteration INT,
       class_id VARCHAR(64),
       uri VARCHAR(255),
       bg DOUBLE,
       bg_stdev DOUBLE,
       bg_mean_stdev DOUBLE,
       user_1 DOUBLE,
       user_2 DOUBLE,
       user_3 DOUBLE,
       user_4 DOUBLE,
       user_5 DOUBLE,
       path_base VARCHAR(255),
       data_state VARCHAR(64),
       fault SMALLINT NOT NULL,
       PRIMARY KEY(det_id, iteration, class_id),
       KEY(fault),
       FOREIGN KEY(det_id, iteration) REFERENCES detRun(det_id, iteration)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE warpRun (
    warp_id BIGINT AUTO_INCREMENT,
    fake_id BIGINT,
    mode VARCHAR(64),
    state VARCHAR(64),
    workdir VARCHAR(255),
    workdir_state VARCHAR(64),
    label VARCHAR(64),
    data_group VARCHAR(64),
    dist_group VARCHAR(64),
    dvodb VARCHAR(255),
    tess_id VARCHAR(64),
    reduction VARCHAR(64),
    end_stage VARCHAR(64),
    registered DATETIME,
    magicked BIGINT,
    software_ver VARCHAR(16),
    maskfrac_npix FLOAT,
    maskfrac_static FLOAT,
    maskfrac_dynamic FLOAT,
    maskfrac_magic FLOAT,
    maskfrac_advisory FLOAT,
    note VARCHAR(255),
    PRIMARY KEY(warp_id),
    KEY(warp_id),
    KEY(fake_id),
    KEY(mode),
    KEY(state),
    KEY(workdir_state),
    KEY(label),
    KEY(data_group),
    KEY(dist_group),
    KEY(end_stage),
    INDEX(warp_id, fake_id),
    FOREIGN KEY(fake_id) REFERENCES fakeRun(fake_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE warpSkyCellMap (
    warp_id BIGINT,
    skycell_id VARCHAR(64),
    tess_id VARCHAR(64),
    class_id VARCHAR(64),
    fault SMALLINT,
    PRIMARY KEY(warp_id, skycell_id, tess_id, class_id),
    KEY(fault),
    FOREIGN KEY(warp_id) REFERENCES warpRun(warp_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE warpImfile (
    warp_id BIGINT,
    skycell_id VARCHAR(64),
    warp_skyfile_id BIGINT AUTO_INCREMENT,
    PRIMARY KEY(warp_id, skycell_id),
    KEY(warp_skyfile_id),
    FOREIGN KEY(warp_id) REFERENCES warpRun(warp_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;


CREATE TABLE warpSkyfile (
    warp_id BIGINT,
    skycell_id VARCHAR(64),
    tess_id VARCHAR(64),
    uri VARCHAR(255),
    path_base VARCHAR(255),
    data_state VARCHAR(64),
    bg DOUBLE,
    bg_stdev DOUBLE,
    dtime_warp FLOAT,
    dtime_script FLOAT,
    hostname VARCHAR(64),
    good_frac FLOAT,
    xmin INT,
    xmax INT,
    ymin INT,
    ymax INT,
    quality SMALLINT NOT NULL DEFAULT 0,
    fault SMALLINT,
    magicked BIGINT,
    software_ver VARCHAR(16),
    maskfrac_npix INT,
    maskfrac_static FLOAT,
    maskfrac_dynamic FLOAT,
    maskfrac_magic FLOAT,
    maskfrac_advisory FLOAT,
    background_model SMALLINT,
    PRIMARY KEY(warp_id, skycell_id, tess_id),
    KEY(good_frac),
    KEY(fault),
    KEY(quality),
    KEY(data_state),
    FOREIGN KEY(warp_id, skycell_id, tess_id) REFERENCES warpSkyCellMap(warp_id, skycell_id, tess_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE warpMask (
    label VARCHAR(64),
    PRIMARY KEY(label)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE warpSummary (
       warp_id BIGINT,
       projection_cell VARCHAR(64) NOT NULL,
       path_base     VARCHAR(255) NOT NULL,
       PRIMARY KEY(warp_id, projection_cell),
       FOREIGN KEY(warp_id) REFERENCES warpRun(warp_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE stackRun (
        stack_id BIGINT AUTO_INCREMENT,
        state VARCHAR(64),
        workdir VARCHAR(255),
        label VARCHAR(64),
        data_group VARCHAR(64),
        dist_group VARCHAR(64),
        reduction VARCHAR(64),
        dvodb VARCHAR(255),
        registered DATETIME,
        skycell_id VARCHAR(64),
        tess_id VARCHAR(64),
        filter VARCHAR(64),
        software_ver VARCHAR(16),
        note VARCHAR(255),
        PRIMARY KEY(stack_id),
        KEY(stack_id),
        KEY(state),
        KEY(skycell_id),
        KEY(tess_id),
        KEY(label),
        KEY(data_group),
        KEY(dist_group)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE stackInputSkyfile (
        stack_id BIGINT,
        warp_id BIGINT,
        PRIMARY KEY(stack_id, warp_id),
        FOREIGN KEY(stack_id) REFERENCES stackRun(stack_id),
        FOREIGN KEY(warp_id) REFERENCES warpSkyfile(warp_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE stackSumSkyfile (
        stack_id BIGINT,
        uri VARCHAR(255),
        path_base VARCHAR(255),
        bg DOUBLE,
        bg_stdev DOUBLE,
        dtime_stack FLOAT,
        dtime_match_mean FLOAT,
        dtime_match_stdev FLOAT,
	dtime_convolve FLOAT,
        dtime_initial FLOAT,
        dtime_reject FLOAT,
        dtime_final FLOAT,
        dtime_phot FLOAT,
        dtime_script FLOAT,
        match_mean FLOAT,
        match_stdev FLOAT,
        match_rms FLOAT,
        stamps_mean FLOAT,
        stamps_stdev FLOAT,
        stamps_min INT,
        reject_images INT,
        reject_pix_mean FLOAT,
        reject_pix_stdev FLOAT,
        sources INT,
        hostname VARCHAR(64),
        good_frac FLOAT,
        mjd_obs DOUBLE,
        fault SMALLINT,
        software_ver VARCHAR(16),
	background_model SMALLINT,
        quality SMALLINT NOT NULL DEFAULT 0,
        PRIMARY KEY(stack_id),
        KEY(dtime_stack),
        KEY(good_frac),
        KEY(fault),
        KEY(quality),
        FOREIGN KEY(stack_id) REFERENCES stackRun(stack_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE stackAssociation (
    sass_id      BIGINT AUTO_INCREMENT,
    data_group   VARCHAR(64) NOT NULL,
    projection_cell VARCHAR(64) NOT NULL,
    tess_id       VARCHAR(64) NOT NULL,
    filter        VARCHAR(64) NOT NULL,
    PRIMARY KEY(sass_id),
    KEY(data_group),
    KEY(projection_cell),
    KEY(tess_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE stackAssociationMap (
    sass_id      BIGINT,
    stack_id     BIGINT,
    PRIMARY KEY(sass_id, stack_id),
    FOREIGN KEY(sass_id) REFERENCES stackAssociation(sass_id),
    FOREIGN KEY(stack_id) REFERENCES stackRun(stack_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE stackSummary (
    sass_id     BIGINT,
    projection_cell VARCHAR(64) NOT NULL,
    path_base   VARCHAR(255) NOT NULL,
    PRIMARY KEY(sass_id, projection_cell),
    FOREIGN KEY(sass_id) REFERENCES stackAssociation(sass_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE diffRun (
        diff_id BIGINT AUTO_INCREMENT,
        state VARCHAR(64),
        workdir VARCHAR(255),
        label VARCHAR(64),
        data_group VARCHAR(64),
        dist_group VARCHAR(64),
        reduction VARCHAR(64),
        dvodb VARCHAR(255),
        registered DATETIME,
        tess_id VARCHAR(64),
        bothways TINYINT DEFAULT 0,
        exposure TINYINT DEFAULT 0,
        magicked BIGINT,
        software_ver VARCHAR(16),
        maskfrac_npix FLOAT,
        maskfrac_static FLOAT,
        maskfrac_dynamic FLOAT,
        maskfrac_magic FLOAT,
        maskfrac_advisory FLOAT,
        diff_mode SMALLINT NOT NULL,
        note VARCHAR(255),
        PRIMARY KEY(diff_id),
        KEY(diff_id),
        KEY(state),
        KEY(tess_id),
        KEY(label),
        KEY(data_group),
        KEY(dist_group)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE diffInputSkyfile (
        diff_id BIGINT,
        skycell_id VARCHAR(64),
        warp1 BIGINT,
        stack1 BIGINT,
        warp2 BIGINT,
        stack2 BIGINT,
        tess_id VARCHAR(64),
        diff_skyfile_id BIGINT AUTO_INCREMENT,
        PRIMARY KEY(diff_skyfile_id),
        KEY(diff_id, skycell_id),
        KEY(warp1),
        KEY(warp2),
        KEY(stack1),
        KEY(stack2),
        KEY(skycell_id),
        KEY(tess_id),
        FOREIGN KEY(diff_id) REFERENCES diffRun(diff_id),
        FOREIGN KEY(warp1, skycell_id, tess_id) REFERENCES warpSkyfile(warp_id, skycell_id, tess_id),
        FOREIGN KEY(warp2, skycell_id, tess_id) REFERENCES warpSkyfile(warp_id, skycell_id, tess_id),
        FOREIGN KEY(stack1) REFERENCES stackSumSkyfile(stack_id),
        FOREIGN KEY(stack2) REFERENCES stackSumSkyfile(stack_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE diffSkyfile (
        diff_id BIGINT,
        skycell_id VARCHAR(64),
        path_base VARCHAR(255),
        data_state VARCHAR(64),
        bg DOUBLE,
        bg_stdev DOUBLE,
        stamps_num INT,
        stamps_mean FLOAT,
        stamps_rms FLOAT,
        norm FLOAT,
        bg_diff FLOAT,
        kernel_x FLOAT,
        kernel_y FLOAT,
        kernel_xx FLOAT,
        kernel_xy FLOAT,
        kernel_yy FLOAT,
        deconv_max FLOAT,
        sources INT,
        dtime_diff FLOAT,
        dtime_match FLOAT,
        dtime_phot FLOAT,
        dtime_script FLOAT,
        hostname VARCHAR(64),
        good_frac FLOAT,
        quality SMALLINT NOT NULL DEFAULT 0,
        fault SMALLINT,
        magicked BIGINT,
        software_ver VARCHAR(16),
        maskfrac_npix INT,
        maskfrac_static FLOAT,
        maskfrac_dynamic FLOAT,
        maskfrac_magic FLOAT,
        maskfrac_advisory FLOAT,
        PRIMARY KEY(diff_id, skycell_id),
        KEY(good_frac),
        KEY(fault),
        KEY(quality),
        KEY(data_state),
        FOREIGN KEY(diff_id) REFERENCES diffRun(diff_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE diffSummary (
    diff_id     BIGINT,
    projection_cell VARCHAR(64) NOT NULL,
    path_base   VARCHAR(255) NOT NULL,
    PRIMARY KEY(diff_id, projection_cell),
    FOREIGN KEY(diff_id) REFERENCES diffRun(diff_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE magicRun (
        magic_id BIGINT AUTO_INCREMENT,
        exp_id BIGINT,
        diff_id BIGINT,
        inverse TINYINT NOT NULL DEFAULT 0,
        state VARCHAR(64),
        workdir VARCHAR(255),
        workdir_state VARCHAR(255),
        label VARCHAR(64),
        data_group VARCHAR(64),
        dvodb VARCHAR(255),
        registered DATETIME,
        fault SMALLINT,
        note VARCHAR(255),
        PRIMARY KEY(magic_id),
        KEY(magic_id),
        KEY(state),
        KEY(workdir_state),
        KEY(label),
        KEY(data_group),
        KEY(fault),
        FOREIGN KEY(exp_id)  REFERENCES rawExp(exp_id),
        FOREIGN KEY(diff_id) REFERENCES diffRun(diff_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE magicInputSkyfile (
        magic_id BIGINT,
        node VARCHAR(64),
        PRIMARY KEY(magic_id, node),
        FOREIGN KEY(magic_id) REFERENCES magicRun(magic_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE magicTree (
        magic_id BIGINT,
        node VARCHAR(64),
        dep VARCHAR(64),
        KEY(magic_id),
        KEY(node),
        KEY(dep),
        INDEX(magic_id, node),
        FOREIGN KEY(magic_id) REFERENCES magicRun(magic_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE magicNodeResult (
        magic_id BIGINT,
        node VARCHAR(64),
        path_base VARCHAR(255),
        fault SMALLINT,
        PRIMARY KEY(magic_id, node),
        FOREIGN KEY(magic_id) REFERENCES magicRun(magic_id),
        FOREIGN KEY(magic_id, node) REFERENCES magicTree(magic_id, node),
        KEY(fault)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE magicMask (
        magic_id BIGINT,
        uri VARCHAR(255),
        path_base VARCHAR(255),
        streaks INT,
        fault SMALLINT,
        PRIMARY KEY(magic_id),
        FOREIGN KEY(magic_id) REFERENCES magicRun(magic_id),
        KEY(fault)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE magicDSRun (
        magic_ds_id BIGINT AUTO_INCREMENT,
        magic_id BIGINT,
        inv_magic_id BIGINT,
        state VARCHAR(64),
        stage VARCHAR(64),
        stage_id BIGINT,
        cam_id BIGINT,
        label VARCHAR(64),
        data_group VARCHAR(64),
        outroot VARCHAR(255),
        recoveryroot VARCHAR(255),
        re_place TINYINT,
        remove TINYINT,
        fault SMALLINT,
        note VARCHAR(255),
        PRIMARY KEY(magic_ds_id),
        KEY(magic_ds_id),
        KEY(state),
        KEY(magic_id),
        KEY(label),
        KEY(fault),
        KEY(stage),
        KEY(stage_id),
        FOREIGN KEY(magic_id) REFERENCES magicRun(magic_id),
        FOREIGN KEY(inv_magic_id) REFERENCES magicRun(magic_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE magicDSFile (
    magic_ds_id BIGINT,
    component VARCHAR(64),
    backup_path_base VARCHAR(255),
    recovery_path_base VARCHAR(255),
    streak_frac FLOAT,
    nondiff_frac FLOAT,
    run_time FLOAT,
    fault SMALLINT,
    data_state VARCHAR(64),
    PRIMARY KEY(magic_ds_id, component),
    KEY(fault),
    KEY(data_state),
    FOREIGN KEY(magic_ds_id) REFERENCES magicDSRun(magic_ds_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE calDB (
        cal_id BIGINT AUTO_INCREMENT,
        dvodb VARCHAR(64),
        state VARCHAR(64),
        PRIMARY KEY(cal_id),
        KEY(cal_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE calRun (
        cal_id BIGINT AUTO_INCREMENT,
        region VARCHAR(64),
        last_step VARCHAR(64),
        state VARCHAR(64),
        PRIMARY KEY(cal_id),
        KEY(cal_id),
        KEY(last_step),
        FOREIGN KEY(cal_id) REFERENCES calDB(cal_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE flatcorrRun (
        corr_id BIGINT AUTO_INCREMENT,
        det_type VARCHAR(64),
        dvodb VARCHAR(64),
        camera VARCHAR(64),
        telescope VARCHAR(64),
        epoch TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        filter VARCHAR(64),
        state VARCHAR(64),
        make_corr TINYINT,
        workdir VARCHAR(255),
        label VARCHAR(64),
        reduction VARCHAR(64),
        region VARCHAR(64),
        hostname VARCHAR(64),
        fault SMALLINT NOT NULL,
        PRIMARY KEY(corr_id),
        KEY(corr_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

-- these three tables link the flatcorrRun to the associated chip, camera, and addstar runs
CREATE TABLE flatcorrChipLink (
        corr_id BIGINT,
        chip_id BIGINT,
        include TINYINT,
        PRIMARY KEY(corr_id, chip_id),
        FOREIGN KEY(corr_id) REFERENCES flatcorrRun(corr_id),
        FOREIGN KEY(chip_id) REFERENCES chipRun(chip_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE flatcorrCamLink (
        corr_id BIGINT,
        chip_id BIGINT,
        cam_id BIGINT,
        include TINYINT,
        PRIMARY KEY(corr_id, cam_id),
        FOREIGN KEY(corr_id) REFERENCES flatcorrRun(corr_id),
        FOREIGN KEY(chip_id) REFERENCES chipRun(chip_id),
        FOREIGN KEY(cam_id) REFERENCES camRun(cam_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE flatcorrAddstarLink (
        corr_id BIGINT,
        cam_id BIGINT,
        add_id BIGINT,
        include TINYINT,
        PRIMARY KEY(corr_id, cam_id, add_id),
        FOREIGN KEY (corr_id)  REFERENCES  flatcorrRun(corr_id),
        FOREIGN KEY (cam_id)  REFERENCES  camRun(cam_id),
        FOREIGN KEY (add_id)  REFERENCES  addRun(add_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE pstampDataStore (
        ds_id BIGINT AUTO_INCREMENT,
        state VARCHAR(64),
        lastFileset VARCHAR(64),
        timestamp  DATETIME,
        label VARCHAR(64),
        outProduct VARCHAR(64) UNIQUE,
        uri VARCHAR(255),
        pollInterval INTEGER DEFAULT 60,
        need_magic TINYINT,
        PRIMARY KEY(ds_id),
        KEY(ds_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE pstampProject (
        proj_id BIGINT AUTO_INCREMENT,
        name VARCHAR(64) UNIQUE,
        state VARCHAR(64),
        dbname VARCHAR(64),
        dvodb VARCHAR(64),
        camera VARCHAR(64),
        telescope VARCHAR(64),
        need_magic TINYINT,
        PRIMARY KEY(proj_id),
        KEY(proj_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE pstampRequest (
        req_id BIGINT AUTO_INCREMENT,
        ds_id BIGINT,
        state VARCHAR(64),
        name VARCHAR(64) UNIQUE,
        reqType VARCHAR(16),
        label VARCHAR(64),
        outProduct VARCHAR(64),
        uri VARCHAR(255),
        outdir VARCHAR(255),
        username VARCHAR(255),
        proj_id BIGINT,
        registered datetime,
        timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        fault SMALLINT,
        PRIMARY KEY(req_id),
        KEY(req_id),
        KEY(state),
        KEY(fault),
        KEY(label),
        KEY(username)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE pstampJob (
        job_id BIGINT AUTO_INCREMENT,
        req_id BIGINT,
        rownum VARCHAR(64),
        state VARCHAR(64),
        jobType VARCHAR(16),
        fault SMALLINT,
        exp_id BIGINT,
        outputBase VARCHAR(255),
        options BIGINT,
        dep_id BIGINT,
        fault_count INT,
        parent_id BIGINT DEFAULT NULL,
        is_parent TINYINT DEFAULT 0,
        PRIMARY KEY(job_id, req_id),
        KEY(job_id),
        KEY(req_id),
        KEY(state),
        KEY(fault),
        KEY(dep_id),
        KEY(parent_id),
        KEY(is_parent),
        FOREIGN KEY(req_id) REFERENCES pstampRequest(req_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE pstampDependent (
        dep_id BIGINT AUTO_INCREMENT,
        state      VARCHAR(64),
        stage      VARCHAR(64),
        stage_id   BIGINT,
        component  VARCHAR(64),
        imagedb    VARCHAR(64),
        outdir     VARCHAR(255),
        rlabel     VARCHAR(64),
        need_magic TINYINT,
        fault      SMALLINT,
        fault_count INT,
        PRIMARY KEY(dep_id),
        KEY(state),
        KEY(stage),
        KEY(stage_id),
        KEY(component),
        KEY(imagedb),
        KEY(fault)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE pstampFile (
    file_id BIGINT AUTO_INCREMENT,
    job_id BIGINT NOT NULL,
    path VARCHAR(255),
    PRIMARY KEY(file_id),
    KEY(job_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE pstampUserDomain (
    domainName      VARCHAR(64),
    accessLevel     INT,
    defaultProduct  VARCHAR(255),
    defaultLabel    VARCHAR(64),
    PRIMARY KEY(domainName)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE pstampUser (
    userName        VARCHAR(64),
    domainName      VARCHAR(64),
    accessLevel     INT,
    defaultProduct  VARCHAR(255),
    defaultLabel    VARCHAR(64),
    PRIMARY KEY (userName, domainName)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE pstampAccessLevel (
    proj_id         BIGINT,
    accessLevel     INT,
    mjd_min         FLOAT DEFAULT 0,
    mjd_max         FLOAT DEFAULT 0,
    PRIMARY KEY (proj_id, accessLevel),
    UNIQUE KEY (proj_id, accessLevel),
    FOREIGN KEY(proj_id) REFERENCES pstampProject(proj_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE pstampWebRequest (
        num BIGINT AUTO_INCREMENT,
        PRIMARY KEY(num)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE distTarget (
    target_id   BIGINT AUTO_INCREMENT,
    dist_group  VARCHAR(64),
    filter      VARCHAR(64),
    stage       VARCHAR(64),
    clean       TINYINT,
    state       VARCHAR(64),
    comment     VARCHAR(255),
    PRIMARY KEY(target_id),
    KEY(stage),
    KEY(dist_group),
    KEY(filter),
    KEY(state),
    CONSTRAINT UNIQUE (dist_group, filter, stage, clean)
)  ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE distRun (
    dist_id     BIGINT AUTO_INCREMENT,
    target_id   BIGINT,
    stage       VARCHAR(64),
    stage_id    BIGINT,
    magic_ds_id BIGINT,
    label       VARCHAR(64),
    outroot     VARCHAR(255),
    outdir      VARCHAR(255),
    clean       TINYINT,
    no_magic    TINYINT,
    alternate   TINYINT,
    state       VARCHAR(64),
    time_stamp  TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    fault       SMALLINT,
    data_group  VARCHAR(64),
    note        VARCHAR(255),
    PRIMARY KEY(dist_id),
    KEY(state),
    KEY(label),
    KEY(stage),
    KEY(stage_id),
    KEY(magic_ds_id),
    KEY(fault),
    KEY(data_group),
    FOREIGN KEY(target_id) REFERENCES distTarget(target_id)
)  ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE distComponent (
    dist_id     BIGINT,
    component   VARCHAR(64),
    bytes       INT,
    md5sum      VARCHAR(32),
    state       VARCHAR(64),
    outdir      VARCHAR(255),
    name        VARCHAR(255),
    fault       SMALLINT,
    PRIMARY KEY(dist_id, component),
    KEY(state),
    FOREIGN KEY(dist_id) REFERENCES distRun(dist_id)
)  ENGINE=innodb DEFAULT CHARSET=latin1;


CREATE TABLE rcDestination (
    dest_id     BIGINT AUTO_INCREMENT,
    name        VARCHAR(64),
    status_uri  VARCHAR(255),
    comment     VARCHAR(255),
    last_fileset VARCHAR(255),
    dbname      VARCHAR(64),
    dbhost      VARCHAR(64),
    state       VARCHAR(64),
    PRIMARY KEY(dest_id)
)  ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE rcInterest (
    int_id      BIGINT AUTO_INCREMENT,
    dest_id     BIGINT,
    target_id   BIGINT,
    state       VARCHAR(64),
    PRIMARY KEY(int_id),
    FOREIGN KEY(dest_id) REFERENCES rcDestination(dest_id),
    FOREIGN KEY(target_id) REFERENCES distTarget(target_id),
    CONSTRAINT UNIQUE (dest_id, target_id)
)  ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE rcDSFileset (
    fs_id       BIGINT AUTO_INCREMENT,
    dist_id     BIGINT,
    dest_id     BIGINT,
    name        VARCHAR(255),
    state       VARCHAR(64),
    registered  TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    fault       SMALLINT DEFAULT 0,
    PRIMARY KEY(dist_id, dest_id),
    KEY(fs_id),
    FOREIGN KEY(dist_id) REFERENCES distRun(dist_id),
    FOREIGN KEY(dest_id) REFERENCES rcDestination(dest_id)
)  ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE rcRun (
    rc_id       BIGINT AUTO_INCREMENT,
    fs_id       BIGINT,
    dest_id     BIGINT,
    state       VARCHAR(64),
    status_fs_name   VARCHAR(64),
    registered  TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    fault       SMALLINT DEFAULT 0,
    PRIMARY KEY(rc_id),
    FOREIGN KEY(fs_id) REFERENCES rcDSFileset(fs_id),
    FOREIGN KEY(dest_id) REFERENCES rcDestination(dest_id)
)  ENGINE=innodb DEFAULT CHARSET=latin1;

-- Sources from which to receive files
CREATE TABLE receiveSource (
    source_id BIGINT AUTO_INCREMENT, -- unique identifier
    source VARCHAR(128) NOT NULL, -- source URI
    product VARCHAR(64) NOT NULL, -- product of interest
    workdir VARCHAR(255) NOT NULL, -- where to extract
    state VARCHAR(64) NOT NULL, -- state 'enabled' or 'disabled'
    comment VARCHAR(255),       -- for human memory
    fileset_last VARCHAR(128),  -- last fileset seen
    status_product VARCHAR(64), -- status data store product
    ds_dbname VARCHAR(64),      -- status data store's database name
    ds_dbhost VARCHAR(64),      -- status data store's host name
    PRIMARY KEY(source_id),
    KEY(source),
    KEY(product),
    KEY(comment)
) ENGINE=innodb DEFAULT CHARSET=latin1;

-- Filesets to receive
CREATE TABLE receiveFileset (
    fileset_id BIGINT AUTO_INCREMENT, -- unique identifier
    source_id BIGINT NOT NULL,  -- link to receiveSource
    fileset VARCHAR(128) NOT NULL, -- fileset to receive
    state VARCHAR(64), -- new or full
    dirinfo VARCHAR(255), -- uri for directory info file for this run
    dbinfo VARCHAR(255), -- uri for database dump file for this run
    fault SMALLINT NOT NULL DEFAULT 0, -- Fault code
    PRIMARY KEY(fileset_id),
    KEY(source_id),
    CONSTRAINT UNIQUE(source_id, fileset),
    FOREIGN KEY(source_id) REFERENCES receiveSource(source_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

-- Files to receive
CREATE TABLE receiveFile (
    file_id BIGINT AUTO_INCREMENT, -- unique identifier
    fileset_id BIGINT NOT NULL,  -- link to receiveFileset
    file VARCHAR(128) NOT NULL, -- file to receive
    bytes BIGINT,
    md5sum VARCHAR(255),
    file_type VARCHAR(64),
    component VARCHAR(64),
    PRIMARY KEY(file_id),
    KEY(fileset_id),
    CONSTRAINT UNIQUE(fileset_id, file),
    FOREIGN KEY(fileset_id) REFERENCES receiveFileset(fileset_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

-- Result of receiving files
CREATE TABLE receiveResult (
    file_id BIGINT AUTO_INCREMENT, -- link to receiveFile
    dtime_copy FLOAT,           -- Time to copy
    dtime_extract FLOAT,        -- Time to extract
    fault SMALLINT NOT NULL DEFAULT 0, -- Fault code
    PRIMARY KEY(file_id),
    KEY(fault),
    FOREIGN KEY(file_id) REFERENCES receiveFile(file_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;



-- Tables to support publishing of detections to a Science Client

-- Clients to which we send stuff
CREATE TABLE publishClient (
    client_id BIGINT AUTO_INCREMENT, -- unique identifier
    active TINYINT DEFAULT 0,        -- whether we should worry about this or not
    product VARCHAR(64),             -- product name
    stage VARCHAR(64) NOT NULL, -- stage of interest (chip, camera, diff, etc.)
    magicked TINYINT DEFAULT 1, -- Require magicked data?
    workdir VARCHAR(255) NOT NULL, -- working directory
    comment VARCHAR(255),            -- for human memory
    name varchar(64) default NULL, -- unique client_id verbose identifier
    output_format SMALLINT NOT NULL default 1, -- format output versioning
    PRIMARY KEY(client_id),
    UNIQUE KEY name (name)
) ENGINE=innodb DEFAULT CHARSET=latin1;

-- Publishing a set of data (e.g., a specific diffRun)
CREATE TABLE publishRun (
    pub_id BIGINT AUTO_INCREMENT, -- unique identifier
    client_id BIGINT NOT NULL,  -- link to publishClient
    stage_id BIGINT NOT NULL,   -- link to various stage tables
    label VARCHAR(64),          -- label for run
    state VARCHAR(64),          -- state of run (new, full, etc.)
    PRIMARY KEY(pub_id),
    KEY(client_id),
    KEY(stage_id),
    KEY(label),
    KEY(state),
    FOREIGN KEY(client_id) REFERENCES publishClient(client_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

-- Publishing a file within a set
CREATE TABLE publishDone (
    pub_id BIGINT AUTO_INCREMENT, -- link to publishRun
    path_base VARCHAR(255),     -- base path of output
    hostname VARCHAR(64),       -- name of host
    dtime_script FLOAT,         -- run time for script
    fault SMALLINT NOT NULL DEFAULT 0, -- Fault code
    PRIMARY KEY(pub_id),
    KEY(fault),
    FOREIGN KEY(pub_id) REFERENCES publishRun(pub_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;


-- Tables for static sky analysis

-- A static sky analysis set
CREATE TABLE staticskyRun (
      sky_id BIGINT AUTO_INCREMENT, -- unique identifier
      state VARCHAR(64) NOT NULL,  -- state of run (new, full, etc.)
      workdir VARCHAR(255) NOT NULL, -- working directory
      label VARCHAR(64),             -- processing label
      data_group VARCHAR(64),        -- group for data
      dist_group VARCHAR(64),        -- group for distribution
      reduction VARCHAR(64),         -- reduction class (for altering recipe)
      note VARCHAR(255),             -- note
      registered TIMESTAMP DEFAULT CURRENT_TIMESTAMP, -- time run was registered
      PRIMARY KEY(sky_id),
      KEY(state),
      KEY(label),
      KEY(data_group),
      KEY(dist_group)
) ENGINE=innodb DEFAULT CHARSET=latin1;

-- Inputs for static sky analysis
CREATE TABLE staticskyInput (
      sky_id BIGINT,             -- static sky identifier
      stack_id BIGINT,          -- stack identifier
      PRIMARY KEY(sky_id,stack_id),
      KEY(stack_id),
      FOREIGN KEY(sky_id) REFERENCES staticskyRun(sky_id),
      FOREIGN KEY(stack_id) REFERENCES stackSumSkyfile(stack_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

-- Result of static sky analysis
CREATE TABLE staticskyResult (
      sky_id BIGINT,             -- static sky identifier
      path_base VARCHAR(255) NOT NULL, -- root name for outputs
      dtime_phot FLOAT,                -- elapsed time for photometry
      dtime_script FLOAT,              -- elapsed time for script
      sources INT,                     -- number of sources
      num_inputs INT,                  -- number of input images
      hostname VARCHAR(64) NOT NULL,   -- host that executed script
      good_frac FLOAT,                 -- good fraction of skycell
      quality SMALLINT NOT NULL,       -- bad quality flag
      fault SMALLINT NOT NULL,         -- fault code
      PRIMARY KEY(sky_id),
      KEY(good_frac),
      KEY(fault),
      KEY(quality),
      FOREIGN KEY(sky_id) REFERENCES staticskyRun(sky_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;


-- Processing labels with their priorities
CREATE TABLE Label (
    label       VARCHAR(64),
    priority    INT,
    active      TINYINT,
    comment     VARCHAR(80),
    PRIMARY KEY(label),
    KEY(priority),
    KEY(active)
) ENGINE=innodb DEFAULT CHARSET=latin1;

-- Tables to support background restoration

-- Background replacement on a chipRun
CREATE TABLE chipBackgroundRun (
    chip_bg_id BIGINT AUTO_INCREMENT, -- unique identifier
    chip_id BIGINT NOT NULL,          -- link to chipRun
    cam_id BIGINT DEFAULT 0,          -- optional link to camRun
    state VARCHAR(64) NOT NULL,       -- state of run (new, full, etc.)
    workdir VARCHAR(255) NOT NULL,    -- working directory
    label VARCHAR(64),                -- processing label
    data_group VARCHAR(64),           -- group for data
    dist_group VARCHAR(64),           -- group for distribution
    reduction VARCHAR(64),    -- reduction class (for altering recipe)
    note VARCHAR(255),        -- note
    registered TIMESTAMP DEFAULT CURRENT_TIMESTAMP, -- time run was registered
    magicked BIGINT DEFAULT 0 NOT NULL, -- magic status
    PRIMARY KEY(chip_bg_id),
    KEY(chip_id),
    KEY(state),
    KEY(label),
    KEY(data_group),
    KEY(dist_group),
    FOREIGN KEY(chip_id) REFERENCES chipRun(chip_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

-- Results of background replacement from chipBackgroundRun
CREATE TABLE chipBackgroundImfile (
    chip_bg_id BIGINT NOT NULL,        -- unique identifier
    class_id VARCHAR(64) NOT NULL,     -- class (component) identifier
    path_base VARCHAR(255) NOT NULL,   -- root name for outputs
    magicked BIGINT,                   -- magic_id if magicked
    dtime_script FLOAT,                -- elapsed time for script
    hostname VARCHAR(64) NOT NULL,     -- host that executed script
    quality SMALLINT NOT NULL,         -- bad quality flag
    fault SMALLINT NOT NULL,           -- fault code
    software_ver VARCHAR(16),          -- software version
    bg FLOAT,                          -- background level
    bg_stdev FLOAT,                    -- stdev of background
    maskfrac_npix FLOAT,               -- Number of pixels masked
    maskfrac_static FLOAT,             -- Fraction masked static
    maskfrac_dynamic FLOAT,            -- Fraction masked dynamic
    maskfrac_magic FLOAT,              -- Fraction masked magic
    maskfrac_advisory FLOAT,           -- Fraction masked advisory
    PRIMARY KEY(chip_bg_id,class_id),
    KEY(fault),
    KEY(quality),
    FOREIGN KEY(chip_bg_id) REFERENCES chipBackgroundRun(chip_bg_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

-- Background replacement on a warpRun (utilising chipBackgroundRun)
CREATE TABLE warpBackgroundRun (
    warp_bg_id BIGINT AUTO_INCREMENT, -- unique identifier
    warp_id BIGINT NOT NULL,          -- link to warpRun
    chip_bg_id BIGINT NOT NULL,       -- link to chipBackgroundRun
    state VARCHAR(64) NOT NULL,       -- state of run (new, full, etc.)
    workdir VARCHAR(255) NOT NULL,    -- working directory
    label VARCHAR(64),                -- processing label
    data_group VARCHAR(64),           -- group for data
    dist_group VARCHAR(64),           -- group for distribution
    reduction VARCHAR(64),            -- reduction class (for altering recipe)
    note VARCHAR(255),        -- note
    registered TIMESTAMP DEFAULT CURRENT_TIMESTAMP, -- time run was registered
    magicked BIGINT DEFAULT 0 NOT NULL, -- magic status
    PRIMARY KEY(warp_bg_id),
    KEY(warp_id),
    KEY(chip_bg_id),
    KEY(state),
    KEY(label),
    KEY(data_group),
    KEY(dist_group),
    FOREIGN KEY(warp_id) REFERENCES warpRun(warp_id),
    FOREIGN KEY(chip_bg_id) REFERENCES chipBackgroundRun(chip_bg_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

-- Results of background replacement from warpBackgroundRun
CREATE TABLE warpBackgroundSkyfile (
    warp_bg_id BIGINT NOT NULL,        -- unique identifier
    skycell_id VARCHAR(64) NOT NULL,   -- skycell identifier
    path_base VARCHAR(255) NOT NULL,   -- root name for outputs
    magicked BIGINT,                   -- magic_id if magicked
    dtime_script FLOAT,                -- elapsed time for script
    hostname VARCHAR(64) NOT NULL,     -- host that executed script
    quality SMALLINT NOT NULL,         -- bad quality flag
    fault SMALLINT NOT NULL,           -- fault code
    software_ver VARCHAR(16),          -- software version
    bg FLOAT,                          -- background level
    bg_stdev FLOAT,                    -- stdev of background
    maskfrac_npix FLOAT,               -- Number of pixels masked
    maskfrac_static FLOAT,             -- Fraction masked static
    maskfrac_dynamic FLOAT,            -- Fraction masked dynamic
    maskfrac_magic FLOAT,              -- Fraction masked magic
    maskfrac_advisory FLOAT,           -- Fraction masked advisory
    PRIMARY KEY(warp_bg_id,skycell_id),
    KEY(fault),
    KEY(quality),
    FOREIGN KEY(warp_bg_id) REFERENCES warpBackgroundRun(warp_bg_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;


-- Tables to support (re-)photometry of a diff

CREATE TABLE diffPhotRun (
    diff_phot_id BIGINT AUTO_INCREMENT, -- Identifier for diffPhotRun
    diff_id BIGINT NOT NULL,            -- Identifier for diffRun
    state VARCHAR(64) NOT NULL,         -- State of run
    workdir VARCHAR(255) NOT NULL, -- working directory
    label VARCHAR(64),             -- processing label
    data_group VARCHAR(64),        -- group for data
    reduction VARCHAR(64),         -- reduction class (for altering recipe)
    registered TIMESTAMP DEFAULT CURRENT_TIMESTAMP, -- time run was registered
    note VARCHAR(255),             -- note
    magicked BIGINT NOT NULL DEFAULT 0, -- magic mask applied
    PRIMARY KEY(diff_phot_id),
    KEY(diff_id),
    KEY(state),
    KEY(label),
    KEY(data_group),
    FOREIGN KEY(diff_id) REFERENCES diffRun(diff_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE diffPhotSkyfile (
    diff_phot_id BIGINT AUTO_INCREMENT, -- Identifier for diffPhotRun
    skycell_id VARCHAR(64) NOT NULL,            -- Skycell identifier
    path_base VARCHAR(255) NOT NULL, -- Base of path for output
    dtime_script FLOAT,              -- elapsed time for script
    hostname VARCHAR(64) NOT NULL,   -- host that executed script
    fault SMALLINT NOT NULL,         -- fault code
    quality SMALLINT NOT NULL,       -- bad quality flag
    software_ver VARCHAR(16),                       -- software version
    magicked BIGINT NOT NULL DEFAULT 0, -- magic mask applied
    PRIMARY KEY(diff_phot_id, skycell_id),
    KEY(fault),
    KEY(quality),
    FOREIGN KEY(diff_phot_id) REFERENCES diffPhotRun(diff_phot_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

-- Tables for large area processing

CREATE TABLE lapSequence (
    seq_id BIGINT AUTO_INCREMENT, -- Identifier for the processing sequence
    name VARCHAR(64) NOT NULL,    -- short name of the sequence
    description VARCHAR(255) NOT NULL, -- longer description of the sequence
    PRIMARY KEY(seq_id),
    KEY(name)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE lapRun (
    lap_id BIGINT AUTO_INCREMENT, -- Identifier for the processing run
    seq_id BIGINT NOT NULL,       -- Identifier to match to the sequence
    tess_id VARCHAR(64) NOT NULL, -- tessellation id to use
    projection_cell VARCHAR(64) NOT NULL, -- projection cell from the tessellation to consider
    filter VARCHAR(64) NOT NULL,  -- filter
    state VARCHAR(64) NOT NULL,   -- state of run
    label VARCHAR(64) NOT NULL,   -- processing label
    dist_group VARCHAR(64) NOT NULL, -- distribution group for products of this run
    registered TIMESTAMP DEFAULT CURRENT_TIMESTAMP, -- time run was registered
    fault SMALLINT NOT NULL,      -- fault code
    quick_sass_id BIGINT,         -- stackAssociation id for quick stack
    final_sass_id BIGINT,         -- stackAssociation id for final stack
    PRIMARY KEY(lap_id),
    KEY(seq_id),
    KEY(projection_cell),
    KEY(filter),
    KEY(state),
    KEY(label),
    KEY(fault),
    FOREIGN KEY(seq_id) REFERENCES lapSequence(seq_id),
    FOREIGN KEY(quick_sass_id) REFERENCES stackAssociation(sass_id),
    FOREIGN KEY(final_sass_id) REFERENCES stackAssociation(sass_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE lapExp (
    lap_id BIGINT NOT NULL, -- Link back to processing run
    exp_id BIGINT NOT NULL, -- exposure definition
    chip_id BIGINT,         -- processing id from chipRun
    pair_id BIGINT,         -- companion chip_id
    private TINYINT DEFAULT 0, -- denotes this exposure is private
    pairwise TINYINT DEFAULT 0, -- denotes if this exposure should be pairwise diffed
    active TINYINT DEFAULT 0, -- denotes if this exposure is currently in use
    data_state VARCHAR(64) NOT NULL, -- state of exposure
    PRIMARY KEY (lap_id),
    KEY (exp_id),
    KEY (chip_id),
    KEY (pair_id),
    KEY (data_state),
    FOREIGN KEY (lap_id) REFERENCES lapRun(lap_id),
    FOREIGN KEY (exp_id) REFERENCES rawExp(exp_id),
    FOREIGN KEY (chip_id,exp_id) REFERENCES chipRun(chip_id,exp_id),
    FOREIGN KEY (pair_id) REFERENCES chipRun(chip_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;    

CREATE TABLE minidvodbRun (
minidvodb_id BIGINT AUTO_INCREMENT,
       minidvodb_name VARCHAR(64),
       minidvodb_group VARCHAR(64) NOT NULL,
       minidvodb_path VARCHAR(255) NOT NULL,
       state VARCHAR(64) NOT NULL,
       creation_date TIMESTAMP,
       PRIMARY KEY(minidvodb_id), KEY(minidvodb_name), INDEX(minidvodb_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE minidvodbProcessed (
      minidvodb_id BIGINT,
      dtime_resort FLOAT,
      dtime_relphot FLOAT,
      dtime_script FLOAT,
      epoch timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
      fault SMALLINT NOT NULL,
      UNIQUE KEY (minidvodb_id),
      KEY(minidvodb_id),
      FOREIGN KEY(minidvodb_id) REFERENCES minidvodbRun(minidvodb_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;


CREATE TABLE minidvodbCopy (
    minidvodbcopy_id     BIGINT AUTO_INCREMENT,
    minidvodb_id     BIGINT,
    minidvodb_rsync_path VARCHAR(255) NOT NULL,
    destination_host VARCHAR(255) NOT NULL,
    fault  SMALLINT NOT NULL,
    state   VARCHAR(64) NOT NULL,
    epoch TIMESTAMP,
    dtime FLOAT,
    PRIMARY KEY(minidvodbcopy_id),
    KEY(minidvodb_id),
    FOREIGN KEY(minidvodb_id) REFERENCES minidvodbRun(minidvodb_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE mergedvodbRun (
       merge_id BIGINT AUTO_INCREMENT,
       minidvodb_id BIGINT,
       mergedvodb VARCHAR(64) NOT NULL,
       mergedvodb_path VARCHAR(255) NOT NULL,
       state VARCHAR(64) NOT NULL,
       creation_date TIMESTAMP,
       PRIMARY KEY(merge_id), KEY(minidvodb_id), KEY(mergedvodb), INDEX(merge_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE mergedvodbProcessed (
      merge_id BIGINT,
      merge_order BIGINT,
      dtime_verify FLOAT,
      dtime_merge FLOAT,
      dtime_script FLOAT,
      epoch timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
      fault SMALLINT NOT NULL,
      UNIQUE KEY (merge_id),
      KEY(merge_id),
      FOREIGN KEY(merge_id) REFERENCES mergedvodbRun(merge_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE mergedvodbCopy (
    mergedvodbcopy_id     BIGINT AUTO_INCREMENT,
    merge_id     BIGINT,
    mergedvodb_rsync_path VARCHAR(255) NOT NULL,
    destination_host VARCHAR(255) NOT NULL,
    fault  SMALLINT NOT NULL,
    state   VARCHAR(64) NOT NULL,
    epoch TIMESTAMP,
    dtime FLOAT,
    PRIMARY KEY(mergedvodbcopy_id),
    KEY(merge_id),
    FOREIGN KEY(merge_id) REFERENCES mergedvodbRun(merge_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE vpRun (
    vp_id BIGINT AUTO_INCREMENT,
    exp_id BIGINT,
    state VARCHAR(64),
    label VARCHAR(64),
    data_group VARCHAR(64),
    workdir VARCHAR(255),
    note VARCHAR(255),
    dest_id BIGINT,
    outroot VARCHAR(255),
    dtime_script FLOAT,
    hostname VARCHAR(64),
    fault SMALLINT NOT NULL DEFAULT 0,
    PRIMARY KEY(vp_id),
    KEY(exp_id),
    KEY(state),
    KEY(label),
    KEY(data_group),
    KEY(fault),
    INDEX(vp_id, exp_id),
    FOREIGN KEY(exp_id) REFERENCES rawExp(exp_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE vpProcessedCell (
    vp_id BIGINT,
    class_id VARCHAR(64),
    cell_id VARCHAR(64),
    dtime_photom FLOAT,
    quality SMALLINT NOT NULL DEFAULT 0,
    path_base VARCHAR(255),
    fault SMALLINT NOT NULL DEFAULT 0,
    KEY(quality),
    KEY(fault),
    FOREIGN KEY(vp_id) REFERENCES vpRun(vp_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE skycalRun (
      skycal_id BIGINT AUTO_INCREMENT, -- unique identifier
      sky_id BIGINT,
      stack_id BIGINT,
      state VARCHAR(64) NOT NULL,  -- state of run (new, full, etc.)
      workdir VARCHAR(255) NOT NULL, -- working directory
      label VARCHAR(64),             -- processing label
      data_group VARCHAR(64),        -- group for data
      dist_group VARCHAR(64),        -- group for distribution
      reduction VARCHAR(64),         -- reduction class (for altering recipe)
      registered TIMESTAMP DEFAULT CURRENT_TIMESTAMP, -- time run was registered
      note VARCHAR(255),             -- note
      PRIMARY KEY(skycal_id),
      KEY(state),
      KEY(label),
      KEY(data_group),
      KEY(dist_group),
      KEY(sky_id),
      KEY(stack_id),
      FOREIGN KEY(sky_id) REFERENCES staticskyRun(sky_id),
      FOREIGN KEY(stack_id) REFERENCES stackRun(stack_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

-- Result of sky calibration analysis
CREATE TABLE skycalResult (
      skycal_id BIGINT,
      path_base VARCHAR(255) NOT NULL,
      dtime_script FLOAT,
      dtime_astrom FLOAT,
      sigma_ra FLOAT,
      sigma_dec FLOAT,
      n_astrom INT,
      n_detections INT,
      n_extended INT,
      n_forced INT,
      zpt_obs FLOAT,
      zpt_stdev FLOAT,
      fwhm_major  FLOAT,
      fwhm_minor  FLOAT,
      quality SMALLINT NOT NULL,
      hostname VARCHAR(64) NOT NULL,
      software_ver VARCHAR(64),
      fault SMALLINT NOT NULL,
      PRIMARY KEY(skycal_id),
      KEY(fault),
      KEY(quality),
      FOREIGN KEY(skycal_id) REFERENCES skycalRun(skycal_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE skycell (
    tess_id     VARCHAR(64),
    skycell_id  VARCHAR(64),
    radeg       float,
    decdeg      float,
    glong       float,
    glat        float,
    width       float,
    height      float,
    PRIMARY KEY (tess_id, skycell_id),
    KEY(radeg),
    KEY(decdeg),
    KEY(glong),
    KEY(glat)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE survey (
    surveyID    INT,
    surveyName  VARCHAR(16) NOT NULL,
    description VARCHAR(255),
    PRIMARY KEY(surveyID),
    UNIQUE  KEY(surveyName)
) ENGINE=InnoDB CHARSET=latin1;

CREATE TABLE ippRelease (
    rel_id      INT AUTO_INCREMENT,
    surveyID    INT,
    release_name VARCHAR(64),
    release_state VARCHAR(16),   -- active, pending, archive, drop 
    dataRelease INT,         -- PSPS dataRelease
    priority    INT,
    dvodb       VARCHAR(255),
    ubercal_file VARCHAR(255),
    accessLevelMin INT DEFAULT 0,
    PRIMARY KEY(rel_id),
    UNIQUE KEY(surveyID, release_name),
    KEY(release_name),
    KEY(release_state),
    KEY(priority),
    FOREIGN KEY(surveyID) REFERENCES survey(surveyID)
) ENGINE=Innodb DEFAULT CHARSET=latin1;

CREATE TABLE relExp (
    relexp_id   BIGINT AUTO_INCREMENT,
    rel_id      INT,
    exp_id      BIGINT,
    chip_id     BIGINT,         -- links to the runs that supplied the data
    cam_id      BIGINT,         -- for this release
    group_id    INT,            -- id of relGroup that this exposure is contained in
    state       VARCHAR(16),    -- released, pending, archive, drop  
    flags       INT UNSIGNED,   -- flags for relphot, relastro, ??
    zpt_obs     FLOAT,          -- calibrated zero point for this release of
    zpt_stdev   FLOAT,          -- this exposure
    mcal        FLOAT,          -- this exposure
    ubercal_dist INT,           -- ubercal distance (from dvo calibration)
    path_base   VARCHAR(255),   -- path_base of any supporting files for this
                                -- release of this exposure.
    fault SMALLINT NOT NULL,
    registered  DATETIME,       -- insertion time for this row
    time_stamp  DATETIME,       -- time of last update for this row
    PRIMARY KEY (relexp_id),
    FOREIGN KEY(rel_id) REFERENCES ippRelease(rel_id),
    UNIQUE KEY(rel_id, exp_id),
    KEY (state),
    KEY (fault),
    KEY (group_id),
    FOREIGN KEY(exp_id) REFERENCES rawExp(exp_id),
    FOREIGN KEY(chip_id) REFERENCES chipRun(chip_id),
    FOREIGN KEY(cam_id) REFERENCES camRun(cam_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE relStack (
    relstack_id BIGINT AUTO_INCREMENT,
    rel_id      INT,
    stack_id    BIGINT,         -- id of the stackRun
    skycal_id   BIGINT,         -- id of the sky calibration run that supplied the calibration
                                -- for this release of this skycell.
    skycell_id  VARCHAR(64),
    tess_id     VARCHAR(64),
    filter      VARCHAR(16),
    state       VARCHAR(16),    -- released, pending, archive, drop  
    flags       INT UNSIGNED,
    stack_type  VARCHAR(16),    -- nightly, deep, reference
    zpt_obs     FLOAT,          -- calibrated zero point for this release of this skycell
    zpt_stdev   FLOAT,
    mjd_obs     INT UNSIGNED,   -- for nightly stacks the mjd_obs, zero for deep and reference
    path_base   VARCHAR(255),   -- path_base of any supporting files,
    fault SMALLINT NOT NULL,
    registered  DATETIME,
    time_stamp  DATETIME,
    PRIMARY KEY (relstack_id),
    UNIQUE KEY (rel_id, tess_id, skycell_id, filter, stack_type, mjd_obs),
    KEY (tess_id, skycell_id),
    KEY (stack_type),
    KEY (mjd_obs),
    KEY (state),
    KEY (fault),
    FOREIGN KEY(rel_id) REFERENCES ippRelease(rel_id),
    FOREIGN KEY(stack_id) REFERENCES stackRun(stack_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE relGroup (
    group_id    INT AUTO_INCREMENT,
    rel_id      INT,
    group_type  VARCHAR(16),
    lap_id      BIGINT,
    group_name  VARCHAR(16),
    state       VARCHAR(16),    -- new full
    label       VARCHAR(64),
    exp_list_path   VARCHAR(255),
    fault SMALLINT NOT NULL,
    registered  DATETIME,
    PRIMARY KEY (group_id),
    KEY (rel_id, group_name),
    KEY (group_type),
    KEY (lap_id),
    KEY (group_name),
    KEY (state),
    KEY (label),
    KEY (fault),
    FOREIGN KEY(rel_id) REFERENCES ippRelease(rel_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE lapGroup  (
    seq_id  BIGINT,
    tess_id VARCHAR(64),
    projection_cell VARCHAR(64),
    state   VARCHAR(16),
    label   VARCHAR(64),
    registered TIMESTAMP DEFAULT CURRENT_TIMESTAMP, -- time group was registered
    fault   SMALLINT,
    PRIMARY KEY(seq_id, tess_id, projection_cell),
    KEY(state),
    KEY(fault),
    FOREIGN KEY(seq_id) REFERENCES lapSequence(seq_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE dqstatsRun (
  dqstats_id BIGINT NOT NULL AUTO_INCREMENT,
  state VARCHAR(64) DEFAULT NULL,
  registered TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  label VARCHAR(64) DEFAULT NULL,
  fault SMALLINT NOT NULL,
  PRIMARY KEY  (dqstats_id),
  KEY label (label),
  KEY fault (fault)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE dqstatsContent (
  dqstats_id BIGINT,
  exp_id BIGINT,
  chip_id BIGINT,
  cam_id BIGINT,
  warp_id BIGINT,
  invalid TINYINT DEFAULT NULL,
  KEY dqstats_id (dqstats_id),
  KEY exp_id (exp_id),
  KEY chip_id (chip_id),
  KEY cam_id (cam_id),
  KEY warp_id (warp_id),
  FOREIGN KEY (dqstats_id) REFERENCES dqstatsRun (dqstats_id),
  FOREIGN KEY (exp_id) REFERENCES rawExp (exp_id),
  FOREIGN KEY (chip_id) REFERENCES chipRun (chip_id),
  FOREIGN KEY (cam_id) REFERENCES camRun (cam_id),
  FOREIGN KEY (warp_id) REFERENCES warpRun (warp_id)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

CREATE TABLE fullForceRun (
    ff_id           BIGINT NOT NULL AUTO_INCREMENT,
    skycal_id       BIGINT,
    sources_path_base VARCHAR(255),
    state           VARCHAR(64),
    workdir         VARCHAR(255),
    label           VARCHAR(64),
    data_group      VARCHAR(64),
    dist_group      VARCHAR(64),
    note            VARCHAR(255),
    reduction       VARCHAR(64),
    registered TIMESTAMP DEFAULT CURRENT_TIMESTAMP, -- time run was registered
    PRIMARY KEY(ff_id),
    KEY(state),
    KEY(label),
    KEY(data_group),
    KEY(skycal_id),
    FOREIGN KEY(skycal_id) REFERENCES skycalRun(skycal_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE fullForceInput (
    ff_id           BIGINT,
    warp_id         BIGINT,
    PRIMARY KEY(ff_id, warp_id),
    FOREIGN KEY(ff_id) REFERENCES fullForceRun(ff_id),
    FOREIGN KEY(warp_id) REFERENCES warpRun(warp_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE fullForceResult (
    ff_id           BIGINT,
    warp_id         BIGINT,
    path_base       VARCHAR(255) NOT NULL,
    dtime_script    FLOAT,
    quality         SMALLINT NOT NULL,
    hostname        VARCHAR(64) NOT NULL,
    software_ver    VARCHAR(16),
    fault           SMALLINT NOT NULL,
    PRIMARY KEY(ff_id, warp_id),
    KEY(fault),
    KEY(quality),
    FOREIGN KEY(ff_id) REFERENCES fullForceRun(ff_id),
    FOREIGN KEY(warp_id) REFERENCES warpRun(warp_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE fullForceSummary (
    ff_id           BIGINT,
    path_base       VARCHAR(255) NOT NULL,
    dtime_script    FLOAT,
    quality         SMALLINT NOT NULL,
    hostname        VARCHAR(64) NOT NULL,
    software_ver    VARCHAR(16),
    fault           SMALLINT NOT NULL,
    PRIMARY KEY(ff_id),
    KEY(fault),
    KEY(quality),
    FOREIGN KEY(ff_id) REFERENCES fullForceRun(ff_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE remoteRun (
    remote_id BIGINT NOT NULL AUTO_INCREMENT,
    state     VARCHAR(64) NOT NULL,
    stage     VARCHAR(64) NOT NULL,
    label     VARCHAR(64) NOT NULL,
    path_base VARCHAR(255) NOT NULL,
    job_id    BIGINT,
    last_poll TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    fault     SMALLINT NOT NULL,
    PRIMARY KEY (remote_id),
    KEY (state),
    KEY (stage),
    KEY (label),
    KEY (job_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE remoteComponent (
    remote_id BIGINT,
    stage_id  BIGINT,
    jobs      INT,
    state     VARCHAR(64) NOT NULL,
    path_base VARCHAR(255),
    KEY (stage_id),
    FOREIGN KEY (remote_id) REFERENCES remoteRun(remote_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE fpcamRun (
    fpcam_id      BIGINT AUTO_INCREMENT,
    cam_id        BIGINT,
    chip_id       BIGINT,
    state         VARCHAR(64),
    workdir       VARCHAR(255),
    workdir_state VARCHAR(64),
    label         VARCHAR(64),
    data_group    VARCHAR(64),
    dist_group    VARCHAR(64),
    reduction     VARCHAR(64),
    dvodb         VARCHAR(255),
    software_ver  VARCHAR(16),
    note          VARCHAR(255),
    PRIMARY KEY(fpcam_id),
    KEY(cam_id),
    KEY(chip_id),
    KEY(state),
    KEY(label),
    INDEX(chip_id, cam_id),
    FOREIGN KEY (cam_id) REFERENCES camRun(cam_id),
    FOREIGN KEY (chip_id) REFERENCES chipRun(chip_id))
ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE fpcamProcessedExp (
    fpcam_id     BIGINT,
    path_base    VARCHAR(255),

    zpt_obs      FLOAT,
    zpt_stdev    FLOAT,
    zpt_lq       FLOAT,
    zpt_uq       FLOAT,

    dtime_script FLOAT,

    hostname     VARCHAR(64),
    n_stars      INT,
    fault        SMALLINT NOT NULL,
    epoch        TIMESTAMP DEFAULT CURRENT_TIMESTAMP,

    software_ver VARCHAR(16),

    deteff_obs   FLOAT,
    deteff_err   FLOAT,
    deteff_lq    FLOAT,
    deteff_uq    FLOAT,

    quality      SMALLINT,

    PRIMARY KEY(fpcam_id),
    KEY(fault),
    FOREIGN KEY (fpcam_id) REFERENCES fpcamRun(fpcam_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

-- These comment lines are here to avoid an empty query error.
-- Another way to avoid that problem is to omit the semicolon above but I think that is untidy.
