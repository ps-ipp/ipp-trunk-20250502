#                Table 11: Skyprobe Line Absorption Table (sample entries)
# Column Name         Datatype Description
# Time                date/time   The time the LRProbe observation was taken.
# Disperser ID        string      ID of the dispersing element
# Atm Component 1 float           The strength of the 1st atmospheric component.
# Atm Component 2 float           The strength of the 2nd atmospheric component.
# Atm Component 3 float           The strength of the 3rd atmospheric component.
# Disperser ID        string      ID of the dispersing element
# Number of stars     int         Number of stars used to measure the absorptions.
# Astrometry          coords      The astrometry used on the LRProbe image.
# Exposure time       float       The exposure time of the LRProbe image.
# Sky brightness      float       The measured sky (surface) brightness, in physical units.

skyp_absorption METADATA
#    time        DATETIME    2006-01-10T00:00:00
    disperser_id    STR     255
    atmcomp1    F32         0.0
    atmcomp2    F32         0.0
    atmcomp3    F32         0.0
    nstars      S32         0
    ra          F64         0.0
    decl        F64         0.0
    exptime     F32         0.0
    sky_bright  F64         0.0
END
