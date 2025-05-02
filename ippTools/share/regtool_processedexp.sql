SELECT
    *,
    DEGREES(ra) / 15.  AS rahours,
    DEGREES(ra) AS radeg,
    DEGREES(decl) AS decdeg
FROM rawExp
-- bogus where clause so there is already a where statement to append too
WHERE rawExp.exp_id IS NOT NULL

