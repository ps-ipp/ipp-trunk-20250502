-- stop jobs which have a faulted dependent
UPDATE pstampDependent
    JOIN pstampJob USING(dep_id)
    JOIN pstampRequest USING(req_id)
SET pstampJob.state = 'stop', 
    pstampJob.fault = %d,
    pstampDependent.fault = %d, 
    pstampDependent.state = '%s'
WHERE pstampJob.state = 'run'
    AND pstampDependent.state = 'new'
