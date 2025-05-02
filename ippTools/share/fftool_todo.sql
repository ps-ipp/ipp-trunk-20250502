SELECT
    fullForceRun.*,
    fullForceInput.warp_id,
    stackRun.tess_id,
    stackRun.skycell_id,
    stackRun.filter,
    rawExp.camera,
    warpSkyfile.path_base AS warp_path_base
FROM fullForceRun
    JOIN fullForceInput USING(ff_id)
    JOIN skycalRun USING(skycal_id)
    JOIN stackRun USING(stack_id)
    JOIN warpRun USING(warp_id, tess_id)
    JOIN warpSkyfile USING(warp_id, tess_id, skycell_id)
    JOIN fakeRun USING(fake_id)
    JOIN camRun USING(cam_id)
    JOIN chipRun USING(chip_id)
    JOIN rawExp USING(exp_id)
    LEFT JOIN fullForceResult USING(ff_id, warp_id)
WHERE fullForceRun.state = 'new'
    AND fullForceResult.ff_id IS NULL
    AND warpSkyfile.data_state = 'full'
