INSERT INTO duplicate
SELECT exp_id FROM
    (
        SELECT
        MAX(exp_id) AS exp_id,
--            tmp_exp_name,
--            tmp_camera,
--            tmp_telescope,
        count(exp_id) AS count
        FROM newExp
        GROUP BY tmp_exp_name, tmp_camera, tmp_telescope
        HAVING count > 1
    ) as Foo
