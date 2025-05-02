UPDATE rawImfile JOIN rawExp USING(exp_id)
       SET data_state = '%s'
WHERE 1
-- where hook %s
-- limit hook %s