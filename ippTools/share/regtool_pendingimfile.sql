SELECT DISTINCT
    newExp.exp_id,
    newExp.tmp_exp_name,
    newExp.tmp_camera,
    newExp.tmp_telescope,
    newExp.workdir,
    newImfile.tmp_class_id,
    newImfile.uri,
    newImfile.bytes,
    newImfile.md5sum,
    summitExp.dateobs AS summit_dateobs
FROM newImfile
JOIN newExp
    USING(exp_id)
LEFT JOIN summitExp USING(summit_id)
LEFT JOIN rawImfile
    ON newExp.exp_id = rawImfile.exp_id
    AND newImfile.tmp_class_id = rawImfile.tmp_class_id
WHERE
    newExp.state = 'run'
    AND rawImfile.tmp_class_id IS NULL
