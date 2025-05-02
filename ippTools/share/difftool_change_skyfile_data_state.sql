-- handle changes in diffSkyfile.data_state.
-- Used for the modes tocleanedskyfile and topurgedskyfile
-- args are new data_state, diff_id, skycell_id and current expected state for diffRun
UPDATE diffSkyfile
    SET 
    data_state = '%s'
    -- magicked hook %s
WHERE
    diff_id = %lld
    AND skycell_id = '%s'
