SELECT * FROM (
-- rawExp magicked with re_place
SELECT
    distRun.dist_id,
    distRun.label,
    distTarget.dist_group,
    'raw' AS stage,
    rawExp.exp_id AS stage_id,
    rawImfile.class_id AS component,
    rawExp.exp_type,
    clean,
    rawExp.camera,
    CONCAT_WS('.', distRun.outroot, CONVERT(distRun.dist_id, CHAR)) as outdir,
    -- XXX: replace this with rawImfile.path_base once it exists
    TRIM(TRAILING '.fits' FROM rawImfile.uri) AS path_base,
    CAST(NULL AS CHAR(255)) AS alt_path_base,
    -- pass camera stage path base since we want the camera mask file. The script knows what to do
    camProcessedExp.path_base as chip_path_base,
    CAST(NULL AS CHAR(255)) AS state,
    CAST(NULL AS CHAR(255)) AS data_state,
    0 as quality,
    distRun.no_magic,
    rawImfile.magicked,
    IFNULL(Label.priority, 10000) AS priority
FROM distRun
JOIN distTarget USING(target_id, stage, clean)
JOIN rawExp ON rawExp.exp_id = stage_id AND distTarget.stage = 'raw' AND distRun.alternate = 0
JOIN rawImfile USING(exp_id)
JOIN chipRun ON chipRun.exp_id = rawExp.exp_id AND chipRun.magicked
JOIN chipProcessedImfile
    USING(chip_id, class_id)
JOIN camRun USING(chip_id)
JOIN camProcessedExp using(cam_id)
LEFT JOIN distComponent 
    ON distRun.dist_id = distComponent.dist_id 
    AND rawImfile.class_id = distComponent.component
LEFT JOIN Label ON distRun.label = Label.label
WHERE
    distRun.state = 'new'
    AND distRun.clean = 0
    AND distRun.stage = 'raw'
    AND distComponent.dist_id IS NULL
    -- AND (rawExp.magicked OR distRun.no_magic)
    AND rawExp.magicked
    AND rawImfile.ignored = 0
    -- need to have magicked the chip image which makes the camera mask
    AND chipProcessedImfile.magicked != 0
    AND camRun.magicked > 0 
    AND (Label.active OR Label.active IS NULL)
UNION
SELECT
    -- raw images no_magic required
    distRun.dist_id,
    distRun.label,
    distTarget.dist_group,
    'raw' AS stage,
    rawExp.exp_id AS stage_id,
    rawImfile.class_id AS component,
    rawExp.exp_type,
    clean,
    rawExp.camera,
    CONCAT_WS('.', distRun.outroot, CONVERT(distRun.dist_id, CHAR)) as outdir,
    -- XXX: replace this with rawImfile.path_base once it exists
    TRIM(TRAILING '.fits' FROM rawImfile.uri) AS path_base,
    CAST(NULL AS CHAR(255)) AS alt_path_base,
    -- pass camera stage path base since we want the camera mask file. The script knows what to do
    CAST(NULL AS CHAR(255)) AS chip_path_base,
    CAST(NULL AS CHAR(255)) AS state,
    CAST(NULL AS CHAR(255)) AS data_state,
    0 as quality,
    distRun.no_magic,
    rawImfile.magicked,
    IFNULL(Label.priority, 10000) AS priority
FROM distRun
JOIN distTarget USING(target_id, stage, clean)
JOIN rawExp ON rawExp.exp_id = stage_id 
            AND distTarget.stage = 'raw' AND distRun.alternate = 0
JOIN rawImfile USING(exp_id)
LEFT JOIN distComponent 
    ON distRun.dist_id = distComponent.dist_id 
    AND rawImfile.class_id = distComponent.component
LEFT JOIN Label ON distRun.label = Label.label
WHERE
    distRun.state = 'new'
    AND distRun.clean = 0
    AND distRun.stage = 'raw'
    AND distComponent.dist_id IS NULL
    AND (distRun.no_magic)
    AND rawImfile.ignored = 0
    AND (Label.active OR Label.active IS NULL)
UNION
    -- raw stage alternate inputs
SELECT
    distRun.dist_id,
    distRun.label,
    distTarget.dist_group,
    'raw' AS stage,
    rawExp.exp_id AS stage_id,
    rawImfile.class_id AS component,
    rawExp.exp_type,
    clean,
    rawExp.camera,
    CONCAT_WS('.', distRun.outroot, CONVERT(distRun.dist_id, CHAR)) as outdir,
    -- XXX: replace this with rawImfile.path_base once it exists
    TRIM(TRAILING '.fits' FROM rawImfile.uri) AS path_base,
    magicDSFile.backup_path_base AS alt_path_base,
    -- pass camera stage path base since we want the camera mask file. The script knows what to do
    camProcessedExp.path_base AS chip_path_base,
    CAST('full' AS CHAR(255)) AS state,
    CAST('full' AS CHAR(255)) AS data_state,
    0 as quality,
    distRun.no_magic,
    rawImfile.magicked,
    IFNULL(Label.priority, 10000) AS priority
FROM distRun
JOIN distTarget USING(target_id, stage, clean)
JOIN rawExp ON rawExp.exp_id = stage_id AND distTarget.stage = 'raw' AND distRun.alternate = 1
JOIN rawImfile USING(exp_id)
JOIN magicDSRun ON magicDSRun.stage = 'raw' AND magicDSRun.stage_id = rawExp.exp_id
JOIN magicDSFile ON magicDSRun.magic_ds_id = magicDSFile.magic_ds_id 
                AND rawImfile.class_id = magicDSFile.component
JOIN camRun USING(cam_id)
JOIN camProcessedExp USING(cam_id)
JOIN chipProcessedImfile USING(chip_id, class_id)
LEFT JOIN distComponent 
    ON distRun.dist_id = distComponent.dist_id 
    AND rawImfile.class_id = distComponent.component
LEFT JOIN Label ON distRun.label = Label.label
WHERE
    distRun.state = 'new'
    AND distRun.clean = 0
    AND distComponent.dist_id IS NULL
    AND chipProcessedImfile.magicked != 0
    AND camRun.magicked > 0 
    AND rawImfile.ignored = 0
    AND (Label.active OR Label.active IS NULL)
UNION
    -- raw stage clean (dbinfo only)
SELECT
    distRun.dist_id,
    distRun.label,
    distTarget.dist_group,
    'raw' AS stage,
    rawExp.exp_id AS stage_id,
    'exposure' AS component,
    rawExp.exp_type,
    clean,
    rawExp.camera,
    CONCAT_WS('.', distRun.outroot, CONVERT(distRun.dist_id, CHAR)) as outdir,
    CAST(NULL AS CHAR(255)) AS path_base,
    CAST(NULL AS CHAR(255)) AS alt_path_base,
    CAST(NULL AS CHAR(255)) AS chip_path_base,
    CAST(NULL AS CHAR(255)) AS state,
    CAST(NULL AS CHAR(255)) AS data_state,
    0 as quality,
    distRun.no_magic,
    rawExp.magicked,
    IFNULL(Label.priority, 10000) AS priority
FROM distRun
JOIN distTarget USING(target_id, stage, clean)
JOIN rawExp ON rawExp.exp_id = stage_id
LEFT JOIN distComponent 
    ON distRun.dist_id = distComponent.dist_id 
LEFT JOIN Label ON distRun.label = Label.label
WHERE
    distRun.state = 'new'
    AND distRun.stage = 'raw'
    AND distRun.clean
    AND distComponent.dist_id IS NULL
    AND (Label.active OR Label.active IS NULL)
) AS distRun
WHERE 1
