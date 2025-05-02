SELECT
    chipRun.*
FROM chipRun
JOIN rawExp
    USING(exp_id)
