SELECT relExp.relexp_id
FROM relGroup 
    JOIN lapRun USING(lap_id)
    JOIN lapExp USING(lap_id)
    JOIN relExp using(rel_id, exp_id)
WHERE relExp.group_id = 0
