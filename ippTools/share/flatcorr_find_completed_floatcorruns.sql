SELECT DISTINCT
    flatcorrRun.*
FROM flatcorrRun
JOIN flatcorrExp
    USING(corr_id)
JOIN chipRunDone
    USING(chip_id)
WHERE
    flatcorrRun.state != "stop"
GROUP BY
    corr_id, chip_id
HAVING
    COUNT(flatcorrExp.chip_id) = COUNT(chipRunDone.chip_id)
