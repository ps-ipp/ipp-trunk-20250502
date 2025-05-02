SELECT 
    skycell.tess_id,
    skycell.skycell_id,
    TRUNCATE(skycell.radeg/15., 4) AS rahours,
    skycell.radeg,
    skycell.decdeg,
    skycell.glong,
    skycell.glat,
    skycell.width,
    skycell.height
FROM skycell
