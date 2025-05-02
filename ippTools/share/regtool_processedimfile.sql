SELECT
    rawImfile.*,
    newExp.tmp_exp_name,
    newExp.tmp_camera,
    newExp.tmp_telescope
FROM rawImfile
JOIN newExp
    USING(exp_id)
WHERE rawImfile.ignored = 0
