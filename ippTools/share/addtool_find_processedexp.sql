SELECT
    addProcessedExp.*,
    addRun.workdir
FROM addProcessedExp
JOIN addRun
    USING(add_id)
JOIN camRun
    USING(cam_id)
JOIN chipRun
    USING(chip_id)
JOIN rawExp
    ON chipRun.exp_id = rawExp.exp_id
