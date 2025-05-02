/* FITS specific macros and structures */

# include <assert.h>
# include <errno.h>

# ifndef GFITSIO
# define GFITSIO

# ifndef BYTE_SWAP
# ifndef NOT_BYTE_SWAP
# error "neither BYTE_SWAP not NOT_BYTE_SWAP is set"
# endif
# endif

/* if we are not correctly including the ohana headers, this will fail */
# ifndef NEWLINE
# define NEWLINE                 10  /* UNIX RETURN character */
# endif /* NEWLINE */

/* gfits_bintable_format returns 'gfbyte' as the type associated with one-byte logical (non-char) values */
typedef unsigned char gfbyte;

/********** FITS Constants *********/
# define FT_TEXT_LENGTH          18  /* max length text header field */
# define FT_MAX_NAXES            10  /* max number of axes */
# define FT_FIELD_LENGTH          8  /* max length header field */   
# define FT_COMMENT_LENGTH       47  /* max length comment field */
# define FT_HISTORY_LENGTH       72  /* max length history / comment */
# define FT_LINE_LENGTH          80  /* FITS header line length */
# define FT_RECORD_SIZE        2880  /* FITS block size */

# define FT_BZERO_INT8 -1.0*0x80
# define FT_BZERO_INT16 1.0*0x8000	  
# define FT_BZERO_INT32 1.0*0x80000000
# define FT_BZERO_INT64 1.0*0x8000000000000000

/* this structure defines the buffer which contains a header
   and contains the minimum required keywords */

typedef struct {
  int                     simple; // T or F
  int                     unsign; // T or F
  int                     extend; // T or F
  int                     bitpix; // 8, 16, 32, 64, or -32, -64
  int                     Naxes;  // < FT_MAX_NAXES (10)
  off_t                   Naxis[FT_MAX_NAXES];
  off_t                   datasize;
  off_t                   pcount;
  int                     gcount;
  double                  bzero;
  double                  bscale;
  char                   *buffer;
} Header;

/* this structure defines the buffer which contains a matrix
   and contains some relavant info */

typedef struct {
  int                     unsign;
  int                     bitpix;
  int                     Naxes;
  off_t                   Naxis[FT_MAX_NAXES];
  off_t                   datasize;
  double                  bzero;
  double                  bscale;
  char                   *buffer;
} Matrix;
   
/* the FTable represents a complete table on disk */          
typedef struct {
  Header                 *header;
  char                   *buffer;
  off_t                   datasize; // size of the buffer (including block padding at the end)
  off_t                   validsize;  // size of the valid portion of the table (< validsize if file is short)
  off_t                   heap_start; // byte offset to start of HEAP
} FTable;

/* the VTable represents rows from a table on disk */          
typedef struct {
  Header                 *header;
  char                  **buffer;
  off_t                   Nrow;
  off_t                  *row;
  off_t                   datasize;  /* total buffer size */
  off_t                   pad;   /* bytes of padding at the end */
} VTable;

// description / metadata needed for a single varlength column.  A varlength column has a
// 2-element array giving the (length,offset) of data in the heap.  Below, 'real data' are
// the values in the heap; 'metadata' are the values in the main table.
typedef struct {
  int   column;		      // field number (of the metadata)
  char  format;		      // format character for real data (one of: XLABIJKEDCM)
  char  mode;		      // format character for metadata (one of P [float] or Q [double])
  int   maxlen; 	      // max size of all table rows for this column
  int   nbytes;		      // number of bytes per real data column element 
  int   offset;		      // byte offset of the metadata column relative to first column
} VarLengthColumn;

