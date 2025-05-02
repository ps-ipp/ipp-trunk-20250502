#             Table 12: Skyprobe Line Emission Table (sample entries)
# Column Name         Datatype Description
# Time                date/time   The time the LRProbe observation was taken.
# Disperser ID        string      ID of the dispersing element
# Atm Component 1 float           The strength of the 1st atmospheric component.
# Atm Component 2 float           The strength of the 2nd atmospheric component.
# Atm Component 3 float           The strength of the 3rd atmospheric component.
# Continuum           float       The strength of the continuum emission.
# Disperser ID        string      ID of the dispersing element
# Exposure time       float       The exposure time of the LRProbe image.

skyp_emission METADATA
#    time        DATETIME    2006-01-11T00:00:00
    disperser_id    STR     255
    atmcomp1    F32         0.0
    atmcomp2    F32         0.0
    atmcomp3    F32         0.0
    continuum   F32         0.0
    exptime     F32         0.0
END
