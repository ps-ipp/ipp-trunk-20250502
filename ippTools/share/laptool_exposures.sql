-- Join the diff information
-- select DISTINCT V3.*,
--       diffRun.diff_id,diffRun.state as diffRun_state,
--       coalesce(CONVERT(sum(diffSkyfile.quality != 0),SIGNED),0) AS diff_bad_quality,
--       coalesce(CONVERT(count(diffSkyfile.diff_id),SIGNED),0) AS diff_component_count
--       FROM
select V3.*,-1 as diff_id ,"full" as diffRun_state,0 as diff_bad_quality,0 AS diff_component_count FROM
-- Join the warp information
( select V2.*,
       warpRun.warp_id,warpRun.state as warpRun_state,
       coalesce(CONVERT(sum(warpSkyfile.quality != 0),SIGNED),0) AS warp_bad_quality,
       coalesce(CONVERT(count(warpSkyfile.warp_id),SIGNED),0) AS warp_component_count,
       warpRun.magicked
       FROM
-- Join the camera and fake information 
( select V1.*,
       camRun.cam_id,camRun.state as camRun_state,
       coalesce(CONVERT(sum(camProcessedExp.quality != 0),SIGNED),0) AS cam_bad_quality,
       coalesce(CONVERT(count(camProcessedExp.cam_id),SIGNED),0) AS cam_component_count,
       fakeRun.fake_id,fakeRun.state as fakeRun_state FROM
-- Get all the lap, exposure, chip stage information
( SELECT DISTINCT
       lap_id,lapRun.tess_id,projection_cell,filter,lapRun.state as lapRun_state, lapRun.registered, lapRun.fault, lapRun.label, lapRun.dist_group,
       lapExp.exp_id,lapExp.chip_id,lapExp.pair_id,private,pairwise,active,lapExp.data_state,
       chipRun.state as chipRun_state,
       coalesce(CONVERT(sum(chipProcessedImfile.quality != 0),SIGNED),0) AS chip_bad_quality,
       coalesce(CONVERT(count(chipProcessedImfile.chip_id),SIGNED),0) AS chip_component_count
       FROM lapRun JOIN lapExp USING(lap_id)
       LEFT JOIN chipRun USING(chip_id) LEFT JOIN chipProcessedImfile USING(chip_id)
WHERE @WHERE@ 
       GROUP BY lap_id,exp_id
       ) AS V1
-- End lap/exposure/chip
       LEFT JOIN camRun USING(chip_id) LEFT JOIN camProcessedExp USING(cam_id)
       LEFT JOIN fakeRun USING(cam_id)
       GROUP BY lap_id,exp_id
  ) AS V2
-- End camera/fake
  LEFT JOIN warpRun USING(fake_id) LEFT JOIN warpSkyfile USING(warp_id)
  GROUP BY lap_id,exp_id
) AS V3
-- End warp
-- LEFT JOIN
--  (SELECT DISTINCT diff_id,warp1,warp2 FROM diffInputSkyfile) AS DI ON
--  (DI.warp1 = warp_id OR DI.warp2 = warp_id)
-- LEFT JOIN diffRun USING(diff_id)
-- LEFT JOIN diffSkyfile USING(diff_id)
GROUP BY lap_id,exp_id

