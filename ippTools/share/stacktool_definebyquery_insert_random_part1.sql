-- This is the part 1 of 2 of a query to use a random set of warps for
-- the inputs to a defined stack.
-- stacktool_definebyquery_insert_random_part2.sql should be appended.
INSERT INTO
        stackInputSkyfile(stack_id, warp_id)
SELECT
        @STACK_ID@, -- This should be replaced with the stack_id
        warp_id
FROM (
    -- Sub-select to get the random set of warps
    SELECT
            warpSkyfile.*,
            rand() AS rnd_num
    FROM warpSkyfile
    JOIN warpRun
        USING(warp_id)
    JOIN fakeRun
        USING(fake_id)
    JOIN camRun
        USING(cam_id)
    JOIN camProcessedExp
        USING(cam_id)
    JOIN chipRun
        USING(chip_id)
    JOIN rawExp
        USING(exp_id)
    WHERE
        skycell_id = '@SKYCELL_ID@'
        AND warpRun.state = 'full'
        AND rawExp.filter = '@FILTER@' -- the result of the query is grouped by filter and inserted for one at a time
        AND warpSkyfile.fault = 0
        AND warpSkyfile.quality = 0
-- Put additional constraints here
-- stacktool_definebyquery_insert_random_part2.sql goes here
