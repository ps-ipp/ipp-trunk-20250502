SELECT
    corr_id,
    chipRun.*
FROM flatcorrRun
JOIN flatcorrChipLink
    USING(corr_id)
JOIN chipRun
    USING(chip_id)
LEFT JOIN flatcorrCamLink
    using(corr_id, chip_id)
WHERE
    flatcorrRun.state = 'new'
    AND flatcorrChipLink.include = 1
    AND chipRun.state = 'full'
    AND flatcorrCamLink.corr_id is NULL
