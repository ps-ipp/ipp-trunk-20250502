SELECT
    fakeRun.fake_id,
    rawExp.camera,
    fakeRun.state
FROM fakeRun
JOIN camRun
USING (cam_id)
JOIN chipRun
USING (chip_id)
JOIN rawExp 
USING (exp_id)
WHERE
    (fakeRun.state = 'goto_cleaned' OR fakeRun.state = 'goto_scrubbed' OR fakeRun.state = 'goto_purged')

