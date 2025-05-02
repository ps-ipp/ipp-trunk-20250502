# include "autocode.h"

int gfits_convert_$STRUCT ($STRUCT *data, off_t size, off_t nitems) {

  off_t i;
  unsigned char *byte, tmp;

  if (size != $SIZE) { 
    fprintf (stderr, "WARNING: mismatch in data types $STRUCT: "OFF_T_FMT" vs %d\n",  size,  $SIZE);
    return (FALSE);
  }

  /* provide initial values to avoid compiler warnings for non-BYTE_SWAP arch */
  i = tmp = 0;
  byte = NULL;

# ifdef BYTE_SWAP
  byte = (unsigned char *) data;
  for (i = 0; i < nitems; i++, byte += $SIZE) {
    /** BYTE SWAP **/
  }
# endif  

  return (TRUE);
} 

/*** add test of EXTNAME and header-defined columns? ***/
/* return internal structure representation */
/* scaledData & nativeBytes describe the current state of the ftable.buffer */
$STRUCT *gfits_table_get_$STRUCT (FTable *ftable, off_t *Ndata, char *scaledValue, char *nativeOrder) {

  int Ncols;
  $STRUCT *data;

  Ncols = ftable[0].header[0].Naxis[0];
  if (Ncols != $SIZE) {
    fprintf (stderr, "ERROR: mis-match in table size: width is %d but should be %d bytes\n", Ncols, $SIZE);
    return NULL;
  }

  *Ndata = ftable[0].header[0].Naxis[1];
  data = ($STRUCT *) ftable[0].buffer;

  // if the pointer is not passed, we need to swap
  if (!nativeOrder || !*nativeOrder) {
    if (!gfits_convert_$STRUCT (data, sizeof ($STRUCT), *Ndata)) {
      return NULL;
    }
    if (nativeOrder) *nativeOrder = TRUE;
  }
  // if the pointer is not passed, we need to scale
  if (!scaledValue || !*scaledValue) {
    gfits_table_scale_data (ftable);
    if (scaledValue) *scaledValue = TRUE;
  }
  return (data);
}

int gfits_table_set_$STRUCT (FTable *ftable, $STRUCT *data, off_t Ndata, int swapFromNative) {

  Header *header;

  header = ftable[0].header;

  /* create table header */
  if (!gfits_create_table_header (header, "$TYPE", "$EXTNAME")) return (FALSE);

  /* define table layout */
  /** TABLE DEFINITION **/

  /* create table */
  if (!gfits_create_table (header, ftable)) return (FALSE);

  /* add data values */
  if (!gfits_table_scale_data (ftable)) return (FALSE);
  if (swapFromNative) {
    if (!gfits_convert_$STRUCT (data, sizeof ($STRUCT), Ndata)) return (FALSE);
  }
  if (!gfits_add_rows (ftable, (char *) data, Ndata, sizeof ($STRUCT))) return (FALSE);

  return (TRUE);
}

int gfits_table_mkheader_$STRUCT (Header *header) {

  /* create table header */
  if (!gfits_create_table_header (header, "$TYPE", "$EXTNAME")) return (FALSE);

  /* define table layout */
  /** TABLE DEFINITION **/

  return (TRUE);
}

int Send_$STRUCT (int device, $STRUCT *data, int Ndata, int copy) {

  int Nwrite, Nbytes;
  $STRUCT *tmpdata;

  Nbytes = Ndata * sizeof ($STRUCT);

  if (copy) {
    ALLOCATE (tmpdata, $STRUCT, Ndata);
    memcpy (tmpdata, data, Nbytes);
  } else {
    tmpdata = data;
  }

  if (!gfits_convert_$STRUCT (tmpdata, sizeof ($STRUCT), Ndata)) return (FALSE);

  SendCommand (device, 16, "NVALUE: %6d", Ndata);
  SendCommand (device, 16, "NBYTES: %6d", Nbytes);
  Nwrite = write (device, tmpdata, Nbytes);
  if (Nwrite != Nbytes) {
    return (FALSE);
  }
  
  /* perform handshaking? */

  return (TRUE);
}

int Recv_$STRUCT (int device, $STRUCT **data, int *Ndata) {

  int ndata;
  IOBuffer message;
  $STRUCT *tmpdata;

  ExpectCommand (device, 16, 1.0, &message);
  sscanf (message.buffer, "%*s %d", &ndata);
  FreeIOBuffer (&message);
  
  /* what is reasonable for timeout? */
  ExpectMessage (device, 1.0, &message);
  
  tmpdata = ($STRUCT *) message.buffer;
  if (!gfits_convert_$STRUCT (tmpdata, sizeof ($STRUCT), ndata)) return (FALSE);

  /* double-check data length? */
  /* perform handshaking? */

  *Ndata = ndata;
  *data = tmpdata;

  return (TRUE);
}
