SELECT DISTINCT diff_id,"GPC1" as camera,diffRun.workdir,diffRun.tess_id,diffRun.diff_mode,diffRun.state
       FROM diffRun 
       LEFT JOIN diffSummary USING(diff_id)
WHERE diffRun.state = 'full' AND 
      diffSummary.projection_cell IS NULL 
