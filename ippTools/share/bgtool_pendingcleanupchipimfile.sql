SELECT
    chipBackgroundImfile.*,
    chipBackgroundRun.state,
    chipBackgroundRun.workdir,
    chipBackgroundRun.label
FROM chipBackgroundRun
JOIN chipBackgroundImfile
    USING(chip_bg_id)
WHERE
   ((chipBackgroundRun.state = 'goto_cleaned' AND (chipBackgroundImfile.data_state = 'full'
                                       OR chipBackgroundImfile.data_state = 'update'))
OR 
   (chipBackgroundRun.state = 'goto_scrubbed' AND chipBackgroundImfile.data_state != 'scrubbed')
OR 
   (chipBackgroundRun.state = 'goto_purged' AND chipBackgroundImfile.data_state != 'purged'))
