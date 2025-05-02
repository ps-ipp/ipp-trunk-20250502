-- the output of this query must match the format of chipRun row
SELECT DISTINCT
    chip_id,
    exp_id,
    state,
    workdir,
    workdir_state,
    label,
    data_group,
    dist_group,
    reduction,
    expgroup,
    dvodb,
    tess_id,
    end_stage,
    imfile_magicked as magicked,
    0 as update_mode,
    note
FROM
    (SELECT
        chipRun.*,
        rawImfile.class_id as rawimfile_class_id,
        chipProcessedImfile.class_id,
        -- XXX using chipProcessedImfile assumes that all imfile's have
        -- the same magicked value if that isn't right then more than one 
        -- row will be returned. In practice this is the case but bugs could
        -- cause it to not be true.
        -- We could use rawExp.magicked but that would make it possible for
        -- the chipRun to have a different magicked value than the imfiles
        chipProcessedImfile.magicked AS imfile_magicked
    FROM chipRun
    JOIN rawImfile
        ON chipRun.exp_id = rawImfile.exp_id
        AND rawImfile.ignored = 0
    LEFT JOIN chipProcessedImfile
        ON chipRun.chip_id = chipProcessedImfile.chip_id
        AND rawImfile.exp_id = chipProcessedImfile.exp_id
        AND rawImfile.class_id = chipProcessedImfile.class_id
    WHERE
        chipRun.state = 'new'
    GROUP BY
        chipRun.chip_id,
        chipRun.exp_id
    HAVING
        COUNT(rawImfile.class_id) = COUNT(chipProcessedImfile.class_id)
        AND SUM(chipProcessedImfile.fault) = 0
        AND SUM(chipProcessedImfile.quality > 0) != COUNT(*)
    ) as Foo
