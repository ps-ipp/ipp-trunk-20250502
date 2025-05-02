-- does this result in too many entries (one for each diffInputSkyfile?)
-- all of this is just to get the camera used for the diff run
SELECT DISTINCT * FROM 
   (SELECT DISTINCT
	    diffRun.diff_id,
	    rawExp.camera,
	    diffRun.state,
            diffRun.workdir,
            diffRun.tess_id,
	    diffRun.label
	FROM diffRun
	JOIN diffInputSkyfile
	    USING(diff_id)
	JOIN warpSkyfile
	    ON  diffInputSkyfile.warp1    = warpSkyfile.warp_id
	    AND diffInputSkyfile.skycell_id = warpSkyfile.skycell_id
	    AND diffInputSkyfile.tess_id    = warpSkyfile.tess_id
	JOIN warpRun
	    ON warpRun.warp_id = warpSkyfile.warp_id
	JOIN fakeRun
	    USING(fake_id)
	JOIN camRun
	    USING(cam_id)
	JOIN chipRun
	    USING(chip_id)
	JOIN rawExp
	    USING(exp_id)
	WHERE
	@INNERCONSTRAINT@
	    (diffRun.state = 'goto_cleaned' OR 
             diffRun.state = 'goto_scrubbed' OR 
	     diffRun.state = 'goto_purged')
	@INNERLIMITS@
     UNION
     SELECT DISTINCT
	    diffRun.diff_id,
	    rawExp.camera,
	    diffRun.state,
            diffRun.workdir,
            diffRun.tess_id,
	    diffRun.label
	FROM diffRun
	JOIN diffInputSkyfile
	    USING(diff_id)
	JOIN warpSkyfile
	    ON  diffInputSkyfile.warp2    = warpSkyfile.warp_id
	    AND diffInputSkyfile.skycell_id = warpSkyfile.skycell_id
	    AND diffInputSkyfile.tess_id    = warpSkyfile.tess_id
	JOIN warpRun
	    ON warpRun.warp_id = warpSkyfile.warp_id
	JOIN fakeRun
	    USING(fake_id)
	JOIN camRun
	    USING(cam_id)
	JOIN chipRun
	    USING(chip_id)
	JOIN rawExp
	    USING(exp_id)
	WHERE
	@INNERCONSTRAINT@
	    (diffRun.state = 'goto_cleaned' OR 
             diffRun.state = 'goto_scrubbed' OR 
	     diffRun.state = 'goto_purged')
	@INNERLIMITS@
     UNION
     SELECT DISTINCT 
	    diffRun.diff_id,
	    rawExp.camera,
	    diffRun.state,
            diffRun.workdir,
            diffRun.tess_id,
	    diffRun.label
	FROM diffRun
	JOIN diffInputSkyfile
	    USING(diff_id)
--	JOIN stackSumSkyfile
--	    ON  diffInputSkyfile.stack1 = stackSumSkyfile.stack_id
	JOIN stackInputSkyfile
	    ON diffInputSkyfile.stack1 = stackInputSkyfile.stack_id
	JOIN warpRun
	    ON warpRun.warp_id = stackInputSkyfile.warp_id
	JOIN fakeRun
	    USING(fake_id)
	JOIN camRun
	    USING(cam_id)
	JOIN chipRun
	    USING(chip_id)
	JOIN rawExp
	    USING(exp_id)
	WHERE
	@INNERCONSTRAINT@
	    (diffRun.state = 'goto_cleaned' OR 
             diffRun.state = 'goto_scrubbed' OR 
	     diffRun.state = 'goto_purged')
	@INNERLIMITS@
     UNION
     SELECT DISTINCT
	    diffRun.diff_id,
	    rawExp.camera,
	    diffRun.state,
            diffRun.workdir,
            diffRun.tess_id,
	    diffRun.label
	FROM diffRun
	JOIN diffInputSkyfile
	    USING(diff_id)
--	JOIN stackSumSkyfile
--	    ON  diffInputSkyfile.stack2 = stackSumSkyfile.stack_id
	JOIN stackInputSkyfile
	    ON diffInputSkyfile.stack2 = stackInputSkyfile.stack_id
	JOIN warpRun
	    ON warpRun.warp_id = stackInputSkyfile.warp_id
	JOIN fakeRun
	    USING(fake_id)
	JOIN camRun
	    USING(cam_id)
	JOIN chipRun
	    USING(chip_id)
	JOIN rawExp
	    USING(exp_id)
	WHERE
	@INNERCONSTRAINT@
	    (diffRun.state = 'goto_cleaned' OR 
             diffRun.state = 'goto_scrubbed' OR 
	     diffRun.state = 'goto_purged')
	@INNERLIMITS@
	) as Foo
	WHERE 1
