UPDATE pstampDependent
    JOIN pstampJob USING(dep_id)
    JOIN pstampRequest USING(req_id)
SET pstampDependent.fault = 0
-- fault count hook %s
WHERE pstampRequest.state ='run'
    AND pstampJob.state ='run'
    AND pstampDependent.state = 'new'
    AND (pstampDependent.fault > 0)
