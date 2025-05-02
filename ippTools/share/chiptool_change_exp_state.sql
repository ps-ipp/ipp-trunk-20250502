-- change state of chipRun from goto_cleaned to cleaned or goto_purged to purged
-- when all of the constituant imfiles are in the right state
-- arguments are new state (cleaned or purged) chip_id and new state again for 
-- the chipProcessedImfile sub query
UPDATE chipRun
JOIN rawExp using(exp_id)
    SET chipRun.state = '%s', chipRun.update_mode = 0
    -- set magicked hook %s
    WHERE
    chipRun.chip_id = %lld
    AND (SELECT
        COUNT(chip_id)
        FROM chipProcessedImfile
        WHERE
            chipProcessedImfile.chip_id = chipRun.chip_id
            AND chipProcessedImfile.quality = 0
            AND data_state != '%s'
        ) = 0
