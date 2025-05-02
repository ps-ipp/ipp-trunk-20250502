DELETE FROM magicDSFile USING magicDSFile, magicDSRun  
WHERE (magicDSRun.magic_ds_id = magicDSFile.magic_ds_id) 
    AND ((magicDSRun.state = 'new' AND magicDSFile.fault != 0)
      OR (magicDSRun.state = 'goto_restored'
          OR magicDSRun.state = 'goto_censored'
         )
    )
