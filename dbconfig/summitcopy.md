pzDataStore METADATA
    camera      STR         64      # Primary Key
    telescope   STR         64      # Primary Key
    uri         STR         255
    epoch       UTC         0001-01-01T00:00:00Z
    use_compress S16        0
END

# list of source exposures -- updated as exposures are seen
# summitExp.imfiles is updated as filesets are queried default value should be
# -1 as NULL or 0 might be a valid value (empty fileset)
summitExp METADATA
    summit_id   S64         0       # Primary Key AUTO_INCREMENT
    exp_name    STR         64      # Key
    camera      STR         64      # Key
    telescope   STR         64      # Key
    dateobs     UTC         NULL
    exp_type    STR         64
    uri         STR         255
    imfiles     S32         0
    fault       S16         0       # Key NOT NULL
    epoch       UTC         0001-01-01T00:00:00Z
END

# class == type of file
# class_id == type set id
# list of source images -- updated as exposures/filesets are queried
summitImfile METADATA
    summit_id   S64         0       # Primary Key fkey(summit_id) ref summitExp(summit_id)
    exp_name    STR         64      # Key fkey(exp_name, camera, telescope) ref summitExp(exp_name, camera, telescope)
    camera      STR         64      # Key
    telescope   STR         64      # Key
    file_id     STR         64      # Key
    bytes       S32         0
    md5sum      STR         32
    class       STR         64      # Primary Key
    class_id    STR         64      # Primary Key
    uri         STR         255
    epoch       UTC         0001-01-01T00:00:00Z
END

# list of exposures that have had their imfiles/files registered (but not
# downloaded) 
pzDownloadExp METADATA
    summit_id   S64         0       # Primary Key fkey(summit_id) ref summitExp(summit_id)
    exp_name    STR         64      # Key fkey(exp_name, camera, telescope) ref summitExp(exp_name, camera, telescope)
    camera      STR         64      # Key
    telescope   STR         64      # Key
    state       STR         64      # Key 
    epoch       UTC         0001-01-01T00:00:00Z
END

pzDownloadImfile METADATA
    summit_id   S64         0       # Primary Key fkey(summit_id) ref pzDownloadExp(summit_id)
    exp_name    STR         64      # Key fkey(exp_name, camera, telescope) ref pzDownloadExp(exp_name, camera, telescope)
    camera      STR         64      # Key fkey(exp_name, camera, telescope, class, class_id) ref summitImfile(exp_name, camera, telescope, class, class_id)
    telescope   STR         64      # Key fkey(exp_name, camera, telescope) ref summitExp(exp_name, camera, telescope)
    class       STR         64      # Primary Key fkey(summit_id,class,class_id) ref summitImfile(summit_id,class,class_id)
    class_id    STR         64      # Primary Key
    uri         STR         255
    fault       S16         0       # Key NOT NULL
    epoch       UTC         0001-01-01T00:00:00Z
    hostname    STR         64
    bytes       S32         0
    md5sum      STR         32
END

