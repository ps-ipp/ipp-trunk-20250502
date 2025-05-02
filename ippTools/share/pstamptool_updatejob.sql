UPDATE pstampJob 
    JOIN pstampRequest USING(req_id) 
    LEFT JOIN pstampDependent USING(dep_id)
SET 
