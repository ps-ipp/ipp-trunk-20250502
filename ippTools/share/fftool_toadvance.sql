SELECT
    fullForceRun.*,
    stackRun.tess_id,
    stackRun.skycell_id,
    stackRun.filter,
    rawExp.camera
FROM fullForceRun
    JOIN fullForceInput USING(ff_id)
    JOIN skycalRun USING(skycal_id)
    JOIN stackRun USING(stack_id)
    JOIN warpRun USING(warp_id, tess_id)
    JOIN fakeRun USING(fake_id)
    JOIN camRun USING(cam_id)
    JOIN chipRun USING(chip_id)
    JOIN rawExp USING(exp_id)
    LEFT JOIN fullForceResult USING(ff_id, warp_id)
    LEFT JOIN fullForceSummary USING(ff_id)
WHERE fullForceRun.state = 'new' AND fullForceSummary.ff_id IS NULL
    -- WHERE hook %s
    GROUP BY ff_id
    HAVING COUNT(fullForceInput.warp_id) = COUNT(fullForceResult.warp_id)
        AND SUM(fullForceResult.fault) = 0
