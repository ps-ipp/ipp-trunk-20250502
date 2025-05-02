UPDATE chipBackgroundRun
SET chipBackgroundRun.state = '%s',
    chipBackgroundRun.magicked = IF(chipBackgroundRun.magicked != 0, -1, 0)
WHERE chip_bg_id = %lld
AND (SELECT COUNT(class_id)
    FROM chipBackgroundImfile 
    WHERE 
        chipBackgroundImfile.chip_bg_id = chipBackgroundRun.chip_bg_id
        AND data_state != '%s'
    ) = 0
