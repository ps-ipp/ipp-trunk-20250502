-- dettool -childlessrun

SELECT DISTINCT
   detRun.*
 FROM detRun
 LEFT JOIN detRun as foo
   ON foo.ref_det_id = detRun.det_id
-- XXX do we need to restrict to foo.ref_iter = detRun.iteration ?   
 WHERE
   detRun.state = 'stop'
   AND detRun.mode = 'master'
   AND foo.det_id IS NULL
