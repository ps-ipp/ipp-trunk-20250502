SELECT 
    dist_id,
    target_id,
    stage,
    stage_id,
    component,
    distComponent.outdir,
    bytes,
    md5sum,
    name,
    distComponent.state,
    no_magic,
    distComponent.fault
FROM    distRun
JOIN distComponent USING(dist_id)
