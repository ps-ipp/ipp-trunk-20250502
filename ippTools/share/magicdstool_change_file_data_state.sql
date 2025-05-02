-- handle changes in data_state.
UPDATE magicDSFile
    JOIN magicDSRun USING(magic_ds_id)
SET 
    magicDSFile.data_state = '%s'
WHERE
    magic_ds_id = %lld
    AND component = '%s'
