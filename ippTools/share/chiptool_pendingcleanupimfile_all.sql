-- list of chipProcessedImfiles for runs in a goto_cleaned like state without regard to 
-- data_state or quality. Used for confirming that a run is really in the cleaned state
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
WHERE ( chipRun.state = 'goto_cleaned' OR chipRun.state = 'goto_scrubbed' OR chipRun.state = 'goto_purged' )
