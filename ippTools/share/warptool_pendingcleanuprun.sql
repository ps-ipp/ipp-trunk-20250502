SELECT
    warpRun.warp_id,
    warpRun.workdir,
    rawExp.camera,
    rawExp.exp_tag,
    warpRun.state
FROM warpRun
JOIN fakeRun
USING (fake_id)
JOIN camRun
USING (cam_id)
JOIN chipRun
USING (chip_id)
JOIN rawExp 
USING (exp_id)
WHERE
    (warpRun.state = 'goto_cleaned' OR warpRun.state = 'goto_scrubbed' OR warpRun.state = 'goto_purged')
