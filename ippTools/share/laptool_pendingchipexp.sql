SELECT re_id,projection_cell,registered,reprocRun.state as reproc_state, tess_id,
  exp_id,private,active,
  chip_id,chipRun.state AS chip_state,
  cam_id,camRun.state AS cam_state,
  fake_id,fakeRun.state AS fake_state,
  warp_id,warpRun.state AS warp_state
  FROM reprocRun JOIN reprocExp USING(re_id)
  LEFT JOIN chipRun USING(chip_id,tess_id)
  LEFT JOIN camRun  USING(chip_id,tess_id)
  LEFT JOIN fakeRun USING(fake_id,tess_id)
  LEFT JOIN warpRun USING(fake_id,tess_id)
