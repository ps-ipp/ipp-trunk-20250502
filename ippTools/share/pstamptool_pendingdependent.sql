SELECT DISTINCT
    pstampDependent.*,
    pstampRequest.label,
    MIN(req_id) as first_req_id,
    concat( pstampDependent.outdir, '/checkdep.', dep_id, '.log') as logfile,
    IFNULL(Label.priority, 10000) AS priority
FROM pstampDependent
JOIN pstampJob USING(dep_id)
JOIN pstampRequest USING(req_id)
LEFT JOIN Label ON pstampRequest.label = Label.label
WHERE pstampDependent.state = 'new'
    AND pstampJob.state = 'run'
    AND pstampRequest.state = 'run'
    AND (Label.active OR Label.active IS NULL)
