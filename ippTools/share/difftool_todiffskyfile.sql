-- This query is a little complicated, since it contains multiple self-joins.
-- This is because of the options involved: both templates and inputs can be either warps or stacks.
-- All of the templates and inputs need to exist, so we query against all of them.
SELECT DISTINCT
    diffRun.diff_id,
    diffInputSkyfile.diff_skyfile_id,
    diffRun.workdir,
    diffInputSkyfile.skycell_id,
    diffRun.tess_id,
    diffRun.label,
    diffRun.state,
    diffRun.reduction,
    diffRun.bothways,
    diffRun.diff_mode,
    diffSkyfile.path_base,
    IFNULL(priority, 10000) AS priority
FROM diffRun
JOIN diffInputSkyfile USING(diff_id)

-- Get list of templates for each diffRun
-- JOIN diffInputSkyfile AS diffTemplateSkyfile
--     ON diffRun.diff_id = diffTemplateSkyfile.diff_id
--     AND diffRun.skycell_id = diffTemplateSkyfile.skycell_id
--     AND diffTemplateSkyfile.template = 1
-- Get list of inputs for each diffRun
-- JOIN diffInputSkyfile
--     ON diffRun.diff_id = diffInputSkyfile.diff_id
--     AND diffRun.skycell_id = diffInputSkyfile.skycell_id
--     AND diffInputSkyfile.template = 0

-- Get warp templates
LEFT JOIN warpRun AS warpTemplateRun
    ON warpTemplateRun.warp_id = diffInputSkyfile.warp2
    AND diffInputSkyfile.warp2 IS NOT NULL
LEFT JOIN warpSkyfile AS warpTemplateSkyfile
    ON warpTemplateSkyfile.warp_id = warpTemplateRun.warp_id
    AND warpTemplateSkyfile.skycell_id = diffInputSkyfile.skycell_id

-- Get warp inputs
LEFT JOIN warpRun
    ON warpRun.warp_id = diffInputSkyfile.warp1
    AND diffInputSkyfile.warp1 IS NOT NULL
LEFT JOIN warpSkyfile
    ON warpSkyfile.warp_id = warpRun.warp_id
    AND warpSkyfile.skycell_id = diffInputSkyfile.skycell_id

-- Get stack templates
LEFT JOIN stackRun AS stackTemplateRun
    ON stackTemplateRun.stack_id = diffInputSkyfile.stack2
    AND diffInputSkyfile.stack2 IS NOT NULL
LEFT JOIN stackSumSkyfile AS stackTemplateSkyfile
    ON stackTemplateSkyfile.stack_id = stackTemplateRun.stack_id

-- Get stack inputs
LEFT JOIN stackRun
    ON stackRun.stack_id = diffInputSkyfile.stack1
    AND diffInputSkyfile.stack1 IS NOT NULL
LEFT JOIN stackSumSkyfile
    ON stackSumSkyfile.stack_id = stackRun.stack_id

-- Get what's already been processed
LEFT JOIN diffSkyfile
    ON diffInputSkyfile.diff_id = diffSkyfile.diff_id
    AND diffInputSkyfile.skycell_id = diffSkyfile.skycell_id
LEFT JOIN Label ON Label.label = diffRun.label
WHERE
-- Ready to be processed
    ((diffRun.state = 'new'
    AND diffSkyfile.diff_id IS NULL)
    OR (diffRun.state = 'update'
    AND diffSkyfile.fault = 0
    AND diffSkyfile.data_state = 'update')
    )
    AND (Label.active OR Label.active IS NULL)
-- Ensure input warps are available
    AND (diffInputSkyfile.warp1 IS NULL
    OR (warpSkyfile.data_state = 'full'
    AND warpSkyfile.fault = 0
--    AND warpSkyfile.magicked >= 0
    AND warpSkyfile.quality = 0))
-- Ensure input stacks are available
    AND (diffInputSkyfile.stack1 IS NULL
    OR (stackRun.state = 'full'
    AND stackSumSkyfile.fault = 0
    AND stackSumSkyfile.quality = 0))
-- Ensure template warps are available
    AND (diffInputSkyfile.warp2 IS NULL
    OR (warpTemplateSkyfile.data_state = 'full'
    AND warpTemplateSkyfile.fault = 0
--    AND warpSkyfile.magicked >= 0
    AND warpTemplateSkyfile.quality = 0))
-- Ensure template stacks are available
    AND (diffInputSkyfile.stack2 IS NULL
    OR (stackTemplateRun.state = 'full'
    AND stackTemplateSkyfile.fault = 0
    AND stackTemplateSkyfile.quality = 0))

