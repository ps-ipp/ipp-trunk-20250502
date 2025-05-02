SELECT DISTINCT
     summit_id,
     exp_id,
     tmp_exp_name,
     tmp_camera,
     tmp_telescope,
     state,
     workdir,
     workdir_state,
     reduction,
     dvodb,
     tess_id,
     end_stage,
     label,
     camera,
     filter,
     obs_mode,
     obs_group,
     epoch,
     dateobs
FROM 
        (
        select DISTINCT * from 
         (
          select newExp.*,count(newImfile.tmp_class_id) AS new_count
          from newExp
          LEFT JOIN newImfile USING (exp_id)
          LEFT JOIN rawExp USING (exp_id) 
          where 
           newExp.state = 'run' AND rawExp.exp_id IS NULL
          GROUP BY newExp.exp_id
         ) AS NEWEXPOSURES
         JOIN 
         (
          select DISTINCT newExp.exp_id,
	  rawImfile.camera,
	  rawImfile.filter,
	  rawImfile.dateobs,
	  rawImfile.obs_mode,
	  rawImfile.obs_group,
	  count(rawImfile.tmp_class_id) AS raw_count
          from newExp
          LEFT JOIN rawExp USING (exp_id) 
          LEFT JOIN rawImfile USING (exp_id)
          where 
           newExp.state = 'run' AND rawExp.exp_id IS NULL
           AND rawImfile.data_state = 'full'
	   -- where hook %s
          GROUP BY newExp.exp_id
          HAVING SUM(rawImfile.fault) = 0
	  LIMIT 256
         ) AS RAWEXPOSURES
	 USING (exp_id)
        WHERE raw_count = new_count 
    ) AS Foo
-- limit hook %s
