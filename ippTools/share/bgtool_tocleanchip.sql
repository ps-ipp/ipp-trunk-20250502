SELECT
    chipBackgroundRun.chip_bg_id,
    chipBackgroundRun.state,
    rawExp.camera
FROM chipBackgroundRun
JOIN chipRun USING(chip_id)
JOIN rawExp USING(exp_id)
WHERE chipBackgroundRun.state IN ('goto_cleaned', 'goto_scrubbed', 'goto_purged')

