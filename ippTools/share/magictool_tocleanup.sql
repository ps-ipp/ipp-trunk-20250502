SELECT magic_id,
    magicRun.exp_id,
    rawExp.camera,
    magicRun.workdir,
    IFNULL(priority, 10000) as priority
FROM magicRun 
    JOIN rawExp USING(exp_id)
    LEFT JOIN Label ON magicRun.label = Label.label
WHERE magicRun.workdir_state = 'goto_cleaned'
    AND (Label.active OR Label.active IS NULL)
