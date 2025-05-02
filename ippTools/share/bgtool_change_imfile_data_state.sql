UPDATE chipBackgroundImfile
    JOIN chipBackgroundRun USING(chip_bg_id)
SET data_state = '%s',
    chipBackgroundImfile.magicked = IF(chipBackgroundImfile.magicked != 0, -1, 0)
WHERE chip_bg_id = %lld AND class_id = '%s'
