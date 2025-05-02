-- a det_run + iteration is ready for normstat when: all detResidImfile entries for that iteration corresponding to all detProcessedImfile entries are available 
SELECT 
  detRun.det_id, 
  detRun.det_type, 
  detRun.iteration, 
  detRun.camera, 
  detRun.workdir, 
  detProcessedImfile.class_id,
  COUNT(detProcessedImfile.class_id),
  COUNT(detResidImfile.class_id)
FROM detRun
JOIN detInputExp
    USING (det_id, iteration)
JOIN detProcessedImfile
    ON  detProcessedImfile.det_id    = detRun.det_id
    AND detProcessedImfile.exp_id    = detInputExp.exp_id
LEFT JOIN detResidImfile
    ON  detResidImfile.det_id    = detProcessedImfile.det_id
    AND detResidImfile.exp_id    = detProcessedImfile.exp_id
    AND detResidImfile.class_id  = detProcessedImfile.class_id
    AND detResidImfile.iteration = detRun.iteration
LEFT JOIN detNormalizedStatImfile
    ON  detNormalizedStatImfile.det_id    = detRun.det_id    
    AND detNormalizedStatImfile.iteration = detRun.iteration 
    AND detNormalizedStatImfile.class_id  = detProcessedImfile.class_id
WHERE
    detRun.state = 'run'
    AND detRun.mode = 'master'
    AND detNormalizedStatImfile.det_id IS NULL
    AND detNormalizedStatImfile.iteration IS NULL
    AND detNormalizedStatImfile.class_id IS NULL
GROUP BY
    detRun.iteration,
    detRun.det_id
HAVING
    COUNT(detProcessedImfile.class_id) = COUNT(detResidImfile.class_id)
    AND SUM(detResidImfile.fault) = 0
