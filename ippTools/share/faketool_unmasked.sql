SELECT
 *
FROM
 (SELECT 
     fakeRun.label, 
     count(fakeRun.fake_id) as n_fakeruns
  FROM fakeRun 
  LEFT JOIN fakeMask 
  USING (label) 
  WHERE fakeMask.label IS NULL 
  GROUP BY fakeRun.label) as fakeUnmask
