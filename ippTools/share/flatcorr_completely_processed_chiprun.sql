INSERT INTO chipRunDone
SELECT
    chip_id,
    exp_id
FROM (SELECT DISTINCT
        chip_id,
        exp_id,
        state,
        workdir,
        workdir_state,
        label,
        reduction,
        expgroup,
        dvodb
    FROM
        (SELECT
            chipRun.*,
            rawImfile.class_id as rawimfile_class_id,
            chipProcessedImfile.class_id
        FROM chipRun
        JOIN rawImfile
            USING(exp_id)
        LEFT JOIN chipProcessedImfile
            ON chipRun.chip_id = chipProcessedImfile.chip_id
            AND rawImfile.exp_id = chipProcessedImfile.exp_id
            AND rawImfile.class_id = chipProcessedImfile.class_id
        WHERE
            chipRun.state = 'full'
        GROUP BY
            chipRun.chip_id,
            chipRun.exp_id
        HAVING
            COUNT(rawImfile.class_id) = COUNT(chipProcessedImfile.class_id)
            AND SUM(chipProcessedImfile.fault) = 0
            AND SUM(chipProcessedImfile.quality > 0) != COUNT(*)
       ) as Foo
    ) as Bar
