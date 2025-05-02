-- Get a list of exposures on which magic may be performed
SELECT DISTINCT
    chipRun.exp_id,
    MAX(diffWarps.diff_id) AS diff_id,
    -- The following trick pulls out the 'inverse' value for the maximum diff_id
    CONVERT(SUBSTRING_INDEX(GROUP_CONCAT(diffWarps.inverse ORDER BY diffWarps.diff_id DESC), ',', 1), UNSIGNED) AS inverse,
    diff_data_group
FROM (
    -- Forward diffs
    SELECT
        diffRun.diff_id,
        warp1 AS warp_id,
        0 AS inverse,
        diffRun.data_group AS diff_data_group
    FROM diffRun
    JOIN diffInputSkyfile USING(diff_id)
    JOIN diffSkyfile USING(diff_id, skycell_id)
    WHERE diffInputSkyfile.warp1 IS NOT NULL
        AND diffRun.exposure = 1
        AND diffRun.magicked = 0
        AND diffSkyfile.quality = 0
    -- diff WHERE hook %s
    UNION
    -- Backward diffs
    SELECT
        diffRun.diff_id,
        warp2 AS warp_id,
        1 AS inverse,
        diffRun.data_group AS diff_data_group
    FROM diffRun
    JOIN diffInputSkyfile USING(diff_id)
    JOIN diffSkyfile USING(diff_id, skycell_id)
    WHERE diffInputSkyfile.warp2 IS NOT NULL
        AND diffRun.exposure = 1
        AND diffRun.bothways = 1
        AND diffRun.magicked = 0
        AND diffSkyfile.quality = 0
    -- diff WHERE hook %s
    ) AS diffWarps
JOIN warpRun USING(warp_id)
JOIN fakeRun USING(fake_id)
JOIN camRun USING(cam_id)
JOIN chipRun USING(chip_id)
JOIN rawExp USING(exp_id)
LEFT JOIN
    (SELECT magic_id, exp_id, label
        FROM magicRun
        -- rerun hook %s
    ) AS oldMagicRun
    ON oldMagicRun.exp_id = chipRun.exp_id
-- WHERE hook %s
GROUP BY chipRun.exp_id
