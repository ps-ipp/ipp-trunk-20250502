SELECT
   detRun.mode,
   detResidExp.*,
   detInputExp.include
 FROM detRun
 JOIN detInputExp
   USING(det_id, iteration)
 JOIN detResidExp
   USING(det_id, iteration, exp_id)
 WHERE
   detRun.state = 'run'
