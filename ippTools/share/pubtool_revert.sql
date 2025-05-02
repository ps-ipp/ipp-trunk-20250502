DELETE FROM publishDone
USING publishDone, publishRun, publishClient
WHERE publishDone.pub_id = publishRun.pub_id
    AND publishRun.state = 'new'
    AND publishRun.client_id = publishClient.client_id
    AND publishClient.active = 1
    AND publishDone.fault != 0
