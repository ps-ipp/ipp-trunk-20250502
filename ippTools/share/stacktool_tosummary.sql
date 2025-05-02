SELECT sass_id,camera,workdir,tess_id,state FROM 
(SELECT sass_id,rawExp.camera,stackRun.workdir,stackRun.tess_id,stackRun.state,
       sum((stackRun.state = 'full')) AS is_done, sum(1) AS is_defined
       FROM stackRun
       JOIN stackInputSkyfile ON stackRun.stack_id = stackInputSkyfile.stack_id
       JOIN stackAssociationMap ON stackRun.stack_id = stackAssociationMap.stack_id
       JOIN stackAssociation USING(sass_id)
       JOIN warpRun USING(warp_id)
       JOIN fakeRun USING(fake_id)
       JOIN camRun USING(cam_id)
       JOIN chipRun USING(chip_id)
       JOIN rawExp USING(exp_id)
       LEFT JOIN stackSummary USING(sass_id)
WHERE stackRun.state = 'full' AND
      stackSummary.projection_cell IS NULL
-- WHERE HOOK %s
GROUP BY sass_id
) AS T
WHERE is_done = is_defined