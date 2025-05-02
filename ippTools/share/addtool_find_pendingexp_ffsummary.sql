   SELECT
    addRun.*,
    fullForceSummary.path_base as stageroot,
    'GPC1' as camera,
    'PS1' as telescope
FROM addRun
JOIN fullForceSummary
    ON (ff_id = stage_id and stage = "fullforce_summary")
JOIN fullForceRun
    using(ff_id)
LEFT JOIN addProcessedExp using (add_id)
LEFT JOIN addMask
    ON addRun.label = addMask.label
WHERE
    fullForceRun.state = 'full'
    AND ((addRun.state = 'new' AND addProcessedExp.add_id IS NULL) OR addRun.state = 'update')
    AND addRun.dvodb IS NOT NULL
    AND addRun.workdir IS NOT NULL
    AND addMask.label IS NULL
