SELECT pstampRequest.* ,
    IFNULL(Label.priority, 0) AS priority
FROM pstampRequest 
    LEFT JOIN Label USING(label) 
WHERE state = 'run'
    AND pstampRequest.fault = 0
    AND (
    	SELECT count(*) FROM pstampJob 
	WHERE pstampJob.req_id = pstampRequest.req_id
		AND pstampJob.state != 'stop' 
                AND pstampJob.state != 'cancel'
	) = 0
