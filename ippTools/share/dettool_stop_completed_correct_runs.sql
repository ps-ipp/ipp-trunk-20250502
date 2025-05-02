-- det1 is the 'correction' detRun, det2 is the 'parent' detRun 
UPDATE detRun
SET state = 'stop'
WHERE det_id = (SELECT det_id FROM
    (SELECT
        det1.det_id,
        imfile.class_id,
        detRegisteredImfile.class_id as reg_class_id
    FROM detRun AS det1
    INNER JOIN detRun as det2
        ON det1.ref_det_id = det2.det_id 
	-- do we need to restrict by iteration?  not clear this logic below is right or tested
    JOIN detNormalizedImfile as imfile
        ON det2.det_id = imfile.det_id
        AND det2.iteration = imfile.iteration
    LEFT JOIN detRegisteredImfile
        ON det1.det_id = detRegisteredImfile.det_id
        AND det1.iteration = detRegisteredImfile.iteration
        AND imfile.class_id = detRegisteredImfile.class_id
    WHERE 
        det1.state = 'run'
        AND det1.mode = 'correction'
        AND det2.state = 'stop'
    GROUP BY
        det1.det_id
    HAVING
        COUNT(imfile.class_id) = COUNT(reg_class_id)
        AND SUM(imfile.fault) = 0
    ) as Foo
)
