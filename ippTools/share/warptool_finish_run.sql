UPDATE warpRun
    SET state = 'full',
    magicked = %lld
WHERE warp_id = %lld

