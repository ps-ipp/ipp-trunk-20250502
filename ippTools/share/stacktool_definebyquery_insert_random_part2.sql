-- This is the part 2 of 2 of a query to use a random set of warps for
-- the inputs to a defined stack.
-- stacktool_definebyquery_insert_random_part1.sql should be prepended.
    -- Sort by the random number, and take the first N
    -- to get a random set of N.
    ORDER BY rnd_num
    LIMIT @RANDOM_LIMIT@
) AS randomWarps
