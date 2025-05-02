UPDATE chipProcessedImfile
    JOIN chipRun using(chip_id,exp_id)
    JOIN rawExp using(exp_id)
SET chipProcessedImfile.fault = 0
WHERE
    chipRun.state = 'update'
    AND chipProcessedImfile.data_state = 'update'
    AND chipProcessedImfile.fault != 0
