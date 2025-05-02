#                      Table 16: Telescope Status
# Column Name  Datatype Description
# Time         date/time   The time for which the telescope status is valid.
# Guide status enum        The status of the guiding.
# Altitude     float       The telescope altitude.
# Azimuth      float       The telescope azimuth.
# RA           float The telescope Right Ascension (ICRS ~ J2000).
# Dec          float The telescope Declination (ICRS ~ J2000).

telescope METADATA
#    time        DATETIME    2006-01-11T00:00:00 # Primary Key
# XXX there is currently no way to declare an enum - use str or int instead?
    guide       STR         255
    alt         F32         0.0
    az          F32         0.0
    ra          F64         0.0
    decl        F64         0.0
END
