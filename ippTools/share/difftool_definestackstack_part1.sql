SELECT distinct
       stackRun.stack_id,
       stackRun.data_group,
       stackRun.filter,
       stackRun.label,
       stackRun.tess_id,
       stackRun.skycell_id,
       stackSumSkyfile.good_frac,
       template.max_stack_id,
       template.template_label,
       count(diffExp.diff_id) as n_diff
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
	   @STACK2_QUERY@ -- template constraint
     GROUP BY
     	   skycell_id,
	   filter
) AS template ON stackRun.tess_id = template.tess_id 
	      AND stackRun.skycell_id = template.skycell_id 
	      @FILT1_QUERY@  -- include filter constraint if requested
LEFT JOIN (
     SELECT 
     	    diffRun.diff_id,
	    diffInputSkyfile.stack1,
	    diffInputSkyfile.stack2
     FROM diffRun JOIN diffInputSkyfile USING(diff_id) 
     	  JOIN stackRun ON stackRun.stack_id = diffInputSkyfile.stack1 
	  JOIN stackSumSkyfile USING(stack_id)
	  WHERE 1
	  @STACK1_QUERY@ -- input constraint
) AS diffExp ON diffExp.stack1 = stackRun.stack_id 
WHERE
	stackSumSkyfile.fault = 0 AND stackSumSkyfile.quality = 0
	@STACK1_QUERY@ -- input constraint
group by stackRun.stack_id,
       stackRun.data_group,
       stackRun.filter,
       stackRun.label,
       stackRun.tess_id,
       stackRun.skycell_id,
       stackSumSkyfile.good_frac,
       template.max_stack_id,
       template.template_label
       @DIFF1_QUERY@  -- diff constraint
ORDER BY stackRun.data_group,stackRun.filter,stackRun.skycell_id
