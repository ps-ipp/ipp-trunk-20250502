SELECT diffRun.*, diffInputSkyfile.diff_skyfile_id 
FROM 
     diffInputSkyfile 
JOIN diffSkyfile using (diff_id, skycell_id) 
JOIN diffRun using (diff_id,tess_id)
JOIN skycell using (skycell_id, tess_id) 
     	     	   JOIN warpRun on (warp1 = warp_id 
                   AND diffInputSkyfile.skycell_id = skycell_id) 
                   JOIN fakeRun using (fake_id) 
                   JOIN camRun using (cam_id) 
                   JOIN chipRun using (chip_id)  
                   JOIN rawExp using (exp_id) 
WHERE diffRun.state = 'full' and diffSkyfile.quality = 0
    AND diffInputSkyfile.diff_skyfile_id NOT IN (SELECT diffInputSkyfile.diff_skyfile_id
    FROM 
    	 diffInputSkyfile  
    JOIN diffSkyfile     USING (diff_id, skycell_id) 
    JOIN diffRun     using(diff_id, tess_id) 
    JOIN skycell using (skycell_id, tess_id) 
    left join addRun  ON (diff_id = stage_id and diff_skyfile_id = stage_extra1 and stage = "diff")
       WHERE addRun.stage = 'diff' AND %s
      )
