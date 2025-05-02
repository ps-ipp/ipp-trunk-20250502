-- change state of fakeRun from goto_cleaned to cleaned or goto_purged to purged
-- when all of the constituant imfiles are in the right state
-- arguments are new state (cleaned or purged) fake_id and new state again for 
-- the fakeProcessedImfile sub query
UPDATE fakeRun
    SET state = '%s'
    WHERE
    fakeRun.fake_id = %lld
    AND (SELECT
        COUNT(fake_id)
        FROM fakeProcessedImfile
        WHERE
            fakeProcessedImfile.fake_id = fakeRun.fake_id
            AND data_state != '%s'
        ) = 0
