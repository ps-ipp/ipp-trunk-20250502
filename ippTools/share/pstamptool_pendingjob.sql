SELECT * FROM
(SELECT pstampJob.*,
    IFNULL(Label.priority, 10000) AS priority,
    0 AS myParent,
    0 AS runningChildren
FROM pstampJob
    JOIN pstampRequest USING(req_id)
    LEFT JOIN pstampDependent USING(dep_id)
    LEFT JOIN Label ON pstampRequest.label = Label.label
WHERE pstampRequest.state = 'run'
    AND pstampRequest.fault = 0
    AND pstampJob.state = 'run'
    AND pstampJob.fault = 0
    AND NOT is_parent
    AND (dep_id = 0 OR pstampDependent.state = 'full')
    AND (Label.active OR Label.active IS NULL)
    -- where hook 1 %s
UNION
SELECT * FROM
  (SELECT pstampJob.*,
        IFNULL(Label.priority, 10000) AS priority
    FROM pstampJob
        JOIN pstampRequest USING(req_id)
        LEFT JOIN pstampDependent USING(dep_id)
        LEFT JOIN Label ON pstampRequest.label = Label.label
    WHERE pstampRequest.state = 'run'
        AND pstampRequest.fault = 0
        AND pstampJob.state = 'run'
        AND pstampJob.fault = 0
        AND is_parent
        AND (dep_id = 0 OR pstampDependent.state = 'full')
        AND (Label.active OR Label.active IS NULL)
        -- where hook 2 %s
    ) as parentJob
    LEFT JOIN
    (SELECT parent_id AS myParent,
            count(job_id) as runningChildren
        FROM pstampRequest JOIN pstampJob USING(req_id)
        WHERE pstampRequest.state ='run'
            AND pstampJob.state = 'run'
            AND pstampJob.parent_id
        GROUP BY parent_id
    ) as childJobs
    ON childJobs.myParent = parentJob.job_id
    WHERE runningChildren IS NULL
) AS theQuery


