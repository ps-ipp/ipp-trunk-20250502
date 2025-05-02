SELECT
    addRun.add_id,
    rawExp.camera,
    addRun.state
FROM addRun
JOIN camRun
USING (cam_id)
JOIN chipRun
USING (chip_id)
JOIN rawExp 
USING (exp_id)
WHERE
    (addRun.state = 'goto_cleaned' OR addRun.state = 'goto_purged' OR addRun.state = 'goto_scrubbed')
