-- change state of ddiffRun from goto_cleaned to cleaned or goto_purged to purged
-- when all of the consituant skyfiles are in the end state
-- arguments are new state (cleaned or purged) diff_id and new state again for 
-- the chipProcessedImfile sub query
UPDATE diffRun
    SET state = '%s'
    -- set magicked hook %s
    WHERE
    diffRun.diff_id = %lld
    AND (SELECT
        COUNT(diff_id)
        FROM diffSkyfile
        WHERE
            diffSkyfile.diff_id = diffRun.diff_id
            AND data_state != '%s'
            AND diffSkyfile.quality = 0
        ) = 0