typedef struct {
  char ttype[256]; 	      // TTYPE field of original table
  char ttype_cmt[256];	      // comment associated with TTYPE
  char tunit[256];	      // TUNIT field of original table
  char tunit_cmt[256];	      // comment associated with TUNIT
  char tformat[256];	      // TFORM field of original table
  char tformat_cmt[256];      // comment associated with TFORM
  char zctype[256];	      // compression type for this field
  VarLengthColumn zdef;	      // description of the output variable length column

  char datatype[256];	      // named data type associated with ttype (eg, 18J -> int, 18D -> double)
  int Nvalues;		      // number of values per row (eg 18J -> 18 int values)
  int pixsize;		      // number of bytes per value of this row (eg, 18J -> 4 bytes)
  int rowsize;		      // number of bytes for a full row (Nvalues * pixsize)
  int offset;		      // byte offset of the real data column relative to first column
} TableField;

# ifndef PROTO
# define PROTO(A) A
# endif

char   *gfits_version                  PROTO((void));

/******************************* Header functions *************/

Header *gfits_alloc_header             PROTO((void));
int     gfits_init_header              PROTO((Header *header));
void    gfits_free_header              PROTO((Header *header)); 
char   *gfits_header_field             PROTO((Header *header, char *field, int N)); 
char   *gfits_header_lineno            PROTO((Header *header, off_t N)); 
int     gfits_header_append_line_raw   PROTO((Header *header, char *line));
char   *gfits_keyword_end              PROTO((char *line));
int     gfits_copy_header              PROTO((Header *in, Header *out)); 
int     gfits_copy_header_ptr          PROTO((Header *in, Header *out)); 
int     gfits_create_header            PROTO((Header *header)); 
int     gfits_delete                   PROTO((Header *header, char *field, int N)); 
int     gfits_fread_Xheader            PROTO((FILE *f, Header *header, int N));
int     gfits_fread_header             PROTO((FILE *f, Header *header));
int     gfits_fwrite_header            PROTO((FILE *f, Header *header)); 
int     gfits_get_unsign_mode          PROTO((void));
int     gfits_load_header              PROTO((FILE *f, Header *header));
int     gfits_modify                   PROTO((Header *header, char *field, char *mode, int N,...)) OHANA_FORMAT(printf, 3, 5); 
int     gfits_print                    PROTO((Header *header, char *field, char *mode, int N,...)) OHANA_FORMAT(printf, 3, 5); 
int     gfits_modify_alt               PROTO((Header *header, char *field, char *mode, int N,...)); // do not use a FORMAT: non-standard fmt chars
int     gfits_print_alt                PROTO((Header *header, char *field, char *mode, int N,...)); // do not use a FORMAT: non-standard fmt chars
int     gfits_find_Xheader             PROTO((FILE *f, Header *header, char *extname));
int     gfits_read_Xheader             PROTO((char *filename, Header *header, int N));
int     gfits_read_header              PROTO((char *filename, Header *header));
int     gfits_save_header              PROTO((FILE *f, Header *header));
int     gfits_scan                     PROTO((Header *header, char *field, char *mode, int N,...)) OHANA_FORMAT(scanf, 3, 5);
int     gfits_scan_alt                 PROTO((Header *header, char *field, char *mode, int N,...)); // do not use a FORMAT: non-standard fmt chars
int     gfits_set_unsign_mode          PROTO((int mode));
int     gfits_stripwhite               PROTO((char *string));
int     gfits_vscan                    PROTO((Header *header, char *field, char *mode, int N, va_list argp));
int     gfits_vscan_alt                PROTO((Header *header, char *field, char *mode, int N, va_list argp));
int     gfits_write_header             PROTO((char *filename, Header *header)); 
off_t   gfits_data_size                PROTO((Header *header));
off_t   gfits_data_min_size            PROTO((Header *header));
off_t   gfits_data_pad_size            PROTO((off_t rawsize));
off_t   gfits_heap_start               PROTO((Header *header));
int 	gfits_extended_to_primary      PROTO((Header *header, int simple, char *comment));
int 	gfits_primary_to_extended      PROTO((Header *header, char *exttype, char *comment));
int 	gfits_modify_extended          PROTO((Header *header, char *exttype, char *comment));
char   *gfits_keyword_start 	       PROTO((char *line));
char   *gfits_keyword_end   	       PROTO((char *line));
char   *gfits_hierarch_keyword_start   PROTO((char *line, char *field));
char   *gfits_hierarch_keyword_end     PROTO((char *line, char *field));
void    gfits_pad_ending               PROTO((char *line, char value, int Nbyte));
int     gfits_vscan_hierarch           PROTO((Header *header, char *field, char *mode, int N, va_list argp));
char   *gfits_header_hierarch_field    PROTO((Header *header, char *field, int N));

