SELECT
    exp_id,
    warp_id,
    rawExp.filter,
    warpRun.label,
    warpRun.tess_id,
    warpRun.data_group,
    COUNT(skycell_id) as skycell_count
FROM warpRun
JOIN warpSkyfile USING(warp_id)
JOIN fakeRun USING(fake_id)
JOIN camRun USING(cam_id)
JOIN chipRun USING(chip_id)
JOIN rawExp USING(exp_id)
LEFT JOIN (
    SELECT DISTINCT
        diffRun.*,
        warp_id,
        exp_id
    FROM diffRun
    JOIN diffInputSkyfile USING(diff_id)
    JOIN warpRun
        ON warpRun.warp_id = diffInputSkyfile.warp1
    JOIN fakeRun USING(fake_id)
    JOIN camRun USING(cam_id)
    JOIN chipRun USING(chip_id)
    WHERE warp1 IS NOT NULL
        AND warpRun.state = 'full'
    -- warp where hook %s
) AS diffExp USING(exp_id, warp_id)
WHERE
    warpRun.state = 'full'
    AND warpSkyfile.fault = 0
    AND warpSkyfile.quality = 0
    -- warp where hook %s
    -- exp where hook %s
    -- diff where hook %s
GROUP BY exp_id, warp_id DESC
