SELECT DISTINCT
    fakeRun.*,
    chipRun.chip_id,
    rawExp.camera,
    rawExp.telescope,
    rawExp.dateobs,
    rawExp.exp_id,
    rawExp.exp_tag,
    rawExp.exp_name,
    rawExp.exp_type,
    rawExp.filelevel,
    rawExp.filter,
    rawExp.airmass,
    rawExp.ra,
    rawExp.decl,
    rawExp.exp_time,
    rawExp.sat_pixel_frac,
    rawExp.bg,
    rawExp.bg_stdev,
    rawExp.bg_mean_stdev,
    rawExp.alt,
    rawExp.az,
    rawExp.ccd_temp,
    rawExp.posang,
    rawExp.object,
    rawExp.sun_angle
FROM fakeRun
JOIN camRun USING(cam_id)
JOIN chipRun USING(chip_id)
JOIN rawExp USING(exp_id)
WHERE fakeRun.state = 'full'
AND camRun.state = 'full'
AND chipRun.state = 'full'
