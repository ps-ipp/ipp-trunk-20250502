SELECT rawImfile.*,rawExp.workdir,rawExp.exp_tag from rawImfile
       JOIN rawExp USING(exp_id)
       WHERE  1
-- ((data_state = 'goto_compressed' AND state = 'goto_compressed')
--       	   OR (data_state = 'goto_lossy' AND state = 'goto_lossy'))
-- where hook %s
-- limit hook %s       