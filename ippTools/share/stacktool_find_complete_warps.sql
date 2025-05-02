SELECT
    warpSkyfile.skycell_id,
    warpSkyfile.tess_id,
    stackRun.stack_id,
    COUNT(warpSkyfile.skycell_id) AS num_avail,
    COUNT(stackRun.stack_id) AS num_extant
FROM warpRun
JOIN warpSkyfile
    USING(warp_id)
JOIN fakeRun
    USING(fake_id)
JOIN camRun
    USING(cam_id)
JOIN chipRun
    USING(chip_id)
JOIN rawExp
    USING(exp_id)
LEFT JOIN stackInputSkyfile
    ON warpSkyfile.warp_id = stackInputSkyfile.warp_id
LEFT JOIN stackRun
    ON stackRun.skycell_id = warpSkyfile.skycell_id
    AND stackRun.stack_id = stackInputSkyfile.stack_id
WHERE
    warpRun.state = 'full'
    AND warpSkyfile.fault = 0
    AND warpSkyfile.quality = 0
GROUP BY
    warpSkyfile.skycell_id, stackRun.stack_id
HAVING
    num_avail > num_extant

-- It seems like we should be grouping the results here but in fact we don't
-- want to do that as each warp_id may contain multiple skycells.  So a warp_id
-- may end up in more than one stackRun (for different skycells)
--    GROUP BY
--        warp_id


INSERT INTO
        stackInputSkyfile(stack_id, warp_id)
SELECT
        13,
        warp_id
FROM warpSkyfile
WHERE
    skycell_id = 'new';
