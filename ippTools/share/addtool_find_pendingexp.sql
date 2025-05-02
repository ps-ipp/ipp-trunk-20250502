SELECT
    addRun.*
FROM addRun
LEFT JOIN addProcessedExp
    USING(add_id)
LEFT JOIN addMask
    ON addRun.label = addMask.label
WHERE
((addRun.state = 'new' AND addProcessedExp.add_id IS NULL) OR addRun.state = 'update')
    AND addRun.dvodb IS NOT NULL
    AND addRun.workdir IS NOT NULL
    AND addMask.label IS NULL

