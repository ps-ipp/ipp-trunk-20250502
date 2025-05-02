-- fpcamtool only operates on exposures so we can safely queue more then one at a
-- time without worrying about losing the track of the generated cam_id
INSERT INTO fpcamRun
  ( fpcam_id,
    chip_id,
    cam_id,
    state,
    workdir,
    workdir_state,
    label,
    data_group,
    dist_group,
    reduction,
    dvodb,
    software_ver,
    note)
  VALUES
  (
        0,              -- fpcam_id
	%lld,           -- chip_id
        %lld,           -- cam_id
        '%s',           -- state
        '%s',           -- workdir
        '%s',           -- workdir_state
        '%s',           -- label
        '%s',           -- data_group
        '%s',           -- dist_group
        '%s',           -- reduction
        '%s',           -- dvodb
        0,           -- software ver
        '%s'            -- note           
  )
