#             Table 9: Weather Table: some sample weather points
# Column Name Datatype Description
# Time            date/time    The time the weather information was measured.
# Temperature 01 float         The external temperature
# Temperature 02 float         The temperature at top of the dome
# Temperature 03 float         The temperature on the primary mirror
# Humidity        float        The relative humidity.
# Pressure        float        The (external) atmospheric pressure.

weather METADATA
#    time        DATETIME    2006-01-10T00:00:00 # Primary Key
    temp01      F32         0.0
    humi01      F32         0.0
    temp02      F32         0.0
    humi02      F32         0.0
    temp03      F32         0.0
    humi03      F32         0.0
    pressure    F32         0.0
END
