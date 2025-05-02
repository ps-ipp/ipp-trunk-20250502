SELECT DISTINCT
    skycalRun.*,
    stackRun.tess_id,
    stackRun.skycell_id,
    stackRun.filter,
    staticskyResult.path_base,
    rawExp.camera,
    staticskyResult.num_inputs = 1 AS singlefilter
FROM skycalRun
JOIN staticskyRun USING (sky_id)
JOIN staticskyResult USING(sky_id)
JOIN stackRun USING (stack_id)
JOIN stackInputSkyfile USING(stack_id)
JOIN warpRun USING(warp_id)
JOIN fakeRun USING(fake_id)
JOIN camRun USING(cam_id)
JOIN chipRun USING (chip_id)
JOIN rawExp USING(exp_id)
LEFT JOIN skycalResult USING(skycal_id)
WHERE
   ((skycalRun.state = 'new' AND skycalResult.skycal_id IS NULL)
    OR (skycalRun.state ='update' and staticskyRun.state ='full' and skycalResult.fault = 0))
