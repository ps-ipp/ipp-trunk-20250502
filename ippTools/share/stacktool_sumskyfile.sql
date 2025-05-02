SELECT
    stackSumSkyfile.*,
    stackRun.state,
    stackRun.tess_id,
    stackRun.skycell_id,
    stackRun.filter,
    stackRun.workdir,
    stackRun.label,
    (SELECT rawExp.camera FROM 
        stackInputSkyfile 
        JOIN warpRun USING(warp_id)
        JOIN fakeRun ON warpRun.fake_id = fakeRun.fake_id
        JOIN camRun ON camRun.cam_id = fakeRun.cam_id
        JOIN chipRun ON camRun.chip_id  = chipRun.chip_id
        JOIN rawExp ON chipRun.exp_id  = rawExp.exp_id
        where stack_id = stackRun.stack_id limit 1
    ) as camera
FROM stackRun
JOIN stackSumSkyfile
USING(stack_id)
