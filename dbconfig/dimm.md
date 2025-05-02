#              Table 13: DIMM Measurements Table
# Column Name Datatype Description
# Time          date/time   The time the DIMM observation was taken.
# sigmax        float Raw dispersion in x.
# sigmay        float Raw dispersion in y.
# FWHM          float  Dervied seeing full width at half maximum.
# RA            float  The coordinates of the measured star.
# DEC           float  The coordinates of the measured star.
# Exposure time float  The exposure time of the DIMM observation.
# Telescope ID  string source of the DIMM data

dimm METADATA
#    time        DATETIME    2006-01-11T00:00:00
    sigmax      F32         0.0
    sigmay      F32         0.0
    fwhm        F32         0.0
    ra          F64         0.0
    decl        F64         0.0
    expttime    F32         0.0
    telescope_id    STR     255
END
