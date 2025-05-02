SELECT DISTINCT
    magic_ds_id,
    magicked,
    re_place,
    label
FROM
    (
-- raw stage
SELECT
    magicDSRun.*,
    rawExp.magicked
    FROM magicDSRun
    JOIN rawImfile ON stage_id = rawImfile.exp_id
    JOIN rawExp using(exp_id)
    LEFT JOIN magicDSFile
        ON magicDSRun.magic_ds_id = magicDSFile.magic_ds_id
        AND magicDSFile.component = rawImfile.class_id
    WHERE
        magicDSRun.state = 'new'
        AND magicDSRun.stage = 'raw'
    GROUP BY
        magicDSRun.magic_ds_id,
        stage_id
    HAVING
        COUNT(rawImfile.class_id) = COUNT(magicDSFile.component)
        AND SUM(magicDSFile.fault) = 0
UNION
-- chip stage
SELECT
    magicDSRun.*,
    chipRun.magicked
    FROM magicDSRun
    JOIN chipRun ON stage_id = chip_id
    JOIN chipProcessedImfile USING(chip_id)
    LEFT JOIN magicDSFile
        ON magicDSFile.magic_ds_id = magicDSRun.magic_ds_id
        AND magicDSFile.component = chipProcessedImfile.class_id
    WHERE
        magicDSRun.state = 'new'
        AND magicDSRun.stage = 'chip'
        AND chipProcessedImfile.quality = 0
    GROUP BY
        magic_ds_id,
        stage_id
    HAVING
        COUNT(chipProcessedImfile.class_id) = COUNT(magicDSFile.component)
        AND SUM(magicDSFile.fault) = 0
UNION
-- camera stage
SELECT
    magicDSRun.*,
    camRun.magicked
    FROM magicDSRun
    JOIN camRun ON stage_id = camRun.cam_id
    JOIN camProcessedExp ON camRun.cam_id = camProcessedExp.cam_id
    LEFT JOIN magicDSFile
        ON magicDSFile.magic_ds_id = magicDSRun.magic_ds_id
        AND magicDSFile.component IS NOT NULL
    WHERE
        magicDSRun.state = 'new'
        AND magicDSRun.stage = 'camera'
        AND camProcessedExp.quality = 0
        AND magicDSFile.fault = 0
    GROUP BY
        magic_ds_id,
        stage_id
UNION
-- warp stage
SELECT
    magicDSRun.*,
    warpRun.magicked
    FROM magicDSRun
    JOIN warpRun on stage_id = warp_id
    JOIN warpSkyfile USING(warp_id)
    LEFT JOIN magicDSFile
        ON magicDSRun.magic_ds_id = magicDSFile.magic_ds_id
        AND magicDSFile.component = warpSkyfile.skycell_id
    WHERE
        magicDSRun.state = 'new'
        AND magicDSRun.stage = 'warp'
        AND warpSkyfile.fault = 0
        AND warpSkyfile.quality = 0
    GROUP BY
        magicDSRun.magic_ds_id,
        stage_id
    HAVING
        COUNT(warpSkyfile.skycell_id) = COUNT(magicDSFile.component)
        AND SUM(magicDSFile.fault) = 0
UNION
-- diff stage
SELECT DISTINCT
    magicDSRun.*,
    diffRun.magicked
    FROM magicDSRun
    JOIN magicRun USING (magic_id)
    JOIN magicInputSkyfile USING(magic_id)
    JOIN diffRun
        ON magicRun.diff_id = diffRun.diff_id
    JOIN diffSkyfile
        ON diffRun.diff_id = diffSkyfile.diff_id
        AND magicInputSkyfile.node = diffSkyfile.skycell_id
    LEFT JOIN magicDSFile
        ON magicDSRun.magic_ds_id = magicDSFile.magic_ds_id
        AND magicDSFile.component = diffSkyfile.skycell_id
    WHERE
        magicDSRun.state = 'new'
        AND magicDSRun.stage = 'diff'
        AND diffSkyfile.fault = 0
        AND diffSkyfile.quality = 0
    GROUP BY
        magicDSRun.magic_ds_id,
        magicRun.magic_id
    HAVING
        COUNT(magicInputSkyfile.node) = COUNT(magicDSFile.component)
        AND SUM(magicDSFile.fault) = 0

   ) as magicDSRun
