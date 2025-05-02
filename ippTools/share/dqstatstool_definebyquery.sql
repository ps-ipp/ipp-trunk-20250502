-- query to identify unique data to bundle

SELECT DISTINCT exp_id,chip_id,cam_id,warpRun.warp_id,invalid from rawExp 
	JOIN chipRun USING(exp_id) 
	JOIN camRun USING(chip_id) 
	JOIN camProcessedExp USING(cam_id)
	LEFT OUTER JOIN fakeRun USING(cam_id) 
	LEFT OUTER JOIN warpRun USING(fake_id)
	LEFT OUTER JOIN dqstatsContent USING (exp_id,chip_id,cam_id)
WHERE camRun.state = 'full'
