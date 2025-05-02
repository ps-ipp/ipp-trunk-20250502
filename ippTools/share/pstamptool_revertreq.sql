UPDATE pstampRequest
    SET fault = 0
WHERE pstampRequest.state != 'stop'
    
