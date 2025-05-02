SELECT 
       lapRun.lap_id,lapRun.seq_id,lapRun.tess_id,lapRun.projection_cell,lapRun.filter,
       lapRun.state,lapRun.label,lapRun.dist_group,lapRun.registered,lapRun.fault,
       skycell_id,
       lapRun.quick_sass_id,quick_data_group,quick_stack_id,quick_state,quick_fault,quick_quality,
       lapRun.final_sass_id,final_data_group,final_stack_id,final_state,final_fault,final_quality
       FROM
       lapRun LEFT JOIN 
       (
SELECT DISTINCT 
       lap_id,tess_id,projection_cell,filter,skycell_id,
       quick_sass_id,quick_data_group,quick_stack_id,quick_state,quick_fault,quick_quality,
       final_sass_id,final_data_group,final_stack_id,final_state,final_fault,final_quality
       FROM
       (select lapRun.lap_id,
       	       stackAssociation.sass_id AS quick_sass_id,
	       stackAssociation.data_group AS quick_data_group,
	       stackAssociation.projection_cell,
	       stackAssociation.tess_id,
	       stackAssociation.filter,
	       stackRun.stack_id AS quick_stack_id,
	       stackRun.skycell_id,
	       stackRun.state AS quick_state,
	       stackSumSkyfile.fault AS quick_fault,
	       stackSumSkyfile.quality AS quick_quality FROM
	   lapRun 
	   LEFT JOIN stackAssociation ON (lapRun.quick_sass_id = stackAssociation.sass_id)
	   LEFT JOIN stackAssociationMap USING (sass_id)
	   LEFT JOIN stackRun USING(stack_id)
	   LEFT JOIN stackSumSkyfile USING(stack_id)
	   WHERE 1 @WHERE@
	   AND sass_id = lapRun.quick_sass_id
       ) AS quick LEFT JOIN
       (select lapRun.lap_id,
       	       stackAssociation.sass_id AS final_sass_id,
	       stackAssociation.data_group AS final_data_group,
	       stackAssociation.projection_cell,
	       stackAssociation.tess_id,
	       stackAssociation.filter,
	       stackRun.stack_id AS final_stack_id,
	       stackRun.skycell_id,
	       stackRun.state AS final_state,
	       stackSumSkyfile.fault AS final_fault,
	       stackSumSkyfile.quality AS final_quality FROM
	   lapRun 
	   LEFT JOIN stackAssociation ON (lapRun.final_sass_id = stackAssociation.sass_id)
	   LEFT JOIN stackAssociationMap USING (sass_id)
	   LEFT JOIN stackRun USING(stack_id)
	   LEFT JOIN stackSumSkyfile USING(stack_id)
	   WHERE 1 @WHERE@
	   AND sass_id = lapRun.final_sass_id
       ) AS final USING(lap_id,projection_cell,tess_id,filter,skycell_id)
       UNION
SELECT DISTINCT 
       lap_id,tess_id,projection_cell,filter,skycell_id,
       quick_sass_id,quick_data_group,quick_stack_id,quick_state,quick_fault,quick_quality,
       final_sass_id,final_data_group,final_stack_id,final_state,final_fault,final_quality
       FROM
       (select lapRun.lap_id,
       	       stackAssociation.sass_id AS quick_sass_id,
	       stackAssociation.data_group AS quick_data_group,
	       stackAssociation.projection_cell,
	       stackAssociation.tess_id,
	       stackAssociation.filter,
	       stackRun.stack_id AS quick_stack_id,
	       stackRun.skycell_id,
	       stackRun.state AS quick_state,
	       stackSumSkyfile.fault AS quick_fault,
	       stackSumSkyfile.quality AS quick_quality FROM
	   lapRun 
	   LEFT JOIN stackAssociation ON (lapRun.quick_sass_id = stackAssociation.sass_id)
	   LEFT JOIN stackAssociationMap USING (sass_id)
	   LEFT JOIN stackRun USING(stack_id)
	   LEFT JOIN stackSumSkyfile USING(stack_id)
	   WHERE 1 @WHERE@
	   AND sass_id = lapRun.quick_sass_id
       ) AS quick RIGHT JOIN
       (select lapRun.lap_id,
       	       stackAssociation.sass_id AS final_sass_id,
	       stackAssociation.data_group AS final_data_group,
	       stackAssociation.projection_cell,
	       stackAssociation.tess_id,
	       stackAssociation.filter,
	       stackRun.stack_id AS final_stack_id,
	       stackRun.skycell_id,
	       stackRun.state AS final_state,
	       stackSumSkyfile.fault AS final_fault,
	       stackSumSkyfile.quality AS final_quality FROM
	   lapRun 
	   LEFT JOIN stackAssociation ON (lapRun.final_sass_id = stackAssociation.sass_id)
	   LEFT JOIN stackAssociationMap USING (sass_id)
	   LEFT JOIN stackRun USING(stack_id)
	   LEFT JOIN stackSumSkyfile USING(stack_id)
	   WHERE 1 @WHERE@
	   AND sass_id = lapRun.final_sass_id
       ) AS final USING(lap_id,projection_cell,tess_id,filter,skycell_id)
 ) stacks ON (stacks.lap_id = lapRun.lap_id AND
             stacks.projection_cell = lapRun.projection_cell AND
             stacks.tess_id = lapRun.tess_id AND
             stacks.filter = lapRun.filter AND
             (lapRun.quick_sass_id IS NULL OR lapRun.quick_sass_id = stacks.quick_sass_id) AND
             (lapRun.final_sass_id IS NULL OR lapRun.final_sass_id = stacks.final_sass_id))
WHERE 1 @WHERE@