UPDATE rawImfile
    SET %s
  WHERE
    rawImfile.exp_id = %lld
  AND
    rawImfile.class_id = '%s'
