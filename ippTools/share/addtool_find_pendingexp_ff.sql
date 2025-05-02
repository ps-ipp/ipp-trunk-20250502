   SELECT
    addRun.*,
    fullForceResult.path_base as stageroot,
    'GPC1' as camera,
    'PS1' as telescope
FROM addRun
JOIN fullForceResult
    ON (ff_id = stage_id and warp_id = stage_extra1 and stage = "fullforce")
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
