SELECT DISTINCT 
  want.exp_id, 
  have.chip_id, 
  COALESCE(have.private,false) as private, 
  true as active, 
  false as pairwise
  FROM
  (SELECT exp_id FROM rawExp 
     WHERE rawExp.exp_type= 'OBJECT' AND
     rawExp.dateobs >= '2009-04-01T00:00:00.000000' AND
-- Position restriction goes here.
     @WHERE@
  ) AS want
  LEFT JOIN 
  (SELECT 
     exp_id,
     MAX(chip_id) AS chip_id, 
     chipRun.state,
     lapExp.private
     FROM lapExp JOIN chipRun USING(exp_id,chip_id) 
     JOIN lapRun USING(lap_id)
     where chip_id IS NOT NULL
     AND 
-- This is the restriction to only draw from the current lapSequence
     @CHIPWHERE@
--     	 (
	  -- (active = TRUE) OR 
	  -- (chipRun.state = 'full') OR
	  -- (chipRun.state = 'new')
	  -- when we can do updates, put that here.
--	  )
     GROUP BY exp_id
) AS have USING(exp_id)
     

