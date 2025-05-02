SELECT
    chipProcessedImfile.*,
    chipRun.state,
    chipRun.workdir,
    chipRun.label,
    chipRun.reduction,
    chipRun.expgroup,
    chipRun.dvodb,
    chipRun.tess_id,
    chipRun.end_stage
FROM chipRun
JOIN chipProcessedImfile
    USING(chip_id)
WHERE
    ((chipRun.state = 'goto_cleaned'
        AND (chipProcessedImfile.data_state = 'full'
          OR chipProcessedImfile.data_state = 'update'
          OR chipProcessedImfile.data_state = 'error_cleaned')
        AND chipProcessedImfile.quality = 0)
OR 
    (chipRun.state = 'goto_scrubbed' AND chipProcessedImfile.data_state != 'scrubbed')
OR 
    (chipRun.state = 'goto_purged' AND chipProcessedImfile.data_state != 'purged'))
