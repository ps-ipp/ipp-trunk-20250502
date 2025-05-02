-- Get input details for a magic node
SELECT
    magicRun.magic_id,
    magicRun.inverse, -- Using the inverse subtraction?
    magicRun.diff_id,
    magicInputs.node,
    -- Only one of diff_path_base and magic_path_base should be non-NULL
    -- If diff_path_base is non-NULL, then only one of warp_path_base and stack_path_base should be non-NULL
    magicInputs.diff_path_base, -- path_base for the diff (if any)
    magicInputs.warp_path_base, -- path_base for the template warp (if any)
    magicInputs.stack_path_base, -- path_base for the template stack (if any)
    magicInputs.magic_path_base -- path_base for child nodes (if any)
FROM magicRun
JOIN (
    -- Single skycells: have uri=NULL
    SELECT
        magic_id,
        magicRun.diff_id,
        skycell_id AS node,
        diffSkyfile.path_base AS diff_path_base,
        warpSkyfile.path_base AS warp_path_base,
        stackSumSkyfile.path_base AS stack_path_base,
        NULL AS magic_path_base
    FROM magicRun
    JOIN magicTree USING(magic_id)
    JOIN magicInputSkyfile USING(magic_id, node)
    JOIN diffRun USING(diff_id)
    JOIN diffSkyfile
        ON diffSkyfile.diff_id = magicRun.diff_id
        AND diffSkyfile.skycell_id = magicInputSkyfile.node
    JOIN (
        -- Template for non-inverse
        SELECT
            magic_id,
            skycell_id,
            warp2 AS warp_id,
            stack2 AS stack_id
        FROM magicRun
        JOIN diffInputSkyfile USING(diff_id)
        WHERE magicRun.inverse = 0
            -- WHERE hook (magicRun.magic_id, diffInputSkyfile.skycell_id) %s
        UNION
        -- Template for inverse
        SELECT
            magic_id,
            skycell_id,
            warp1 AS warp_id,
            stack1 AS stack_id
        FROM magicRun
        JOIN diffInputSkyfile USING(diff_id)
        WHERE magicRun.inverse = 1
            -- WHERE hook (magicRun.magic_id, diffInputSkyfile.skycell_id) %s
        ) AS diffTemplates USING(magic_id, skycell_id)
    LEFT JOIN warpSkyfile USING(warp_id, skycell_id)
    LEFT JOIN stackSumSkyfile USING(stack_id)
    -- WHERE hook (magicRun.magic_id, magicTree.node) %s
    UNION
    -- Merged skycells: have diff_id=0, various_path_base=NULL
    SELECT
        magicTree.magic_id,
        0 AS diff_id,
        magicTree.dep,
        NULL AS diff_path_base,
        NULL AS warp_path_base,
        NULL AS stack_path_base,
        magicNodeResult.path_base AS magic_path_base
    FROM magicTree
    JOIN magicRun USING(magic_id)
    JOIN magicNodeResult
        ON magicTree.magic_id = magicNodeResult.magic_id
        AND magicTree.dep = magicNodeResult.node
    -- WHERE hook (magicRun.magic_id, magicTree.node) %s
    ) AS magicInputs USING(magic_id)
