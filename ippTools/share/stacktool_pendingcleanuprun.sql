-- results in too many runs                 adding DISTINCT fixes this
-- is the camera needed for this?           YES I think so that update can define the camera
-- is the camera unique for this?           YES. Well, sort of.
--                                          stack_skycell.pl insures that the files
--                                          we're here to clean come from the same camera so
--                                          if the camera is not unique we have nothing to do
SELECT DISTINCT
    stackRun.stack_id,
    rawExp.camera,
    stackRun.state,
    stackSumSkyfile.path_base
FROM stackRun
JOIN stackSumSkyfile using(stack_id)
JOIN stackInputSkyfile
    USING(stack_id)
JOIN warpSkyfile
    ON  stackInputSkyfile.warp_id = warpSkyfile.warp_id
    AND stackRun.skycell_id       = warpSkyfile.skycell_id
    AND stackRun.tess_id          = warpSkyfile.tess_id
JOIN warpRun
    ON warpRun.warp_id = warpSkyfile.warp_id
JOIN fakeRun
    USING(fake_id)
JOIN camRun
    USING(cam_id)
JOIN chipRun
    USING(chip_id)
JOIN rawExp 
     USING (exp_id)
WHERE
    (stackRun.state = 'goto_cleaned' OR stackRun.state = 'goto_scrubbed' OR stackRun.state = 'goto_purged')
