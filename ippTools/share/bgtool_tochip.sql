SELECT
    chipBackgroundRun.*,
    chipProcessedImfile.class_id,
    rawExp.exp_tag,
    rawExp.camera
FROM chipBackgroundRun
JOIN chipRun USING(chip_id)
JOIN rawExp USING(exp_id)
JOIN chipProcessedImfile USING(chip_id)
JOIN camRun using(cam_id)
JOIN chipRun AS bgsub_chipRun on camRun.chip_id = bgsub_chipRun.chip_id
JOIN chipProcessedImfile AS bgsub_chipProcessedImfile 
    ON (bgsub_chipRun.chip_id = bgsub_chipProcessedImfile.chip_id 
        AND bgsub_chipProcessedImfile.class_id = chipProcessedImfile.class_id)
LEFT JOIN chipBackgroundImfile  
    ON (chipBackgroundRun.chip_bg_id = chipBackgroundImfile.chip_bg_id 
        AND chipBackgroundImfile.class_id = chipProcessedImfile.class_id)
LEFT JOIN Label ON chipBackgroundRun.label = Label.label
WHERE chipBackgroundImfile.chip_bg_id IS NULL
    AND chipRun.state = 'full'
    AND chipProcessedImfile.fault = 0
    AND chipProcessedImfile.quality = 0
    AND bgsub_chipProcessedImfile.quality = 0
    AND (Label.active OR Label.active IS NULL)
-- WHERE hook %s
ORDER BY priority DESC, chip_bg_id, class_id
