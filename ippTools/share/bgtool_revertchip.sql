DELETE chipBackgroundImfile
FROM chipBackgroundRun
JOIN chipBackgroundImfile USING(chip_bg_id)
WHERE chipBackgroundImfile.fault != 0
