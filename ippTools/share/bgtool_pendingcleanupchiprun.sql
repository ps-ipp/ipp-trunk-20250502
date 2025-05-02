SELECT 
    chipBackgroundRun.*,
    CONCAT_WS('.', exp_name, exp_id) AS exp_tag,
    rawExp.camera
FROM chipBackgroundRun
    JOIN chipRun USING(chip_id)
    JOIN rawExp USING(exp_id)
WHERE (chipBackgroundRun.state = 'goto_cleaned' 
    OR chipBackgroundRun.state = 'goto_purged'
    OR chipBackgroundRun.state = 'goto_scrubbed'
    )

