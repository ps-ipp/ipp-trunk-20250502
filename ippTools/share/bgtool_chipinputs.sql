SELECT
    chipProcessedImfile.*,
    camProcessedExp.path_base AS cam_path_base
FROM chipBackgroundRun
JOIN chipRun USING(chip_id)
JOIN chipProcessedImfile USING(chip_id)
LEFT JOIN camProcessedExp ON chipBackgroundRun.cam_id = camProcessedExp.cam_id
WHERE chipRun.state = 'full'
    AND chipProcessedImfile.fault = 0
    AND chipProcessedImfile.quality = 0
