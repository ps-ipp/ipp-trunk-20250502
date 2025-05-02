SELECT 
       tess_id,	
       skycell_id, 
       num_filter 
FROM (
  SELECT
      tess_id,
      skycell_id,
      COUNT(DISTINCT filter) AS num_filter
  FROM stackRun
  JOIN stackSumSkyfile USING(stack_id)
  WHERE stackRun.state = 'full'
      AND stackSumSkyfile.fault = 0
      AND stackSumSkyfile.quality = 0
  -- WHERE hook 1 %s 
  -- restrict by stackSumSkyfile.good_frac, stackRun.skycell_id, stackRun.label
  GROUP BY
      tess_id,
      skycell_id
  ) AS TMP
WHERE num_filter == %d

