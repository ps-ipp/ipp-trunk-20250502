UPDATE distRun
SET distRun.fault = 0
WHERE (distRun.state = 'new' OR distRun.state = 'goto_cleaned')
    AND distRun.fault != 0
