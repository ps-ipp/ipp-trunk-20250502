SELECT
    addProcessedExp.*,
    addRun.workdir
FROM addProcessedExp
JOIN addRun
    USING(add_id)
