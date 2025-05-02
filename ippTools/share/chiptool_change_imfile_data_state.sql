-- handle changes in data_state. Used for the modes tocleanedimfile and topurgedimfile
-- args are new data_state, a possibly empty string for updating the magicked state,
-- chip_id and class_id
UPDATE chipProcessedImfile
JOIN chipRun USING(chip_id, exp_id)
JOIN rawImfile USING(exp_id, class_id)
    SET 
    chipProcessedImfile.data_state = '%s'
    -- set magicked hook %s
WHERE
    chip_id = %lld
    AND class_id = '%s'
