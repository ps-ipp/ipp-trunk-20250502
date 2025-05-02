-- this query is used to find potental rawExps to be queued for vptool
-- processeing
SELECT DISTINCT 
    rawExp.*,
    newExp.label
FROM rawExp
JOIN newExp using (exp_id)
JOIN rawImfile using(exp_id)
LEFT JOIN vpRun USING (exp_id)
WHERE
    rawExp.fault = 0
    AND rawImfile.video_cells
