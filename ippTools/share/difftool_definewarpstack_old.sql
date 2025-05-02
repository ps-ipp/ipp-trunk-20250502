-- Get list of warp-stack pairs that can be diffed
-- and check results against existing diffs
SELECT
   exp_id,
   warp_id,
   rawExp.filter,
   warpRun.label AS warpLabel,
   warpRun.data_group AS warpDataGroup,
   warpRun.tess_id,
   warpSkyfile.skycell_id,
   MAX(stack_id) AS stack_id,
   stackRun.label AS stackLabel,
   stackRun.data_group AS stackDataGroup,
   diff_id,
   diffRun.label AS diffLabel
FROM warpRun
     JOIN fakeRun USING(fake_id)
     JOIN camRun USING(cam_id)
     JOIN chipRun USING(chip_id)
     JOIN rawExp USING(exp_id)
     JOIN warpSkyfile USING(warp_id)
     JOIN stackRun ON 
       (stackRun.skycell_id = warpSkyfile.skycell_id AND
        stackRun.filter     = rawExp.filter AND
	stackRun.tess_id    = warpRun.tess_id)
     JOIN stackSumSkyfile USING(stack_id)
     LEFT JOIN diffInputSkyfile ON 
       (warp_id = diffInputSkyfile.warp1 AND
        stack_id = diffInputSkyfile.stack2 AND
	diffInputSkyfile.skycell_id = stackRun.skycell_id AND
	diffInputSkyfile.tess_id = stackRun.tess_id)
     LEFT JOIN diffRun USING(diff_id)
WHERE 
     warpRun.state = 'full'
     AND stackRun.state = 'full'
     AND warpSkyfile.fault = 0
     AND warpSkyfile.quality = 0
     AND stackSumSkyfile.fault = 0
     AND stackSumSkyfile.quality = 0
     AND exp_id IS NOT NULL
-- %s
GROUP BY exp_id,warp_id,skycell_id
