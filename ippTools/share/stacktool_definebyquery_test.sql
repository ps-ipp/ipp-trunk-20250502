-- This is the combination of the two parts of the query to get a list
-- of skycells with warps to be stacked, along with the number of
-- warps already in a stack.

--------------------------------------------------------------------------------
-- THIS FILE IS INTENDED FOR TESTING ONLY!  IT IS NOT USED BY stacktool.
-- CHANGES SHOULD BE MADE TO stacktool_definebyquery_part1.sql
-- AND stacktool_definebyquery_part2.sql
--------------------------------------------------------------------------------

-- stacktool_definebyquery_part1.sql
SELECT
    skycell_id,
    tess_id,
    filter,
    num_warp,
    MAX(num_stack) AS num_stack
FROM ((
    -- Number of stack-ready warps as a function of skycell and filter
    SELECT
        skycell_id,
        tess_id,
        filter,
        COUNT(warpSkyfile.skycell_id) AS num_warp -- number of warps that can be stacked
    FROM warpRun
        JOIN warpSkyfile USING(warp_id)
        JOIN fakeRun USING(fake_id)
        JOIN camRun USING(cam_id)
        JOIN chipRun USING(chip_id)
        JOIN rawExp USING(exp_id)
    WHERE
        warpRun.state = 'full'
    AND warpSkyfile.quality = 0
    AND warpSkyfile.fault = 0
    -- Any additional selection on warps/exposures goes here
-- stacktool_definebyquery_part2.sql
    GROUP BY
        skycell_id,
        filter
        ) AS warpsToStack
        LEFT JOIN (
    -- Number of stack inputs as a function of skycell and filter
    SELECT
        skycell_id,
        tess_id,
        filter,
        COUNT(stackInputSkyfile.warp_id) as num_stack -- number of warps in a stack
    FROM stackRun
        JOIN stackInputSkyfile USING(stack_id)
    GROUP BY
        stack_id
        ) AS stackSizes
    -- JOINing the warpsToStack and stackSizes tables
        USING(skycell_id, filter)
        )
    GROUP BY
        skycell_id,
        filter
;
