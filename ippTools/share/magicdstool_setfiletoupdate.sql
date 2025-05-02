UPDATE magicDSRun
    JOIN magicDSFile USING(magic_ds_id)
SET magicDSRun.state = 'update', 
    magicDSFile.data_state = 'update',
    magicDSFile.fault = 0
    -- set hook %s
WHERE magic_ds_id = %lld
    AND (magicDSRun.state = 'cleaned' OR magicDSRun.state = 'update')
    AND (magicDSFile.data_state = 'cleaned' OR magicDSFile.data_state ='update')
