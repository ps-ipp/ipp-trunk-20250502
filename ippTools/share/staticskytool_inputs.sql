SELECT
  staticskyRun.*,
  staticskyInput.stack_id,
  stackSumSkyfile.path_base,
  stackRun.filter
FROM staticskyRun
JOIN staticskyInput USING(sky_id)
JOIN stackSumSkyfile USING (stack_id)
JOIN stackRun using(stack_id)
