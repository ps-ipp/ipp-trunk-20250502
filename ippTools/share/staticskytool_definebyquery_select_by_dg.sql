SELECT DISTINCT
       tess_id,	
       skycell_id,
       Stacks.data_group,
       num_filter 
FROM (
  SELECT
      tess_id,
      skycell_id,
      label,
      data_group,
      COUNT(DISTINCT filter) AS num_filter
  FROM stackRun
  JOIN stackSumSkyfile USING(stack_id)
  WHERE stackRun.state = 'full'
      AND stackSumSkyfile.fault = 0
      AND stackSumSkyfile.quality = 0
  -- WHERE hook 1 %s 
  -- restrict by selected filters, stackSumSkyfile.good_frac, stackRun.skycell_id, stackRun.label
  GROUP BY
      tess_id,
      skycell_id,
      data_group
  ) AS Stacks
LEFT JOIN (
  SELECT DISTINCT sky_id,
    stackRun.skycell_id,
    stackRun.tess_id,
    stackRun.data_group,
    COUNT(stack_id) AS num_filter
  FROM staticskyRun
    JOIN staticskyInput USING(sky_id)
    JOIN stackRun USING(stack_id)
    JOIN stackSumSkyfile USING(stack_id)
    WHERE 1
    -- WHERE hook 2 %s
    -- restrict by selected filters, stackSumSkyfile.good_frac, stackRun.skycell_id, stackRun.label
  GROUP BY sky_id
 ) AS ExistingRunsInputs
USING(tess_id, skycell_id, data_group, num_filter)
WHERE num_filter = %d
    AND Stacks.data_group = ExistingRunsInputs.data_group
-- WHERE hook 3 %s
