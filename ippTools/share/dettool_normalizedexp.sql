SELECT
   detNormalizedExp.*
 FROM detNormalizedExp
 JOIN detRun
   USING(det_id, iteration)
 WHERE
   detRun.state = 'run'
   AND detRun.mode = 'master'
