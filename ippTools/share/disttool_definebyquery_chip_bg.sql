SELECT DISTINCT
    'chip_bg' as stage,
    chipBackgroundRun.chip_bg_id as stage_id,
    chipBackgroundRun.magicked,
    rawExp.exp_name as run_tag,
    chipBackgroundRun.label,
    chipBackgroundRun.data_group,
    chipBackgroundRun.dist_group,
    distTarget.target_id,
    distTarget.clean
FROM chipBackgroundRun
JOIN chipRun USING(chip_id)
JOIN rawExp USING(exp_id)
JOIN distTarget
    ON distTarget.stage = 'chip_bg'
    AND chipBackgroundRun.dist_group = distTarget.dist_group
    AND rawExp.filter = distTarget.filter
JOIN rcInterest USING(target_id)
LEFT JOIN distRun
    ON distRun.stage_id = chipBackgroundRun.chip_bg_id
    AND distRun.target_id = distTarget.target_id
    -- JOIN hook %s
WHERE distTarget.state = 'enabled'
    AND rcInterest.state = 'enabled'
    AND ((chipBackgroundRun.state = 'full') OR (distTarget.clean AND chipBackgroundRun.state = 'cleaned'))
