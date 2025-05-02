SELECT
    pstampJob.*,
    pstampRequest.name,
    pstampRequest.outProduct
FROM pstampJob
    JOIN pstampRequest USING(req_id)
