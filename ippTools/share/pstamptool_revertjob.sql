UPDATE pstampJob 
    JOIN pstampRequest USING(req_id)
SET pstampJob.fault = 0
 -- clear fault count clause goes here %s
WHERE  pstampRequest.state = 'run'
    AND pstampJob.state = 'run'
    -- fault clause goes here %s
