SELECT DISTINCT sass_id,
       stackAssociation.tess_id,
       stackAssociation.projection_cell,
       stackAssociation.filter,
       stackSummary.path_base,
       stackAssociation.data_group,
       rawExp.camera
FROM stackRun 
    JOIN stackAssociationMap USING(stack_id) 
    JOIN stackAssociation USING(sass_id, filter)
    JOIN stackSummary using(sass_id, projection_cell)
    JOIN stackInputSkyfile using(stack_id)
    JOIN warpRun USING(warp_id)
    JOIN fakeRun USING(fake_id)
    JOIN camRun USING(cam_id)
    JOIN chipRun USING(chip_id)
    JOIN rawExp USING(exp_id)
