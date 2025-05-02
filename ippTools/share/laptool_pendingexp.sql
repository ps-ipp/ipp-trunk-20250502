select lapRun.lap_id, lapRun.seq_id, lapRun.tess_id, lapRun.projection_cell, lapRun.filter, 
       lapRun.state, lapRun.label,
       lapRun.dist_group, lapRun.registered, lapRun.fault, lapRun.quick_sass_id, lapRun.final_sass_id,
       exp_id, chip_id, pair_id, private, pairwise, active, lapExp.data_state,
       dateobs, object, comment,
       chipRun.state AS chip_state
  FROM lapRun JOIN lapExp USING(lap_id)
  JOIN rawExp USING(exp_id,filter)
  LEFT JOIN chipRun USING(exp_id,chip_id)
WHERE active IS TRUE AND lapRun.fault = 0
-- lap_id restriction here.
-- This probably needs to be sorted by dateobs.