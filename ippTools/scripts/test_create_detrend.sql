-- disable constraints

DROP TABLE IF EXISTS detRun;
DROP TABLE IF EXISTS detProcessedImfile;
DROP TABLE IF EXISTS detNormalizedStatImfile;
DROP TABLE IF EXISTS detResidImfile;

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
    -- parent INT, :: dropping this
    PRIMARY KEY(det_id),
    KEY(det_id),
    KEY(iteration),
    KEY(det_type),
    KEY(mode),
    KEY(state),
    KEY(label),
    -- KEY(parent), :: dropping this
    INDEX(det_id, iteration))
ENGINE=innodb DEFAULT CHARSET=latin1;

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
    INDEX(det_id, exp_id)
    -- FOREIGN KEY (det_id, exp_id)
    --     REFERENCES  detInputExp(det_id, exp_id),
    -- FOREIGN KEY (exp_id, class_id)
    --     REFERENCES  rawImfile(exp_id, class_id)
) ENGINE=innodb DEFAULT CHARSET=latin1;

CREATE TABLE detNormalizedStatImfile (
    det_id BIGINT,
    iteration INT,
    class_id VARCHAR(64),
    norm FLOAT,
    data_state VARCHAR(64),
    fault SMALLINT NOT NULL,
    PRIMARY KEY(det_id, iteration, class_id),
    KEY(fault)
    -- FOREIGN KEY (det_id, iteration)
    -- REFERENCES  detInputExp(det_id, iteration),
    -- FOREIGN KEY (det_id, iteration, class_id)
    -- REFERENCES  detStackedImfile(det_id, iteration, class_id)
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
    INDEX(det_id, iteration, exp_id)
    -- FOREIGN KEY (det_id, iteration, exp_id)
    -- REFERENCES  detInputExp(det_id, iteration, exp_id),
    -- FOREIGN KEY (det_id, exp_id, class_id)
    -- REFERENCES  detProcessedImfile(det_id, exp_id, class_id),
    -- FOREIGN KEY (ref_det_id, ref_iter)
    -- REFERENCES  detNormalizedExp(det_id, iteration)
) ENGINE=innodb DEFAULT CHARSET=latin1;