/******************************* Matrix functions *************/

int     gfits_init_matrix              PROTO((Matrix *matrix));
Matrix *gfits_alloc_matrix             PROTO((void));
void    gfits_free_matrix              PROTO((Matrix *matrix)); 
off_t   gfits_npix_matrix              PROTO((Matrix *matrix));
void   	gfits_add_matrix_value         PROTO((Matrix *matrix, off_t x, off_t y, double value)); 
int    	gfits_convert_format           PROTO((Header *header, Matrix *matrix, int outBitpix, double outScale, double outZero, int inBlank, int outUnsign));
int     gfits_copy_matrix              PROTO((Matrix *in, Matrix *out)); 
int     gfits_create_matrix            PROTO((Header *header, Matrix *matrix)); 
int    	gfits_divide_matrix            PROTO((Matrix *M1, Matrix *M2, Matrix *M3)); 
int    	gfits_fread_matrix             PROTO((FILE *f, Matrix *matrix, Header *header));
int    	gfits_fread_matrix_segment     PROTO((FILE *f, Matrix *matrix, Header *header, char *region));
int     gfits_fwrite_matrix            PROTO((FILE *f, Matrix *matrix));      
double  gfits_get_matrix_value         PROTO((Matrix *matrix, off_t x, off_t y)); 
void   	gfits_insert_matrix            PROTO((Matrix *matrix, Matrix *array, off_t x, off_t y)); 
int    	gfits_load_matrix              PROTO((FILE *f, Matrix *matrix, Header *header));
int    	gfits_multiply_matrix          PROTO((Matrix *M1, Matrix *M2, Matrix *M3)); 
int     gfits_read_matrix              PROTO((char *filename, Matrix *matrix));      
int    	gfits_read_matrix_segment      PROTO((char *filename, Matrix *matrix, char *region));
int     gfits_read_portion             PROTO((char *filename, Matrix *matrix, off_t Nskip, off_t Npix));
void   	gfits_set_matrix_value         PROTO((Matrix *matrix, off_t x, off_t y, double value)); 
int     gfits_write_matrix             PROTO((char *filename, Matrix *matrix)); 

int     gfits_uncompress_image 	       PROTO((Header *header, Matrix *matrix, FTable *ftable));
int     gfits_uncompress_data  	       PROTO((char *zdata, unsigned long int Nzdata, char *cmptype, char **optname, char **optvalue, int Nopt, char *outdata, unsigned long int *Nout, unsigned long int Nout_alloc, int out_pixsize));
int     gfits_distribute_data  	       PROTO((Matrix *matrix, char *data, unsigned long int Ndata, int bitpix, int *otile, int oblank, unsigned long int *ztile, int zblank, float zscale, float zzero));

int     gfits_compress_data  	       PROTO((char *zdata, unsigned long int *Nzdata, char *cmptype, char **optname, char **optvalue, int Nopt, char *rawdata, unsigned long int Nraw, int raw_pixsize, int Nx, int Ny));
int     gfits_compress_image 	       PROTO((Header *header, Matrix *matrix, FTable *ftable, unsigned long int *Ztile, char *zcmptype));
int     gfits_collect_data   	       PROTO((Matrix *matrix, char *raw, unsigned long int Nraw, int raw_pixsize, int *otile, int oblank, unsigned long int *ztile, int zblank, float zscale, float zzero));

