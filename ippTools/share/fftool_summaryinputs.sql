SELECT
    fullForceResult.*
FROM fullForceRun 
    JOIN fullForceResult USING(ff_id)
