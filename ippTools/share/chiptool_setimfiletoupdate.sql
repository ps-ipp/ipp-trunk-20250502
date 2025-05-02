UPDATE chipRun
    JOIN chipProcessedImfile USING(chip_id)
    -- LEFT JOIN magicDSRun ON (stage_id = chip_id AND stage = 'chip')
    -- LEFT JOIN magicDSFile ON (magicDSRun.magic_ds_id = magicDSFile.magic_ds_id
      --                         AND component = class_id)
SET chipRun.state = 'update', 
    chipProcessedImfile.data_state = 'update',
    chipProcessedImfile.fault = 0
    -- set hook %s
WHERE chip_id = %lld
    AND (chipRun.state = 'cleaned' OR chipRun.state = 'update')
    AND (chipProcessedImfile.data_state = 'cleaned')
    AND (chipProcessedImfile.quality = 0)
    -- don't queue update if the associated magicDSFile exists and isn't cleaned
    -- AND (chipRun.magicked = 0 
    --   OR ((magicDSRun.state = 'cleaned' OR magicDSRun.state = 'update')
    --         AND (magicDSFile.data_state = 'cleaned' OR magicDSFile.data_state = 'update'))
    -- )
