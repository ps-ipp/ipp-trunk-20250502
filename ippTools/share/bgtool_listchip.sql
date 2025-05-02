SELECT chip_bg_id,
    class_id,
    chipProcessedImfile.path_base
FROM chipBackgroundRun
    JOIN chipBackgroundImfile USING(chip_bg_id)
    JOIN chipProcessedImfile USING(chip_id, class_id)
