SELECT
    fakeRun.*
FROM warpRun
JOIN fakeRun
    USING(fake_id)
WHERE
    warpRun.state = 'new'
    AND fakeRun.state = 'full'
