SELECT relExp.*,
ippRelease.release_name,
ippRelease.release_state
FROM relExp JOIN ippRelease USING(rel_id)
WHERE relExp.state = 'goto_calib' AND relExp.fault = 0
