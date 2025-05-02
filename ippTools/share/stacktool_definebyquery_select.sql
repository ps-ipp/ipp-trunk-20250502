-- This is a query to get a list of skycells with warps to be stacked,
-- along with the number of warps already in a stack.

SELECT
    tess_id,
    skycell_id,
    filter,
    num_warp,
    MAX(num_stack) AS num_stack
FROM ((
    -- Number of stack-ready warps as a function of skycell and filter
    SELECT
        warpSkyfile.tess_id as tess_id,
        skycell_id,
        rawExp.filter,
        COUNT(warpSkyfile.skycell_id) AS num_warp -- number of warps that can be stacked
    FROM warpRun
    JOIN warpSkyfile USING(warp_id)
    JOIN fakeRun USING(fake_id)
    JOIN camRun USING(cam_id)
    JOIN camProcessedExp USING(cam_id)
    JOIN chipRun USING(chip_id)
    JOIN rawExp USING(exp_id)
    WHERE
        warpRun.state = 'full'
        AND warpSkyfile.fault = 0
        AND warpSkyfile.quality = 0
    -- WHERE hook %s
    GROUP BY
        warpSkyfile.tess_id,
        skycell_id,
        filter
    ) AS warpsToStack
LEFT JOIN (
    -- Number of stack inputs as a function of skycell and filter
    SELECT
        tess_id,
        skycell_id,
        filter,
        COUNT(stackInputSkyfile.warp_id) as num_stack -- number of warps in a stack
    FROM stackRun
    JOIN stackInputSkyfile USING(stack_id)
    -- WHERE hook %s
    GROUP BY
        stack_id
    ) AS stackSizes
-- JOINing the warpsToStack and stackSizes tables
    USING(tess_id, skycell_id, filter)
    )
GROUP BY
    tess_id,
    skycell_id,
    filter

