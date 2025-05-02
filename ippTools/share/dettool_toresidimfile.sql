-- which returns a list of processed imfiles (with corresponding detrend id,
-- iteration, class id, uri, type) for which the appropriate stacked imfile has
-- been normalised, and which have not been masked out by detResidImfileAnalysis

SELECT DISTINCT
    detRun.det_id,
    detRun.iteration,
    detRun.det_type,
    detRun.mode,
    detRun.workdir,
    detRun.reduction,
    detProcessedImfile.exp_id,
    detProcessedImfile.class_id,
    detProcessedImfile.uri,
--  detNormalizedImfile.uri AS det_uri,
    detStackedImfile.uri AS det_uri,
    detRun.det_id AS ref_det_id,
    detRun.iteration AS ref_iter,
    rawExp.camera,
    rawExp.exp_tag
FROM detRun
JOIN detInputExp
--  USING(det_id, iteration)
    ON detInputExp.det_id = detRun.det_id
    AND detInputExp.iteration = detRun.iteration
--  AND detInputExp.iteration = 0
JOIN rawExp
    ON detInputExp.exp_id = rawExp.exp_id
JOIN detProcessedImfile
    ON detRun.det_id = detProcessedImfile.det_id
    AND detInputExp.exp_id = detProcessedImfile.exp_id
JOIN detStackedImfile
    ON detRun.det_id = detStackedImfile.det_id
    AND detRun.iteration = detStackedImfile.iteration
    AND detProcessedImfile.class_id = detStackedImfile.class_id
-- EAM : replacing detNormalizedImfile with detStackedImfile to change the sequencing
-- JOIN detNormalizedImfile
--     ON detRun.det_id = detNormalizedImfile.det_id
--     AND detRun.iteration = detNormalizedImfile.iteration
--     AND detProcessedImfile.class_id = detNormalizedImfile.class_id
-- EAM : we there is no reason to wait for all stacks to complete before continuing
-- JOIN detNormalizedExp
--     ON detRun.det_id = detNormalizedExp.det_id
--     AND detRun.iteration = detNormalizedExp.iteration
LEFT JOIN detResidImfile
    ON detRun.det_id = detResidImfile.det_id
    AND detRun.iteration = detResidImfile.iteration
    AND detProcessedImfile.exp_id = detResidImfile.exp_id
    AND detProcessedImfile.class_id = detResidImfile.class_id
WHERE
    detRun.state = 'run'
    AND detRun.mode = 'master'
    AND detStackedImfile.fault = 0
--  AND detNormalizedImfile.fault = 0
--  AND detNormalizedExp.fault = 0
    AND detResidImfile.det_id IS NULL
    AND detResidImfile.iteration IS NULL
    AND detResidImfile.exp_id IS NULL
    AND detResidImfile.class_id IS NULL
UNION
SELECT
    detRun.det_id,
    detRun.iteration,
    detRun.det_type,
    detRun.mode,
    detRun.workdir,
    detRun.reduction,
    detProcessedImfile.exp_id,
    detProcessedImfile.class_id,
    detProcessedImfile.uri,
    detNormalizedImfile.uri AS det_uri,
    detRun.ref_det_id,
    detRun.ref_iter,
    rawExp.camera,
    rawExp.exp_tag
FROM detRun
JOIN detRun AS detRunRef
  ON detRunRef.det_id = detRun.ref_det_id
JOIN detInputExp
  ON detInputExp.det_id    = detRun.det_id
 AND detInputExp.iteration = detRun.iteration
-- AND detInputExp.iteration = 0
JOIN rawExp
  ON rawExp.exp_id = detInputExp.exp_id
JOIN detProcessedImfile
  ON detProcessedImfile.det_id    = detRun.det_id
 AND detProcessedImfile.exp_id    = detInputExp.exp_id
JOIN detNormalizedImfile
  ON detNormalizedImfile.det_id    = detRun.ref_det_id
 AND detNormalizedImfile.iteration = detRun.ref_iter
 AND detNormalizedImfile.class_id  = detProcessedImfile.class_id
LEFT JOIN detResidImfile
  ON detResidImfile.det_id    = detRun.det_id      
 AND detResidImfile.iteration = detRun.iteration   
 AND detResidImfile.exp_id    = detProcessedImfile.exp_id   
 AND detResidImfile.class_id  = detProcessedImfile.class_id 
WHERE
 detRun.state = 'run'
 AND detRun.mode = 'verify'
 AND detRunRef.state = 'stop'
 AND detNormalizedImfile.fault = 0
 AND detProcessedImfile.fault = 0
 AND detResidImfile.det_id IS NULL
 AND detResidImfile.iteration IS NULL
 AND detResidImfile.exp_id IS NULL
 AND detResidImfile.class_id IS NULL
UNION
SELECT
    detRun.det_id,
    detRun.iteration,
    detRun.det_type,
    detRun.mode,
    detRun.workdir,
    detRun.reduction,
    detProcessedImfile.exp_id,
    detProcessedImfile.class_id,
    detProcessedImfile.uri,
    detRegisteredImfile.uri AS det_uri,
    detRun.ref_det_id,
    detRun.ref_iter,
    rawExp.camera,
    rawExp.exp_tag
FROM detRun
JOIN detRun AS detRunRef
  ON detRunRef.det_id = detRun.ref_det_id
JOIN detInputExp
  ON detInputExp.det_id    = detRun.det_id
 AND detInputExp.iteration = detRun.iteration
-- AND detInputExp.iteration = 0
JOIN rawExp
  ON rawExp.exp_id = detInputExp.exp_id
JOIN detProcessedImfile
  ON detProcessedImfile.det_id    = detRun.det_id
 AND detProcessedImfile.exp_id    = detInputExp.exp_id
JOIN detRegisteredImfile
  ON detRegisteredImfile.det_id    = detRun.ref_det_id
 AND detRegisteredImfile.iteration = detRun.ref_iter
 AND detRegisteredImfile.class_id  = detProcessedImfile.class_id
LEFT JOIN detResidImfile
  ON detResidImfile.det_id    = detRun.det_id      
 AND detResidImfile.iteration = detRun.iteration   
 AND detResidImfile.exp_id    = detProcessedImfile.exp_id   
 AND detResidImfile.class_id  = detProcessedImfile.class_id 
WHERE
 detRun.state = 'run'
 AND detRun.mode = 'verify'
 AND detRunRef.state = 'stop'
 AND detRegisteredImfile.fault = 0
 AND detProcessedImfile.fault = 0
 AND detResidImfile.det_id IS NULL
 AND detResidImfile.iteration IS NULL
 AND detResidImfile.exp_id IS NULL
 AND detResidImfile.class_id IS NULL
ORDER BY exp_id
