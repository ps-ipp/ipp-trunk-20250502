DELETE FROM distComponent
USING distComponent, distRun
WHERE distComponent.dist_id = distRun.dist_id
    AND distRun.state = 'new'
    AND distComponent.fault != 0
