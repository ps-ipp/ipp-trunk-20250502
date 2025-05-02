-- this query is used to find potental rawExps to be queued for chiptool
-- processeing
SELECT DISTINCT 
    rawExp.*,
    newExp.label
FROM rawExp
JOIN newExp using (exp_id)
LEFT JOIN chipRun ON chipRun.exp_id = rawExp.exp_id -- JOIN hook %s
WHERE
    rawExp.fault = 0
