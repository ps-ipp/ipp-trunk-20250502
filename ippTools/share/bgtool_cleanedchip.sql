UPDATE chipBackgroundRun
SET state = '%s'
WHERE state IN ('goto_cleaned', 'goto_scrubbed', 'goto_purged')
