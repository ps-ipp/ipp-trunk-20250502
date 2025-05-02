-- insert skycells to diff
INSERT INTO skycellsToDiff
SELECT
    0,                      -- diff_id
    warpSkyfile.skycell_id,
    warpSkyfile.warp_id,    -- warp1
    NULL,                   -- stack1
    NULL,                   -- warp2
    max_stack_id,           -- stack2
    warpSkyfile.tess_id,
    0
FROM warpSkyfile
JOIN warpRun USING(warp_id)
LEFT JOIN (
    SELECT
        MAX(stack_id) AS max_stack_id, -- most recent stack, by virtue of auto-increment
        stackRun.skycell_id,
        stackRun.tess_id,
        filter
    FROM stackRun
    JOIN stackSumSkyfile USING(stack_id)
    WHERE stackSumSkyfile.fault = 0
        AND stackSumSkyfile.quality = 0
        AND stackRun.state = 'full'
    -- stacks where hook %s
    GROUP BY
        skycell_id,
        filter
    ) as bestStacks USING(skycell_id)
WHERE
    warpSkyfile.warp_id = %lld
    AND warpRun.state = 'full'
    AND warpSkyfile.fault = 0
    AND warpSkyfile.quality = 0
    AND filter = '%s'
    AND warpSkyfile.tess_id = bestStacks.tess_id
-- warp where hook %s
