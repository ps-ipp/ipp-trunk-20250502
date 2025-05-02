SELECT
    corr_id,
    chipProcessedImfile.*
FROM flatcorrRun
JOIN flatcorrExp
    USING(corr_id)
JOIN chipRun
    USING(chip_id)
JOIN chipProcessedImfile
    USING(chip_id)
WHERE
    flatcorrRun.state = 'run'
    AND chipRun.state = 'new'

