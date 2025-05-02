-- Get list of warp-stack pairs that can be diffed
-- and check results against existing diffs
SELECT
   exp_id,
   warp_id,
   filter,
   warpLabel,
   warpDataGroup,
   Inputs.tess_id,
   Inputs.skycell_id,
   stack_id,
   stackLabel,
   stackDataGroup,
   diff_id,
   diffLabel
FROM
(
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
   stackRun.data_group AS stackDataGroup
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
) AS Inputs
LEFT JOIN 
(
SELECT 
diff_id,
diffRun.label AS diffLabel,
warp1,
stack2,
skycell_id,
diffInputSkyfile.tess_id
FROM 
diffRun JOIN diffInputSkyfile USING(diff_id,tess_id)
WHERE
diff_mode = 2
-- %s 
) AS Diffs ON
       (warp_id = warp1 AND
        stack_id = stack2 AND
	Diffs.skycell_id = Inputs.skycell_id AND
	Diffs.tess_id = Inputs.tess_id)
WHERE
1
-- %s

