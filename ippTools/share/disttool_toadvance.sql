SELECT DISTINCT
    dist_id,
    stage,
    stage_id,
    CONCAT_WS('.', outroot, CONVERT(dist_id, CHAR)) as outdir,
    label,
    clean
FROM
    (
-- raw stage not clean
SELECT
    distRun.dist_id,
    stage,
    stage_id,
    outroot,
    label,
    clean
    FROM distRun
    JOIN rawImfile ON stage_id = rawImfile.exp_id
    LEFT JOIN distComponent
        ON distRun.dist_id = distComponent.dist_id
        AND distComponent.component = rawImfile.class_id
    WHERE
        distRun.state = 'new'
        AND distRun.clean = 0
        AND distRun.fault = 0
        AND distRun.stage = 'raw'
        AND rawImfile.ignored = 0
    GROUP BY
        distRun.dist_id,
        rawImfile.exp_id
    HAVING
        COUNT(rawImfile.class_id) = COUNT(distComponent.component)
        AND SUM(distComponent.fault) = 0
UNION
-- clean distribution of raw files (dbinfo only) only 1 component
SELECT
    distRun.dist_id,
    stage,
    stage_id,
    outroot,
    label,
    clean
    FROM distRun
    LEFT JOIN distComponent
        ON distRun.dist_id = distComponent.dist_id
    WHERE
        distRun.state = 'new'
        AND distRun.clean
        AND distRun.fault = 0
        AND distRun.stage = 'raw'
        AND distComponent.component IS NOT NULL
        AND distComponent.fault = 0
UNION
-- chip stage
SELECT
    distRun.dist_id,
    stage,
    stage_id,
    outroot,
    label,
    clean
    FROM distRun
    JOIN chipProcessedImfile ON stage_id = chipProcessedImfile.chip_id
    LEFT JOIN distComponent
        ON distComponent.dist_id = distRun.dist_id
        AND distComponent.component = chipProcessedImfile.class_id
    WHERE
        distRun.state = 'new'
        AND distRun.fault = 0
        AND distRun.stage = 'chip'
    GROUP BY
        dist_id,
        chipProcessedImfile.chip_id
    HAVING
        COUNT(chipProcessedImfile.class_id) = COUNT(distComponent.component)
        AND SUM(distComponent.fault) = 0
UNION
-- chip_bg stage
SELECT
    distRun.dist_id,
    stage,
    stage_id,
    outroot,
    label,
    clean
    FROM distRun
    JOIN chipBackgroundImfile ON stage_id = chipBackgroundImfile.chip_bg_id
    LEFT JOIN distComponent
        ON distComponent.dist_id = distRun.dist_id
        AND distComponent.component = chipBackgroundImfile.class_id
    WHERE
        distRun.state = 'new'
        AND distRun.fault = 0
        AND distRun.stage = 'chip_bg'
    GROUP BY
        dist_id,
        chipBackgroundImfile.chip_bg_id
    HAVING
        COUNT(chipBackgroundImfile.class_id) = COUNT(distComponent.component)
        AND SUM(distComponent.fault) = 0
UNION
-- camera stage
SELECT distRun.dist_id,
    stage,
    stage_id,
    outroot,
    distRun.label,
    clean
    FROM distRun
    JOIN camRun ON stage_id = cam_id
    JOIN chipRun USING(chip_id)
    LEFT JOIN distComponent  USING(dist_id)
    WHERE
        distRun.state = 'new'
        AND distRun.fault = 0
        AND distComponent.fault = 0
        AND distRun.stage = 'camera'
--        AND ((chipRun.magicked AND camRun.state = 'full') OR distRun.no_magic)
--        AND (camRun.state = 'full' OR (distRun.clean and camRun.state = 'cleaned'))
UNION
-- fake stage
SELECT
    distRun.dist_id,
    stage,
    stage_id,
    outroot,
    label,
    clean
    FROM distRun
    JOIN fakeProcessedImfile ON stage_id = fakeProcessedImfile.fake_id
    LEFT JOIN distComponent
        ON distComponent.dist_id = distRun.dist_id
        AND distComponent.component = fakeProcessedImfile.class_id
    WHERE
        distRun.state = 'new'
        AND distRun.fault = 0
        AND distRun.stage = 'fake'
    GROUP BY
        dist_id,
        fakeProcessedImfile.fake_id
    HAVING
        COUNT(fakeProcessedImfile.class_id) = COUNT(distComponent.component)
        AND SUM(distComponent.fault) = 0
UNION
-- warp stage
SELECT
    distRun.dist_id,
    stage,
    stage_id,
    outroot,
    label,
    clean
    FROM distRun
    JOIN warpSkyfile on stage_id = warp_id
    LEFT JOIN distComponent
        ON distRun.dist_id = distComponent.dist_id
        AND distComponent.component = warpSkyfile.skycell_id
    WHERE
        distRun.state = 'new'
        AND distRun.fault = 0
        AND distRun.stage = 'warp'
--        AND warpSkyfile.fault = 0
--        AND warpSkyfile.quality = 0
    GROUP BY
        distRun.dist_id,
        warp_id
    HAVING
        COUNT(warpSkyfile.skycell_id) = COUNT(distComponent.component)
        AND SUM(distComponent.fault) = 0
UNION
-- warp_bg stage
SELECT
    distRun.dist_id,
    stage,
    stage_id,
    outroot,
    label,
    clean
    FROM distRun
    JOIN warpBackgroundSkyfile on stage_id = warp_bg_id
    LEFT JOIN distComponent
        ON distRun.dist_id = distComponent.dist_id
        AND distComponent.component = warpBackgroundSkyfile.skycell_id
    WHERE
        distRun.state = 'new'
        AND distRun.fault = 0
        AND distRun.stage = 'warp_bg'
--        AND warpSkyfile.fault = 0
--        AND warpSkyfile.quality = 0
    GROUP BY
        distRun.dist_id,
        warp_bg_id
    HAVING
        COUNT(warpBackgroundSkyfile.skycell_id) = COUNT(distComponent.component)
        AND SUM(distComponent.fault) = 0
UNION
-- diff stage
SELECT
    distRun.dist_id,
    stage,
    stage_id,
    outroot,
    distRun.label,
    clean
    FROM distRun
    JOIN diffSkyfile
        ON stage_id = diffSkyfile.diff_id
    LEFT JOIN distComponent
        ON distRun.dist_id = distComponent.dist_id
        AND distComponent.component = diffSkyfile.skycell_id
    WHERE
        distRun.state = 'new'
        AND distRun.fault = 0
        AND distRun.stage = 'diff'
--        AND diffSkyfile.fault = 0
    GROUP BY
        distRun.dist_id,
        diff_id
    HAVING
        COUNT(diffSkyfile.skycell_id) = COUNT(distComponent.component)
        AND SUM(distComponent.fault) = 0
UNION
-- stack stage
SELECT
    distRun.dist_id,
    stage,
    stage_id,
    outroot,
    label,
    clean
    FROM distRun
    JOIN stackSumSkyfile on stage_id = stack_id
    LEFT JOIN distComponent
        ON distRun.dist_id = distComponent.dist_id
    WHERE
        distRun.state = 'new'
        AND distRun.fault = 0
        AND distRun.stage = 'stack'
        AND distComponent.component IS NOT NULL
        AND distComponent.fault = 0
UNION
-- staticsky stage
-- NOTE this assumes that there is only one component per staticskyRun
-- (one skycell)
SELECT
    distRun.dist_id,
    stage,
    stage_id,
    outroot,
    label,
    clean
    FROM distRun
    JOIN staticskyResult on stage_id = sky_id
    LEFT JOIN distComponent
        ON distRun.dist_id = distComponent.dist_id
    WHERE
        distRun.state = 'new'
        AND distRun.fault = 0
        AND distRun.stage = 'sky'
        AND distComponent.component IS NOT NULL
        AND distComponent.fault = 0
UNION
-- skycal stage
-- NOTE this assumes that there is only one component per skycalRun
-- (one skycell)
SELECT
    distRun.dist_id,
    stage,
    stage_id,
    outroot,
    label,
    clean
    FROM distRun
    JOIN skycalResult on stage_id = skycal_id
    LEFT JOIN distComponent
        ON distRun.dist_id = distComponent.dist_id
    WHERE
        distRun.state = 'new'
        AND distRun.fault = 0
        AND distRun.stage = 'skycal'
        AND distComponent.component IS NOT NULL
        AND distComponent.fault = 0
UNION
-- SSdiff stage
SELECT
    distRun.dist_id,
    stage,
    stage_id,
    outroot,
    distRun.label,
    clean
    FROM distRun
    JOIN diffSkyfile
        ON stage_id = diffSkyfile.diff_id
    LEFT JOIN distComponent
        ON distRun.dist_id = distComponent.dist_id
        AND distComponent.component = diffSkyfile.skycell_id
    WHERE
        distRun.state = 'new'
        AND distRun.fault = 0
        AND distRun.stage = 'SSdiff'
--        AND diffSkyfile.fault = 0
    GROUP BY
        distRun.dist_id,
        diff_id
    HAVING
        COUNT(diffSkyfile.skycell_id) = COUNT(distComponent.component)
        AND SUM(distComponent.fault) = 0
UNION
-- ff stage
SELECT
    distRun.dist_id,
    stage,
    stage_id,
    outroot,
    distRun.label,
    clean
    -- ,COUNT(warp_id)
    -- ,COUNT(ff_id)
    FROM distRun
    JOIN fullForceRun ON stage_id = ff_id
    JOIN fullForceResult USING(ff_id)
    JOIN skycalRun USING(skycal_id)
    JOIN stackRun USING(stack_id)
    LEFT JOIN distComponent
        ON distRun.dist_id = distComponent.dist_id
        AND distComponent.component = CONCAT_WS('.', warp_id, stackRun.skycell_id) 
    WHERE
        distRun.state = 'new'
        AND distRun.fault = 0
        AND distRun.stage = 'ff'
    GROUP BY
        distRun.dist_id,
        ff_id
    HAVING
        -- number of dist components is the number of warps plus 1 (for the summary component)
        COUNT(fullForceResult.ff_id) = COUNT(distComponent.component)
        AND SUM(distComponent.fault) = 0
) as Foo
