#                Table 10: SkyProbe Transparency Table (sample entries)
# Column Name Datatype Description
# Time              date/time   The time the SkyProbe image was taken.
# Filter            string      Filter used for SkyProbe image.
# Transparency      float       The derived transparency.
# Number of stars int           The number of stars used to measure the transparency.
# Astrometry        coords      The astrometry used on the SkyProbe image.
# Exposure time     float       The exposure time of the SkyProbe image.
# Sky brightness    float       The measured sky (surface) brightness, counts / second

skyp_transparency METADATA
#    time        DATETIME    2006-01-10T00:00:00    # Primary Key
    filter      STR         255
    trans       F64         0.0
    nstars      S32         0
#    astrom
    ra          F64         0.0
    decl        F64         0.0
    exptime     F32         0.0
    sky_bright  F64         0.0
END
