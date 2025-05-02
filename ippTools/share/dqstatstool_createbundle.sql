-- Select the information we want from the various tables
SELECT @COLLIST@ FROM rawExp 
        JOIN chipRun USING(exp_id) 
        JOIN camRun USING(chip_id) 
	JOIN camProcessedExp USING(cam_id)
        LEFT OUTER JOIN fakeRun USING(cam_id) 
        LEFT OUTER JOIN warpRun USING(fake_id)