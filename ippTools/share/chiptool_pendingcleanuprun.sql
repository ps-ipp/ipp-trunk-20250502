SELECT DISTINCT
    chipRun.chip_id,
    rawExp.camera,
    rawExp.exp_tag,
    chipRun.state,
    chipRun.workdir,
    chipRun.label,
    IFNULL(Label.priority, 10000) AS priority
FROM chipRun
JOIN rawExp 
USING (exp_id)
LEFT JOIN Label ON chipRun.label = Label.label
WHERE
    (chipRun.state = 'goto_cleaned' OR chipRun.state = 'goto_scrubbed' OR chipRun.state = 'goto_purged')
