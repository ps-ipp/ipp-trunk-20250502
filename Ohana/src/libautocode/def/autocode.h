
/** STRUCT DEFINITION **/

$STRUCT *gfits_table_get_$STRUCT (FTable *table, off_t *Ndata, char *scaledValue, char *nativeOrder);
int      gfits_table_set_$STRUCT (FTable *ftable, $STRUCT *data, off_t Ndata, int swapFromNative);
int      gfits_table_mkheader_$STRUCT (Header *header);
int      gfits_convert_$STRUCT ($STRUCT *data, off_t size, off_t nitems);
int      Send_$STRUCT (int device, $STRUCT *data, int Ndata, int copy);
int      Recv_$STRUCT (int device, $STRUCT **data, int *Ndata);
