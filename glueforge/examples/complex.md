glueforge METADATA
    pkg_name        STR  foodb
    pkg_namespace   STR  foo
END

foo METADATA
    foo     STR     60      # Primary Key # the name of foo thing
    bar     S32     0       ## count of bar
    baz     F32     0.0
    boing   F64     0.0
    zot     BOOL    t       # Key
END

bar METADATA
    zot     BOOL    t       # Key
    boing   F64     0.0
    baz     F32     0.0
    bar     S32     0       ## count of bar
    foo     STR     60      # Primary Key # the name of foo thing
END
