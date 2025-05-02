SELECT
    *
FROM
flatcorrRun
JOIN flatcorrExp
    USING(corr_id)
JOIN chipRun
    USING(chip_id)
JOIN rawImfile
    USING(exp_id)
LEFT JOIN chipProcessedImfile
    ON chipRun.chip_id = chipProcessedImfile.chip_id
    AND rawImfile.exp_id = chipProcessedImfile.exp_id
    AND rawImfile.class_id = chipProcessedImfile.class_id
WHERE
    flatcorrRun.state = 'run'
    AND chipRun.state = 'full'
GROUP BY
    chipRun.chip_id,
    chipRun.exp_id
HAVING
    COUNT(rawImfile.class_id) = COUNT(chipProcessedImfile.class_id)
    AND SUM(chipProcessedImfile.fault > 0) = 0

