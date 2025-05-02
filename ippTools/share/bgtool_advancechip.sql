SELECT DISTINCT
    chipBackgroundRun.chip_bg_id,
    chipBackgroundImfile.magicked
FROM chipBackgroundRun
JOIN camRun using(cam_id)
JOIN chipProcessedImfile ON chipBackgroundRun.chip_id = chipProcessedImfile.chip_id
JOIN chipRun AS bgsub_chipRun on camRun.chip_id = bgsub_chipRun.chip_id
JOIN chipProcessedImfile AS bgsub_chipProcessedImfile 
    ON (bgsub_chipRun.chip_id = bgsub_chipProcessedImfile.chip_id 
        AND bgsub_chipProcessedImfile.class_id = chipProcessedImfile.class_id)
LEFT JOIN chipBackgroundImfile 
    ON (chipBackgroundRun.chip_bg_id = chipBackgroundImfile.chip_bg_id
        AND chipBackgroundImfile.class_id = chipProcessedImfile.class_id)
WHERE chipBackgroundRun.state = 'new'
    AND chipProcessedImfile.quality = 0
    AND chipProcessedImfile.fault = 0
    AND bgsub_chipProcessedImfile.quality = 0
-- WHERE hook %s
GROUP BY chipBackgroundRun.chip_bg_id
HAVING
    COUNT(chipBackgroundImfile.class_id) = COUNT(chipProcessedImfile.class_id)
    AND SUM(IF(chipBackgroundImfile.fault > 0, 1, 0)) = 0
    AND SUM(IF(chipBackgroundImfile.quality > 0, 1, 0)) != COUNT(*)
