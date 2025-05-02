INSERT INTO addRun
    SELECT
        0,              -- add_id
        'stack',		-- stage
        stack_id,         -- stage_id
	%d,		  --stage_extra1
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
    FROM stackRun
    WHERE
        stackRun.state = 'full'
        AND stackRun.stack_id = %lld
