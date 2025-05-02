SELECT
    camProcessedExp.*,
    camRun.state,
    camRun.workdir,
    camRun.label,
    camRun.reduction,
    camRun.expgroup,
    camRun.dvodb,
    camRun.tess_id,
    camRun.end_stage
FROM camRun
JOIN camProcessedExp
    USING(cam_id)
WHERE
    (camRun.state = 'goto_cleaned' OR camRun.state = 'goto_purged' OR camRun.state = 'goto_scrubbed')

