UPDATE chipProcessedImfile
    JOIN chipRun using(chip_id,exp_id)
SET chipProcessedImfile.data_state = 'full', 
    chipRun.state = '%s'
WHERE
    chipRun.state = '%s'
    AND chipProcessedImfile.data_state = '%s'
