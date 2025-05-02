SELECT
    corr_id,
    camRun.*
FROM flatcorrRun
JOIN flatcorrCamLink
    USING(corr_id)
JOIN camRun
    USING(cam_id)
LEFT JOIN flatcorrAddstarLink
    using(corr_id, cam_id)
WHERE
    flatcorrRun.state = 'new'
    AND flatcorrCamLink.include = 1
    AND camRun.state = 'full'
    AND flatcorrAddstarLink.corr_id is NULL
