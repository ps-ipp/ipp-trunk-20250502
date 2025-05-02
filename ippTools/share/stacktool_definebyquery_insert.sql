-- Define the inputs to a defined stack
INSERT INTO
        stackInputSkyfile(stack_id, warp_id)
SELECT
        @STACK_ID@, -- This should be replaced with the stack_id
        warpRun.warp_id
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
    warpSkyfile.skycell_id = '@SKYCELL_ID@'
    AND warpRun.state = 'full'
    AND rawExp.filter = '@FILTER@'
    AND warpSkyfile.fault = 0
    AND warpSkyfile.quality = 0
