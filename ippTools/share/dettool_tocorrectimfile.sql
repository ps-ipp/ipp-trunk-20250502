-- select imfiles from detRun 1 (the reference detRun) that match the desired correction in detRun 2
SELECT
    det1.*,
    imfile.class_id,
    imfile.uri,
    imfile.bg,
    imfile.bg_stdev,
    imfile.bg_mean_stdev,
    imfile.user_1,
    imfile.user_2,
    imfile.user_3,
    imfile.user_4,
    imfile.user_5,
    imfile.path_base,
    imfile.fault
FROM detRun AS det1
JOIN detRun as det2
    ON det1.ref_det_id = det2.det_id 
JOIN detNormalizedImfile as imfile
    ON det1.ref_det_id = imfile.det_id
    AND det1.ref_iter = imfile.iteration
LEFT JOIN detRegisteredImfile
    ON det1.det_id = detRegisteredImfile.det_id
    AND det1.iteration = detRegisteredImfile.iteration
    AND imfile.class_id = detRegisteredImfile.class_id
WHERE 
    det1.state = 'run'
    AND det1.mode = 'correction'
    AND det2.state = 'stop'
    AND detRegisteredImfile.det_id IS NULL
    AND detRegisteredImfile.iteration IS NULL
    AND detRegisteredImfile.class_id IS NULL
