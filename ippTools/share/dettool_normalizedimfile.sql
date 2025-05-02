SELECT
 detNormalizedImfile.*
 FROM detNormalizedImfile
 JOIN detRun
   USING(det_id, iteration)
 WHERE
   detRun.state = 'run'
   AND detRun.mode = 'master'
