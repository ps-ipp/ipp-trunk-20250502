SELECT DISTINCT
   detRun.det_type,
   detRun.workdir,
   rawExp.camera,
   detStackedImfile.uri,
   detNormalizedStatImfile.*
 FROM detRun
 JOIN detStackedImfile
   USING(det_id, iteration)
 JOIN detInputExp
   USING(det_id, iteration)
 JOIN rawExp
   ON detInputExp.exp_id = rawExp.exp_id
 JOIN detNormalizedStatImfile
   ON detStackedImfile.det_id = detNormalizedStatImfile.det_id
   AND detStackedImfile.iteration = detNormalizedStatImfile.iteration
   AND detStackedImfile.class_id = detNormalizedStatImfile.class_id
 LEFT JOIN detNormalizedImfile
   ON detNormalizedStatImfile.det_id = detNormalizedImfile.det_id
   AND detNormalizedStatImfile.iteration = detNormalizedImfile.iteration
   AND detNormalizedStatImfile.class_id = detNormalizedImfile.class_id
 WHERE
   detNormalizedImfile.det_id IS NULL
   AND detNormalizedImfile.iteration IS NULL
   AND detNormalizedImfile.class_id IS NULL
   AND detNormalizedStatImfile.fault = 0
   AND detRun.state = 'run'
