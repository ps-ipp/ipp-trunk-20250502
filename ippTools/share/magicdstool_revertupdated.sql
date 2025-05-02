UPDATE magicDSRun 
    JOIN magicDSFile using(magic_ds_id)
SET magicDSFile.fault = 0
WHERE
    magicDSRun.state = 'update'
    AND magicDSFile.data_state = 'update'
    AND magicDSFile.fault != 0
