SELECT
    *
FROM
    (SELECT DISTINCT
        camRun.cam_id,
        chipRun.*,
        rawExp.camera,
        rawExp.telescope,
        rawExp.dateobs,
        rawExp.exp_tag,
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
        rawExp.sun_angle,
        rawExp.comment
    FROM camRun
    JOIN chipRun
        using(chip_id)
    JOIN rawExp
        using(exp_id)
    WHERE
        camRun.state = 'full'
        AND chipRun.state = 'full'
) as Foo
