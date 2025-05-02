SELECT
    fullForceResult.*,
    fullForceRun.state,
    fullForceRun.label,
    fullForceRun.data_group,
    fullForceRun.skycal_id,
    fullForceRun.workdir,
    fullForceRun.sources_path_base,
    CONCAT_WS('.', warp_id, skycell_id) AS dist_component,
    stackRun.tess_id,
    stackRun.skycell_id,
    stackRun.filter,
    warpImfile.warp_skyfile_id,
    rawExp.exp_id,
    rawExp.exp_name,
    rawExp.camera
FROM fullForceRun
    JOIN fullForceResult USING(ff_id)
    JOIN skycalRun using(skycal_id)
    JOIN stackRun using(stack_id)
    JOIN warpRun using(warp_id, tess_id)
    JOIN warpSkyfile USING(warp_id, tess_id, skycell_id)
    JOIN warpImfile USING(warp_id, skycell_id)
    JOIN fakeRun using(fake_id)
    JOIN camRun using(cam_id)
    JOIN chipRun using(chip_id)
    JOIN rawExp using(exp_id)
