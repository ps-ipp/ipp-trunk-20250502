INSERT INTO addRun
    SELECT
        0,              -- add_id
        'fullforce_summary',		-- stage
        ff_id,         -- stage_id
        %d,	       -- stage_extra1
        '%s',           -- state
        '%s',           -- workdir
	'%s',           -- workdir_state
        '%s',           -- reduction
        '%s',           -- label
        '%s',           -- data_group
        '%s',           -- dvodb 
        '%s',           -- note
	%d,		-- image_only
	%d,		-- minidvodb
	'%s',           -- minidvodb_group 
 	'%s',	        -- minidvodb_name
 	'%s'	        -- minidvodb_host
    FROM fullForceRun
    WHERE
        fullForceRun.state = 'full'
        AND fullForceRun.ff_id = %lld

