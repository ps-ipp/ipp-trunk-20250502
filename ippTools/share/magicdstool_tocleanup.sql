SELECT
    magic_ds_id,
    stage,
    magicDSRun.state,
    magicDSRun.outroot,
    camera
FROM magicDSRun 
    JOIN magicRun USING(magic_id)
    JOIN rawExp USING(exp_id)
WHERE magicDSRun.state = 'goto_cleaned'
-- XXX: need to add fault to magicDSRun
-- XXX: the database has been updated, but fault isn't yet used
--    AND magicDSRun.fault = 0

