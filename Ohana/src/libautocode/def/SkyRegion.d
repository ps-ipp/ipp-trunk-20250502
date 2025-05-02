STRUCT  SkyRegion
EXTNAME SKY_REGION
TYPE    BINTABLE
SIZE    56

FIELD   Rmin,	   R_MIN,	   float, 
FIELD   Rmax,	   R_MAX,	   float, 
FIELD   Dmin,	   D_MIN,	   float, 
FIELD   Dmax,	   D_MAX,	   float, 
FIELD   childS,	   CHILD_S,	   int,            sequence number in full table of first child
FIELD   childE,	   CHILD_E,	   int,            sequence number in full table of last child + 1
FIELD   parent,	   PARENT,	   int,            sequence number in full table of parent
FIELD   index,     INDEX,          int,            sequence number in full table of this entry
FIELD   depth,	   DEPTH,	   gfbyte,         depth of this entry
FIELD   child,	   CHILD,	   gfbyte,         does this entry have children?
FIELD   table,	   TABLE,	   gfbyte,         does this entry have a table?
FIELD   name,      NAME,           char[18],       name / filename
FIELD   hostFlags, HOST_FLAGS,     gfbyte,         flags to define host / backup usage
FIELD   hostID,    HOST_ID,        gfbyte,	   host ID where data is stored
FIELD   backupID,  BACKUP_ID,      gfbyte,	   host ID where backup is stored

# note : 2012.02.05 : stole 3 bytes from 'name' to use for host ID and
#        related (no DBs were created with more than 17 bytes in the
#        name).  The hostID points at an entry in the host table (text
#        file, with name given in SkyTable PHU header).  There is no
#        safety on this, but it is recoverable (by examining the
#        directories) if misplaced.
 
#        Note that SkyTables from databases generated in the past may
#        have garbage characters here.  To ensure catch this
#        condition, we add a header keyword to SkyTables in which the
#        hostID has been set; if that is not present, the SkyTable
#        loading software should init these bytes to 0 (== unassigned).
