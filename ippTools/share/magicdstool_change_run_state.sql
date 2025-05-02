-- change state of magicDSRun from goto_cleaned to cleaned or goto_purged to purged
-- when all of the constituant imfiles are in the right state
-- arguments are new state (cleaned or purged) chip_id and new state again for 
-- the chipProcessedImfile sub query
UPDATE magicDSRun
    SET magicDSRun.state = '%s'
WHERE
    magicDSRun.magic_ds_id = %lld
    AND (SELECT
        COUNT(magic_ds_id)
        FROM magicDSFile
        WHERE
            magicDSFile.magic_ds_id = magicDSRun.magic_ds_id
            AND data_state != '%s'
        ) = 0
