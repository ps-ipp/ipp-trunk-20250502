-- create rcRun's for all destinations that have filesets that they are interested in available
INSERT INTO minidvodbRun
SELECT                  -- rows in this select must match rcRun
    minidvodb_name,
    minidvodb_path,
    'new'

FROM addRun 
LEFT JOIN minidvodbRun USING(minidvodb_name)
-- WHERE minidvodbRun.minidvodb_name IS NULL
  
