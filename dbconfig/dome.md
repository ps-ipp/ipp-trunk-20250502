#                     Table 15: Dome Status Table
# Column Name   Datatype Description
# Time          date/time     The time for which the dome status is valid.
# Azimuth       float         The azimuth of the dome.
# Open status   boolean       Whether the dome is open or not.
# Lights status boolean       Whether lights are on in the dome or not.
# Track status  boolean       Whether dome is tracking telescope or not.

dome METADATA
#    time        DATETIME    2006-01-11T00:00:00
    az          F32         0.0
    open        BOOL        t
    light       BOOL        t
    # XXX is it possible for the dome slit to not track the telescope? ;)
    track       BOOL        t
END