int     gfits_copy_keywords_compress   PROTO((Header *srchead, Header *tgthead));
int     gfits_swap_raw   	       PROTO((Matrix *matrix));

off_t   gfits_tile_size                PROTO((Matrix *matrix, int *otile, unsigned long int *ztile));
int 	gfits_imtile_maxsize           PROTO((Matrix *matrix, unsigned long int *ztile));
int 	gfits_imtile_count             PROTO((Matrix *matrix, unsigned long int *ztile, int *ntile));
int 	gfits_imtile_start             PROTO((Matrix *matrix, int *otile));
int 	gfits_imtile_next              PROTO((Matrix *matrix, unsigned long int *ztile, int *ntile, int *otile));

int     gfits_extension_is_compressed_image  PROTO((Header *header));
int     gfits_extension_is_compressed_table  PROTO((Header *header));
int     gfits_byteswap_zdata   	        PROTO((char *zdata, off_t Nzdata, int pixsize));
int     gfits_compressed_data_pixsize   PROTO((char *cmptype, int out_bitpix, char **optname, char **optvalue, int Noptions));
int     gfits_uncompressed_data_pixsize PROTO((char *cmptype, int out_bitpix, char **optname, char **optvalue, int Noptions, int rice_use_bitpix));
int     gfits_uncompressed_data_bitpix  PROTO((char *cmptype, int out_bitpix, char **optname, char **optvalue, int Noptions, int rice_use_bitpix));
int     gfits_vartable_heap_pixsize     PROTO((char format));
int     gfits_cmptype_valid             PROTO((char *cmptype));

int     gfits_varlength_column_add_data PROTO((FTable *table, char *data, off_t Ndata, int row, VarLengthColumn *column));
void   *gfits_varlength_column_pointer  PROTO((FTable *ftable, VarLengthColumn *column, off_t row, off_t *length));
int     gfits_varlength_column_define   PROTO((FTable *ftable, VarLengthColumn *def, int column));
int     gfits_varlength_column_finish   PROTO((FTable *ftable, VarLengthColumn *def));
int     gfits_byteswap_varlength_column PROTO((FTable *ftable, int column));

/******************************* Table functions *************/

int     gfits_init_table  	       PROTO((FTable *table));
int     gfits_init_vtable 	       PROTO((VTable *table));

