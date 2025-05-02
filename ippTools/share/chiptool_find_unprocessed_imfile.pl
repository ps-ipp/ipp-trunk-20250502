SELECT DISTINCT
   chipInputImfile.*
FROM chipInputImfile
LEFT JOIN chipProcessedImfile
    USING(chip_id, class_id)
WHERE
    chipProcessedImfile.chip_id IS NULL
    AND chipProcessedImfile.class_id IS NULL
