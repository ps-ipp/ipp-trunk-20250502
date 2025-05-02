SELECT
    fpcam_id,
    cam_id,
    quality,
    path_base
FROM fpcamRun
JOIN camProcessedExp
    USING(cam_id)
