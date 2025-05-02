SELECT
    stackRun.stack_id,
    stackRun.tess_id,
    stackRun.skycell_id,
    stackRun.workdir,
    stackRun.reduction,
    stackRun.label,
    stackRun.state,
    stackSumSkyfile.path_base,
    rawExp.camera,
    IFNULL(Label.priority, 10000) AS priority
FROM stackRun
JOIN stackInputSkyfile USING(stack_id)
JOIN warpRun USING(warp_id)
JOIN warpSkyfile USING(warp_id,skycell_id)
JOIN fakeRun USING(fake_id)
JOIN camRun USING(cam_id)
JOIN chipRun USING(chip_id)
JOIN rawExp USING(exp_id)
LEFT JOIN stackSumSkyfile USING(stack_id)
LEFT JOIN Label ON Label.label = stackRun.label
WHERE
    ((stackRun.state = 'full' AND stackSumSkyfile.fault = 0 AND stackSumSkyfile.quality = 0 AND stackSumSkyfile.background_model != 1))
    AND (Label.active OR Label.active IS NULL)
    -- WHERE hook %s
GROUP BY stack_id
HAVING ((SUM(IF(warpRun.state = 'cleaned', 1, 0)) = COUNT(stackInputSkyfile.warp_id) OR
         SUM(IF(warpRun.state = 'full', 1, 0)) = COUNT(stackInputSkyfile.warp_id)) AND
        SUM(IF(warpSkyfile.background_model = 1, 1, 0)) = COUNT(stackInputSkyfile.warp_id))

