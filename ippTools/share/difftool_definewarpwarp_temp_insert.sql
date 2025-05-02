-- Get list of exposures that have diffs
-- Only interested in whole exposures (diffRun.exposure = 1)
INSERT INTO diffs
SELECT DISTINCT
    diffWarps.diff_id,
    exp_id
FROM (
    -- Forward diffs
    SELECT
        diffRun.diff_id,
        warp1 AS warp_id
    FROM diffRun
    JOIN diffInputSkyfile USING(diff_id)
    WHERE diffInputSkyfile.warp1 IS NOT NULL
        AND diffRun.exposure = 1
    -- WHERE hook %s
    UNION
    -- Backward diffs
    SELECT
        diffRun.diff_id,
        warp2 AS warp_id
    FROM diffRun
    JOIN diffInputSkyfile USING(diff_id)
    WHERE diffInputSkyfile.warp2 IS NOT NULL
        AND diffRun.exposure = 1
        AND diffRun.bothways = 1
    -- WHERE hook %s
    ) AS diffWarps
JOIN warpRun USING(warp_id)
JOIN fakeRun USING(fake_id)
JOIN camRun USING(cam_id)
JOIN chipRun USING(chip_id)
