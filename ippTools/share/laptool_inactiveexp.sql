SELECT DISTINCT 
    D.*,diffRun.state AS diff_state,coalesce(CONVERT(sum(others.active),SIGNED),0) AS is_in_use FROM (
  SELECT DISTINCT 
      W.*,IFNULL(diff1.diff_id,diff2.diff_id) AS diff_id FROM (
    SELECT DISTINCT
        lapRun.lap_id, lapRun.seq_id, lapRun.tess_id, lapRun.projection_cell, lapRun.filter, lapRun.state, lapRun.label,
        lapRun.dist_group, lapRun.registered, lapRun.fault, lapRun.quick_sass_id, lapRun.final_sass_id,
        lapExp.exp_id,lapExp.chip_id,lapExp.pair_id,private,pairwise,active,lapExp.data_state,
        chipRun.state as chipRun_state, 
	CONVERT(sum(chipProcessedImfile.fault),SIGNED) as chip_faults, 
	CONVERT(sum(chipProcessedImfile.quality),SIGNED) as chip_quality,
        camRun.cam_id, camRun.state as camRun_state,   
	CONVERT(sum(camProcessedExp.fault),SIGNED) AS cam_faults, 
	CONVERT(sum(camProcessedExp.quality),SIGNED) AS cam_quality,
	fakeRun.fake_id, fakeRun.state as fakeRun_state, 
	CONVERT(sum(fakeProcessedImfile.fault),SIGNED) as fake_faults,
  	warpRun.warp_id, warpRun.state as warpRun_state, 
	CONVERT(sum(warpSkyfile.fault),SIGNED) as warp_faults, 
	CONVERT(sum(warpSkyfile.quality),SIGNED) as warp_quality,
        warpRun.magicked
    FROM lapRun JOIN lapExp USING(lap_id)
    LEFT JOIN chipRun USING(chip_id)
    LEFT JOIN chipProcessedImfile USING(chip_id)
    LEFT JOIN camRun USING(chip_id) LEFT JOIN camProcessedExp USING(cam_id)
    LEFT JOIN fakeRun USING(cam_id) LEFT JOIN fakeProcessedImfile USING(fake_id)
    LEFT JOIN warpRun USING(fake_id) LEFT JOIN warpSkyfile USING(warp_id)
    WHERE lapExp.active = FALSE
    AND @WHERE@
    AND (warpSkyfile.quality IS NULL OR
         (warpSkyfile.quality != 8007      -- known cases where quality != 0, but everything's fine.
          AND warpSkyfile.quality != 3006  -- known cases where quality != 0, but everything's fine.
          ))
    GROUP BY lap_id,exp_id
    ) AS W
-- This was unreasonably slow in testing, so that's why I'm using a subquery here.
  LEFT JOIN diffInputSkyfile AS diff1 ON (W.warp_id = diff1.warp1)
  LEFT JOIN diffInputSkyfile AS diff2 ON (W.warp_id = diff2.warp2)
) AS D
LEFT JOIN diffRun USING(diff_id)
LEFT JOIN lapExp AS others ON (D.chip_id = others.chip_id AND D.lap_id != others.lap_id)
GROUP BY lap_id,exp_id
