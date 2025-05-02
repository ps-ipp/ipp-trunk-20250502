SELECT DISTINCT
    rawImfile.*,
    warpRun.fake_id,
    camRun.cam_id as cam_id,
    chipProcessedImfile.uri as chip_uri,
    chipProcessedImfile.path_base as chip_path_base,
    camProcessedExp.path_base as cam_path_base
FROM warpRun
JOIN fakeRun
    USING(fake_id)
JOIN camRun
    USING(cam_id)
JOIN camProcessedExp
    USING(cam_id)
JOIN chipRun
    ON camRun.chip_id = chipRun.chip_id
JOIN chipProcessedImfile
    ON chipRun.chip_id = chipProcessedImfile.chip_id
JOIN rawImfile -- is there any reason not to refer back to rawimfiles?
    ON chipProcessedImfile.exp_id = rawImfile.exp_id
    AND chipProcessedImfile.class_id = rawImfile.class_id
    AND rawImfile.ignored = 0
WHERE
    warpRun.state = 'new'
    AND fakeRun.state = 'full'
    AND camRun.state = 'full'
    AND chipRun.state = 'full'
    AND chipProcessedImfile.quality = 0
    and camProcessedExp.quality = 0

