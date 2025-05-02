-- SELECT
--  *
-- FROM
--  (SELECT 
--      chipMask.label as label, 
--      count(chipRun.chip_id) as n_chipruns
--   FROM 
--      chipMask 
--   LEFT JOIN 
--      chipRun 
--   USING 
--      (label) 
--   WHERE chipRun.chip_id IS NOT NULL 
--   GROUP BY label) as chipMask

-- SELECT
--  *
-- FROM
--  SELECT 
--      chipRun.*, 
--      rawExp.*,
--      chipRun.chip_id as n_chipruns
--   FROM 
--      chipRun 
--   JOIN rawExp ON chipRun.exp_id = rawExp.exp_id
--   LEFT JOIN chipMask 
--   USING (label) 
--   WHERE chipMask.label IS NULL 
--   AND filter = 'r'
--   AND chipRun.exp_id = 61
--  GROUP BY chipRun.label) as chipUnmask

SELECT
 *
FROM
 (SELECT 
     chipRun.label, 
     count(chipRun.chip_id) as n_chipruns
  FROM chipRun 
  LEFT JOIN chipMask 
  USING (label) 
  WHERE chipMask.label IS NULL 
  GROUP BY chipRun.label) as chipUnmask
