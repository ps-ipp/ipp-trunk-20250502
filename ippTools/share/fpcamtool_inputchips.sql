SELECT
    exp_id,
    chip_id,
    fpcam_id,
    path_base,
    quality,
    class_id
FROM fpcamRun
JOIN chipProcessedImfile
    USING(chip_id)
