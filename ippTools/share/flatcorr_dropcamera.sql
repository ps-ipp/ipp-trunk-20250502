
UPDATE flatcorrCamLink
  SET include = 0
WHERE corr_id = %lld 
  AND cam_id = %lld
