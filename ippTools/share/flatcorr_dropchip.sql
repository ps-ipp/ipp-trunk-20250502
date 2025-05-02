
UPDATE flatcorrChipLink
  SET include = 0
WHERE corr_id = %lld 
  AND chip_id = %lld
