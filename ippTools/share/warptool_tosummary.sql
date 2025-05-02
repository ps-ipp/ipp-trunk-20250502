SELECT DISTINCT warp_id,rawExp.camera,warpRun.workdir,warpRun.tess_id,rawExp.exp_tag,warpRun.state
       FROM warpRun 
       JOIN fakeRun USING(fake_id) JOIN camRun USING(cam_id)
       JOIN chipRun USING(chip_id) JOIN rawExp USING(exp_id)
       LEFT JOIN warpSummary USING(warp_id)
WHERE warpRun.state = 'full' AND 
      warpSummary.projection_cell IS NULL 
