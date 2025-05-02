-- camtool only operates on exposures so we can safely queue more then one at a
-- time without worrying about losing the track of the generated cam_id
INSERT INTO camRun
    SElECT
        0,              -- cam_id
        chip_id,        -- chip_id
        '%s',           -- state
        '%s',           -- workdir
        '%s',           -- workdir_state
        '%s',           -- label
        '%s',           -- data_group
        '%s',           -- dist_group
        '%s',           -- reduction
        '%s',           -- expgroup
        '%s',           -- dvodb
        '%s',           -- tess_id
        '%s',           -- end_stage
        %lld,           -- magicked
        '%s',           -- software ver
        '%s',           -- maskfrac_ref_npix
        '%s',           --         _ref_static
        '%s',           --         _ref_dynamic
        '%s',           --         _ref_magic
        '%s',           --         _ref_advisory
        '%s',           -- maskfrac_max_npix
        '%s',           --         _max_static
        '%s',           --         _max_dynamic
        '%s',           --         _max_magic
        '%s',           --         _max_advisory
        '%s'            -- note
    FROM chipRun
    WHERE
        chipRun.state = 'full'
        AND chipRun.chip_id = %lld
