  SELECT
      MAX(stack_id) as stack_id,
      tess_id,
      skycell_id,
      filter
  FROM stackRun
  JOIN stackSumSkyfile USING(stack_id)
  JOIN skycell USING(tess_id, skycell_id)
  WHERE stackRun.state = 'full'
      AND stackSumSkyfile.fault = 0
      AND stackSumSkyfile.quality = 0
      AND stackRun.tess_id = '%s'
      AND stackRun.skycell_id = '%s'
  -- WHERE hook 1 %s
  -- WHERE hook 2 %s
  GROUP BY stackRun.filter
