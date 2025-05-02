-- delete jobs that got added by an incomplete pstamp parser run
DELETE pstampJob
FROM pstampJob
    JOIN pstampRequest USING(req_id)
WHERE pstampRequest.state = 'new' 
    AND pstampRequest.fault > 0
