SELECT
    chipRun.*,
    camRun.cam_id,
    chip_bg_id,
    CURRENT_TIMESTAMP AS registered
FROM chipRun
JOIN rawExp USING(exp_id)
JOIN chipRun as altChipRun USING(exp_id)
JOIN camRun ON altChipRun.chip_id = camRun.chip_id
JOIN camProcessedExp USING(cam_id)
LEFT JOIN chipBackgroundRun ON chipBackgroundRun.chip_id = chipRun.chip_id -- labelHook %s
WHERE chipRun.state = 'full' AND camRun.state ='full' AND camProcessedExp.quality = 0