char   *gfits_table_print              PROTO((FTable *ftable,...));
int     gfits_add_rows                 PROTO((FTable *ftable, char *data, off_t Nrow, off_t Nbytes));
int     gfits_set_table_rows           PROTO((Header *header, FTable *table, off_t Nrows));
int     gfits_bintable_format          PROTO((char *format, char *type, int *Nval, int *Nbytes));
int     gfits_create_table             PROTO((Header *header, FTable *ftable));
int     gfits_create_table_header      PROTO((Header *header, char *type, char *extname));
int     gfits_define_bintable_column   PROTO((Header *header, char *format, char *label, char *comment, char *unit, double bscale, double bzero));
int     gfits_define_table_column      PROTO((Header *header, char *format, char *label, char *comment, char *unit));
int   	gfits_fread_ftable             PROTO((FILE *f, FTable *ftable, char *extname)); 
int   	gfits_fread_ftable_data        PROTO((FILE *f, FTable *ftable, int padIfShort));
int   	gfits_fread_ftable_range       PROTO((FILE *f, int padIfShort, int noSeek, FTable *ftable, off_t start, off_t Nrows));
int   	gfits_fread_vtable             PROTO((FILE *f, VTable *vtable, char *extname, off_t Nrow, off_t *row));
int   	gfits_fread_vtable_range       PROTO((FILE *f, VTable *vtable, off_t start, off_t Nrows));
int     gfits_free_table               PROTO((FTable *ftable));
int     gfits_free_vtable              PROTO((VTable *vtable));
int     gfits_fwrite_table             PROTO((FILE *f, FTable *table));
int     gfits_fwrite_vtable            PROTO((FILE *f, VTable *table));
int     gfits_fwrite_ftable_range      PROTO((FILE *f, FTable *table, off_t start, off_t Nrows, off_t Ndisk, off_t Ntotal));
int     gfits_byteswap_bintable_column PROTO((FTable *ftable, int column));
int     gfits_get_bintable_column      PROTO((Header *header, FTable *table, char *label, void **data));
int     gfits_get_bintable_column_raw  PROTO((Header *header, FTable *table, char *label, void **data, char nativeOrder));
int     gfits_get_bintable_column_type PROTO((Header *header, char *label, char *type, int *Nval));
void   *gfits_get_bintable_column_data PROTO((Header *header, FTable *table, char *label, char *type, off_t *Nrow, int *Ncol));
int     gfits_get_bintable_column_type_by_N  PROTO((Header *header, int N, char *type, int *Nval));
void   *gfits_get_bintable_column_data_raw   PROTO((Header *header, FTable *table, char *label, char *type, off_t *Nrow, int *Ncol, char nativeOrder));
int     gfits_get_table_column         PROTO((Header *header, FTable *table, char *label, void **data));
int     gfits_get_table_column_type    PROTO((Header *header, char *label, char *type, int *Nval));
int     gfits_read_ftable              PROTO((char *filename, FTable *table, char *extname));
int     gfits_read_table               PROTO((char *filename, FTable *ftable)); 
int     gfits_set_bintable_column      PROTO((Header *header, FTable *table, char *label, void *data, off_t Nrow));
int     gfits_set_bintable_column_reformat PROTO((Header *header, FTable *table, char *label, char *intype, void *data, off_t Nrow, int element, char nativeOrder));
int     gfits_set_table_column         PROTO((Header *header, FTable *table, char *label, void *data, off_t Nrow));
int     gfits_table_column             PROTO((FTable *ftable, char *field, char *mode,...)) OHANA_FORMAT(printf, 3, 4);
int     gfits_table_format             PROTO((char *format, char *type, int *Nval, int *Nbytes));
int     gfits_table_scale_data         PROTO((FTable *ftable));
int     gfits_table_scale_storage      PROTO((FTable *ftable));
int     gfits_table_to_vtable          PROTO((FTable *ftable, VTable *vtable, off_t start, off_t Nkeep));
int     gfits_vadd_rows                PROTO((VTable *vtable, char *data, off_t Nrow, off_t Nbytes));
int     gfits_vtable_from_ftable       PROTO((FTable *ftable, VTable *vtable, off_t *row, off_t Nrow));
int     gfits_write_table              PROTO((char *filename, FTable *ftable)); 
int     gfits_copy_ftable              PROTO((FTable *in, FTable *out));
int     gfits_copy_vtable              PROTO((VTable *in, VTable *out));
int     gfits_copy_ftable_ptr          PROTO((FTable *in, FTable *out));

int     gfits_create_Theader           PROTO((Header *header, char *type));
int     gfits_fread_Theader            PROTO((FILE *f, Header *header));
int     gfits_fwrite_Theader           PROTO((FILE *f, Header *header));
int     gfits_load_Theader             PROTO((FILE *f, Header *header));
int     gfits_read_Theader             PROTO((char *filename, Header *header));     
int     gfits_write_Theader            PROTO((char *filename, Header *header));

int     gfits_compress_table           PROTO((FTable *srctable, FTable *tgttable, unsigned long int ztilelen, char *zcmptype));
int     gfits_uncompress_table         PROTO((FTable *srctable, FTable *tgttable));

// XXX EAM for testing
int gfits_dump_raw_table (FTable *table, char *message);
int gfits_dump_cmp_table (FTable *table, char *message);

void myMemset (char *ptr, int value, size_t N);

#endif /* FITSIO */
