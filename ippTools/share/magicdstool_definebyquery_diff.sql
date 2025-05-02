SELECT *
FROM (
SELECT
    magicRun.magic_id,
    exp_id,
    'diff' AS stage,
    diff_id AS stage_id,
    diffRun.data_group,
    0 AS cam_id,
    magicRun.label,
    magicRun.workdir,
    CAST(NULL AS SIGNED) AS inv_magic_id,
    CAST(NULL AS SIGNED) AS inv_exp_id
FROM magicRun
    JOIN magicMask USING(magic_id)
    JOIN diffRun USING(diff_id)
    LEFT JOIN magicDSRun ON magicRun.magic_id = magicDSRun.magic_id
                         AND magicDSRun.stage  = 'diff'
WHERE magicRun.state = 'full'
    AND ( -- rerun HOOK magicdstool sends "\n1 " if rerun else "\n0 " %s
        OR magicDSRun.magic_ds_id IS NULL)
    AND diffRun.bothways = 0
    AND diffRun.magicked  = 0
    AND diffRun.state = 'full'
    -- WHERE hook %s
UNION
SELECT
    forwardRun.magic_id,
    forwardRun.exp_id,
    'diff' AS stage,
    forwardRun.diff_id AS stage_id,
    forwardRun.data_group,
    0 AS cam_id,
    forwardRun.label,
    forwardRun.workdir,
    inverseRun.magic_id AS inv_magic_id,
    inverseRun.exp_id AS inv_exp_id
FROM (
    SELECT
        magicRun.magic_id,
        exp_id,
        diff_id,
        diffRun.data_group,
        magicRun.label,
        magicRun.workdir
    FROM magicRun
        JOIN magicMask USING(magic_id)
        JOIN diffRun USING(diff_id)
        LEFT JOIN magicDSRun ON magicRun.magic_id = magicDSRun.magic_id
                             AND magicDSRun.stage  = 'diff'
    WHERE magicRun.state = 'full'
        AND magicRun.inverse = 0
        AND ( -- rerun HOOK magicdstool sends "\n1 " if rerun else "\n0 " %s
            OR magicDSRun.magic_ds_id IS NULL)
        AND diffRun.state = 'full'
        AND diffRun.bothways
        AND diffRun.magicked  = 0
        -- WHERE hook %s
) AS forwardRun
JOIN (
    SELECT
        magic_id,
        exp_id,
        diff_id
    FROM magicRun
        JOIN magicMask USING(magic_id)
    WHERE magicRun.state = 'full'
        AND magicRun.inverse = 1
) AS inverseRun USING(diff_id)

) AS Foo
