SELECT DISTINCT
    fullForceSummary.*,
    fullForceRun.state,
    fullForceRun.label,
    fullForceRun.data_group,
    fullForceRun.skycal_id,
    fullForceRun.workdir,
    fullForceRun.sources_path_base,
    'summary' AS dist_component,
    stackRun.stack_id,
    stackRun.tess_id,
    stackRun.skycell_id,
    stackRun.filter,
    rawExp.camera
FROM fullForceRun
    JOIN fullForceSummary USING(ff_id)
    JOIN fullForceInput USING(ff_id)
    JOIN skycalRun using(skycal_id)
    JOIN stackRun using(stack_id)
    JOIN warpRun using(warp_id, tess_id)
    JOIN warpSkyfile USING(warp_id, tess_id, skycell_id)
    JOIN fakeRun using(fake_id)
    JOIN camRun using(cam_id)
    JOIN chipRun using(chip_id)
    JOIN rawExp using(exp_id)
