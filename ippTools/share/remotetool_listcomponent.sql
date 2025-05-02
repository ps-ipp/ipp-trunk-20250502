SELECT DISTINCT
    remoteComponent.remote_id,
    remoteComponent.stage_id,
    remoteComponent.jobs,
    remoteComponent.state,
    remoteComponent.path_base,
    remoteRun.stage,
    remoteRun.path_base as run_path_base
FROM remoteRun
JOIN remoteComponent
    USING(remote_id)
