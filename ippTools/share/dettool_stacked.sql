-- select detStackedImfile.*
-- by:
-- where det_id, iteration, class_id is not in detNormalizedImfile

SELECT
   detStackedImfile.*
 FROM detStackedImfile
 JOIN detRun
   USING(det_id, iteration)
 WHERE
   detRun.state = 'run'
   AND detRun.mode = 'master'
