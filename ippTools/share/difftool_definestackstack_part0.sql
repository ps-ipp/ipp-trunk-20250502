SELECT DISTINCT
       stackRun.data_group AS INPUT_data_group,
       stackRun.dist_group AS INPUT_dist_group,
       stackRun.filter AS INPUT_filter,
       stackRun.label AS INPUT_label,
       stackRun.tess_id AS INPUT_tess_id
--     ,
--       stackRun.stack_id AS INPUT_stack_id,
--       template.max_stack_id,
--       diffExp.diff_id
FROM stackRun JOIN stackSumSkyfile USING(stack_id) 
JOIN (
     SELECT 
     	    MAX(stackRun.stack_id) as max_stack_id,
	    stackRun.filter,
	    stackRun.label as template_label,
	    stackRun.tess_id,
	    stackRun.skycell_id,
	    stackSumSkyfile.good_frac
     FROM stackRun JOIN stackSumSkyfile USING(stack_id)
     WHERE 
     	   stackRun.state = 'full' AND
	   stackSumSkyfile.fault = 0 AND stackSumSkyfile.quality = 0
	   @STACK2_QUERY@ -- template condition
     GROUP BY
     	   skycell_id,
	   filter
) AS template ON stackRun.tess_id = template.tess_id 
  	      AND stackRun.skycell_id = template.skycell_id 
	      @FILT0_QUERY@  -- include filter constraint if requested
LEFT JOIN (
     SELECT 
     	    diffRun.diff_id,
	    diffInputSkyfile.stack1,
	    diffInputSkyfile.stack2
     FROM diffRun JOIN diffInputSkyfile USING(diff_id) 
     	  JOIN stackRun ON stackRun.stack_id = diffInputSkyfile.stack1 
	  JOIN stackSumSkyfile USING(stack_id)
	  WHERE 1
	  @STACK1_QUERY@ -- input condition
) AS diffExp ON diffExp.stack1 = stackRun.stack_id 
WHERE
	stackSumSkyfile.fault = 0 AND stackSumSkyfile.quality = 0
	@DIFF0_QUERY@  -- diff constraint
	@STACK1_QUERY@ -- input constraint
ORDER BY stackRun.data_group,stackRun.filter
