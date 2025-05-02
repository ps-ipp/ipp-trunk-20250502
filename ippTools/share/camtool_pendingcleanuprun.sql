SELECT
    camRun.cam_id,
    rawExp.camera,
    rawExp.exp_tag,
    camRun.workdir,
    camRun.state
FROM camRun
JOIN chipRun
USING (chip_id)
JOIN rawExp 
USING (exp_id)
WHERE
    (camRun.state = 'goto_cleaned' OR camRun.state = 'goto_purged' OR camRun.state = 'goto_scrubbed')
