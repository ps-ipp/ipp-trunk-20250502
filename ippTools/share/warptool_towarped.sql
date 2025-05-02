SELECT 
    warp_id,
    warp_skyfile_id,
    skycell_id,
    tess_id,
    fake_id,
    state,
    reduction,
    cam_id,
    camera,
    exp_tag,
    workdir,
    label,
    magicked,
    path_base,
    priority
FROM (
    (SELECT DISTINCT
        warpSkyCellMap.warp_id,
        warpImfile.warp_skyfile_id,
        warpSkyCellMap.skycell_id,
        warpSkyCellMap.tess_id,
        warpRun.fake_id,
        warpRun.state,
        warpRun.reduction,
        warpRun.label,
        camRun.cam_id,
        rawExp.camera,
        rawExp.exp_tag,
        warpRun.workdir,
        chipRun.magicked,
        CAST(NULL AS CHAR(255)) AS path_base,
        IFNULL(Label.priority, 10000) AS priority
    FROM warpRun
    JOIN warpSkyCellMap
        USING(warp_id)
    JOIN warpImfile
        ON warpSkyCellMap.warp_id = warpImfile.warp_id
        AND warpSkyCellMap.skycell_id = warpImfile.skycell_id
    JOIN fakeRun
        USING(fake_id)
    JOIN camRun
        USING(cam_id)
    JOIN chipRun
        USING(chip_id)
    JOIN chipProcessedImfile
        USING(chip_id)
    JOIN rawExp
        ON chipRun.exp_id = rawExp.exp_id
    LEFT JOIN warpSkyfile
        ON warpRun.warp_id = warpSkyfile.warp_id
        AND warpSkyCellMap.skycell_id = warpSkyfile.skycell_id
        AND warpSkyCellMap.tess_id = warpSkyfile.tess_id
    LEFT JOIN warpMask
        ON warpRun.label = warpMask.label
    LEFT JOIN Label ON warpRun.label = Label.label
    WHERE
        warpRun.state = 'new'
        AND warpSkyfile.warp_id IS NULL
        AND warpSkyfile.skycell_id IS NULL
        AND warpSkyfile.tess_id IS NULL
        AND fakeRun.state = 'full'
        AND camRun.state = 'full'
        AND chipRun.state = 'full'
        AND warpMask.label IS NULL
        AND warpSkyCellMap.fault = 0
        AND (Label.active OR Label.active IS NULL)
        -- where hook 1 %s
        -- limit hook 1 %s
    )
UNION
    (SELECT
        warpSkyCellMap.warp_id,
        warpImfile.warp_skyfile_id,
        warpSkyCellMap.skycell_id,
        warpSkyCellMap.tess_id,
        warpRun.fake_id,
        warpRun.state,
        warpRun.reduction,
        warpRun.label,
        camRun.cam_id,
        rawExp.camera,
        rawExp.exp_tag,
        warpRun.workdir,
        MIN(chipProcessedImfile.magicked) AS magicked,
        warpSkyfile.path_base,
        IFNULL(Label.priority, 10000) AS priority
    FROM warpRun
    JOIN warpImfile USING(warp_id)
    JOIN warpSkyCellMap USING(warp_id, skycell_id)
    JOIN warpSkyfile USING(warp_id, skycell_id)
    JOIN fakeRun USING(fake_id)
    JOIN camRun USING(cam_id)
    JOIN chipRun USING(chip_id)
    JOIN rawExp USING(exp_id)
    LEFT JOIN chipProcessedImfile USING(chip_id, class_id)
    LEFT JOIN Label ON warpRun.label = Label.label
    WHERE warpRun.state = 'update'
        AND warpSkyfile.data_state = 'update'
        AND warpSkyfile.fault = 0
        AND camRun.state = 'full'
        AND chipProcessedImfile.quality = 0
        -- Maybe add this to solve the if any chip is bad a whole warp can't be updated problem?
        -- AND (chipProcessedImfile.fault = 0 OR chipProcessedImfile.fault = 26)
        AND (Label.active OR Label.active IS NULL)
        -- where hook 2 %s
    GROUP BY warp_id, skycell_id
    HAVING COUNT(warpSkyCellMap.class_id) = SUM(IF(chipProcessedImfile.data_state ='full', 1, 0))
    -- limit hook 2 %s
    )
) as towarped
