SELECT
    pstampFile.*,
    pstampJob.req_id
FROM pstampFile
    JOIN pstampJob using(job_id)
