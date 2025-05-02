   SELECT
    addRun.*,
    diffSkyfile.path_base as stageroot,
    'GPC1' as camera,
    'PS1' as telescope
FROM addRun
JOIN diffInputSkyfile
    ON (diff_id = stage_id and diff_skyfile_id = stage_extra1 and stage = "diff")
JOIN diffSkyfile
    USING (diff_id, skycell_id)
JOIN diffRun
    using(diff_id)
LEFT JOIN addProcessedExp using (add_id)
LEFT JOIN addMask
    ON addRun.label = addMask.label
WHERE
    diffRun.state = 'full'
    AND ((addRun.state = 'new' AND addProcessedExp.add_id IS NULL) OR addRun.state = 'update')
    AND addRun.dvodb IS NOT NULL
    AND addRun.workdir IS NOT NULL
    AND addMask.label IS NULL
