UPDATE chipProcessedImfile LEFT JOIN camRun USING(chip_id)
    SET chipProcessedImfile.quality = %d,
        chipProcessedImfile.data_state = 'full',
        chipProcessedImfile.magicked = 0,
        chipProcessedImfile.fault = 0
