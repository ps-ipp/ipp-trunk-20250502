SELECT DISTINCT
   detRunSummary.*,
   detRun.det_type,
   detRun.mode
 FROM detRun
 JOIN detRunSummary
   USING(det_id, iteration)
 WHERE
   detRun.state = 'run'
