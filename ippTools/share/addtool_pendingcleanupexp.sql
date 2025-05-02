SELECT
    addProcessedExp.*,
    addRun.state,
    addRun.workdir,
    addRun.label,
    addRun.dvodb,
FROM addRun
JOIN addProcessedExp
    USING(add_id)
WHERE
    (addRun.state = 'goto_cleaned' OR addRun.state = 'goto_purged' OR addRun.state = 'goto_scrubbed')

