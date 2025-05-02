INSERT INTO addRun
    SELECT
        0,              -- add_id
        'diff',		-- stage
        diff_id,        -- stage_id
        %d, -- stage_extra1
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
 	'%s'	        -- minidvodb_name
    FROM diffRun
    WHERE
        diffRun.state = 'full' 
        AND diffRun.diff_id = %lld
        
