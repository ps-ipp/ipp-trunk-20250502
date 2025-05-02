UPDATE vpRun
    SET fault = 0
WHERE vpRun.state = 'new'
    AND fault > 0
