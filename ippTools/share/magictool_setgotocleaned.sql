UPDATE magicRun 
    SET workdir_state = 'goto_cleaned'
WHERE workdir_state = 'dirty'

