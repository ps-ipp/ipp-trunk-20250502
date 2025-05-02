-- handle changes in data_state. Used for the modes tocleanedimfile and topurgedimfile
-- args are new data_state, fake_id, class_id, and current expected state for fakeRun
UPDATE fakeProcessedImfile
    SET 
    data_state = '%s'
WHERE
    fake_id = %lld
    AND class_id = '%s'
    -- only update if fakeRun.state has the expected value
    AND (
        SELECT state from fakeRun where fakeRun.fake_id = fakeProcessedImfile.fake_id
    ) = '%s'
    

