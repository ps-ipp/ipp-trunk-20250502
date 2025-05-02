# include <ohana.h>
# include <gfitsio.h>
# include <sys/types.h>
# include <regex.h>
# include <nebclient.h>

# define ROC_HEADER_SIZE  0x1000
# define ROC_HEADER_BLOCK 0x1000
# define ROC_BLOCKSIZE    0x8000

int roc_create (int argc, char **argv);
int roc_repair (int argc, char **argv);
int roc_insert (int argc, char **argv);
int roc_delete (int argc, char **argv);
int roc_validate (int argc, char **argv);

int usage (void);
int print_block (char *header, int *header_size, int *Noff, char *format,...);

FILE *fopen_harder(char *filename);

int main (int argc, char **argv) {

  if (get_argument (argc, argv, "-h")) usage();
  if (get_argument (argc, argv, "-help")) usage();
  if (get_argument (argc, argv, "--help")) usage();
  if (argc < 3) usage();

  if (!strcasecmp(argv[1], "-create")) roc_create (argc, argv);
  if (!strcasecmp(argv[1], "-repair")) roc_repair (argc, argv);
  if (!strcasecmp(argv[1], "-insert")) roc_insert (argc, argv);
  if (!strcasecmp(argv[1], "-delete")) roc_delete (argc, argv);
  if (!strcasecmp(argv[1], "-validate")) roc_validate (argc, argv);

  usage();
  exit (2);
}

int roc_create (int argc, char **argv) {

  int i, j, n, Ninput, result, Nblocks, header_size, size_off;
  int status, Noff, Nbytes, Nread, Nwrite;
  off_t maxSize, *sizes, *bytes_read;
  char value;
  char *header;
  char **inputName, *base;
  char **inputData;
  char *outputData;
  FILE **input, *target;
  struct stat stats;

  if (argc < 4) usage();

  /* find the md5 sum for each input file (ADD LATER)
   * find the size of the input files
   * define the output file size = MAX(sizes)
   * create the target file header
   * open & read the input files, write the output
   */

  // the output file
  char *targetName = argv[2];
  Ninput = argc - 3;

  // the input files
  ALLOCATE (inputName, char *, Ninput);
  for (i = 0; i < Ninput; i++) {
    inputName[i] = strcreate (argv[i + 3]);
  }

  // check on the input files (validity & sizes)
  maxSize = 0;
  ALLOCATE (sizes, off_t, Ninput);
  ALLOCATE (bytes_read, off_t, Ninput);
  for (i = 0; i < Ninput; i++) {
    result = stat (inputName[i], &stats);
    if (result) {
      perror("problem with input file");
      exit (1);
    }
    if (!S_ISREG(stats.st_mode)) {
      fprintf (stderr, "%s is not a regular file\n", inputName[i]);
      exit (1);
    }
    sizes[i] = stats.st_size;
    maxSize = MAX(maxSize, sizes[i]);
    bytes_read[i] = 0;
  }

  ALLOCATE (input, FILE *, Ninput);
  for (i = 0; i < Ninput; i++) {
    input[i] = fopen(inputName[i], "r");
    myAssert (input[i], "failed to open file");
  }

  Nblocks = maxSize / ROC_BLOCKSIZE;
  if (maxSize % ROC_BLOCKSIZE) {
    Nblocks = 1 + (int) (maxSize / ROC_BLOCKSIZE);
  }

  // create the output header
  header_size = ROC_HEADER_SIZE;
  ALLOCATE (header, char, header_size);

  Noff = 0;
  print_block (header, &header_size, &Noff, "ROC Version %d\n", 0);

  size_off = Noff;
  print_block (header, &header_size, &Noff, "HEADER SIZE 0x%08x\n", header_size); // this is a dummy: re-write below
  print_block (header, &header_size, &Noff, "N_FILES %d\n", Ninput);
  print_block (header, &header_size, &Noff, "N_BLOCKS %d\n", Nblocks);

  // write the header information
  for (i = 0; i < Ninput; i++) {
    print_block (header, &header_size, &Noff, "FILE_%03d : %s\n", i, inputName[i]);
    
    base = filerootname(inputName[i]);
    print_block (header, &header_size, &Noff, "BASE_%03d : %s\n", i, base);
    print_block (header, &header_size, &Noff, "SIZE_%03d : %d\n", i, sizes[i]);
    print_block (header, &header_size, &Noff, "MD5_%03d -- Not Yet Implemented\n", i);
  }
  print_block (header, &header_size, &size_off, "HEADER SIZE 0x%08x", header_size);
  header[size_off] = '\n'; // kind of a hack: print_block writes a NULL char at the end of the string; replace with <RETURN>

  target = fopen(targetName, "w");
  myAssert (target, "failed to open output");

  Nwrite = fwrite (header, 1, header_size, target);
  myAssert (Nwrite == header_size, "failed to write header");

  ALLOCATE (inputData, char *, Ninput);
  ALLOCATE (outputData, char, ROC_BLOCKSIZE);
  for (i = 0; i < Ninput; i++) {
    ALLOCATE (inputData[i], char, ROC_BLOCKSIZE);
  }

  for (n = 0; n < Nblocks; n++) {
    for (i = 0; i < Ninput; i++) {
      Nread = MIN (ROC_BLOCKSIZE, sizes[i] - bytes_read[i]);
      Nbytes = fread (inputData[i], 1, Nread, input[i]);
      myAssert (Nbytes == Nread, "failed to read data");

      if (Nread < ROC_BLOCKSIZE) {
	// if we have reached the end of the file, fill in the rest with NULLs
	memset (&inputData[i][Nread], 0, ROC_BLOCKSIZE - Nread);
      }
      bytes_read[i] += Nread;
    }
    
    for (j = 0; j < ROC_BLOCKSIZE; j++) {
      value = 0;
      for (i = 0; i < Ninput; i++) {
	value = value ^ inputData[i][j];
      }
      outputData[j] = value;
    }

    Nwrite = fwrite (outputData, 1, ROC_BLOCKSIZE, target);
    myAssert (Nwrite == ROC_BLOCKSIZE, "failed to write data block");
  }

  status = fclose (target);
  myAssert (!status, "failed to close rocfile");

  exit (0);
}

int roc_repair (int argc, char **argv) {

  int i, j, n, Ninput, Nblocks, header_size, Nbytes;
  int status, Nread, Nfile, Nwrite;
  off_t *sizes, *bytes_read;
  char value;
  char *header, *line, *ptr;
  char **inputName, *targetName, *outputName;
  char **inputData;
  char *targetData;
  char *outputData;
  FILE **input, *target, *output;

  if (argc < 4) usage();

  /* find the md5 sum for each input file (ADD LATER)
   * find the size of the input files
   * define the output file size = MAX(sizes)
   * create the target file header
   * open & read the input files, write the output
   */

  // the output file
  targetName = argv[2];
  Nfile = atoi(argv[3]);
  outputName = argv[4];

  if (Nfile < 0) {
    fprintf (stderr, "invalid file number %d\n", Nfile);
    exit (3);
  }

  target = fopen(targetName, "r");
  myAssert (target, "failed to open rocfile");
  
  ALLOCATE (line, char, 1024);

  scan_line (target, line);
  myAssert (!strncmp(line, "ROC Version 0", strlen("ROC Verion 0")), "invalid ROC file or format");

  scan_line (target, line);
  sscanf (line, "HEADER SIZE %x", &header_size);

  ALLOCATE (header, char, header_size);

  // read the full header
  fseeko (target, 0, SEEK_SET);
  Nread = fread (header, 1, header_size, target);
  myAssert (Nread == header_size, "failed to read header");

  ptr = header;
  ptr = strchr (ptr, '\n'); ptr ++; // ROC Version
  ptr = strchr (ptr, '\n'); ptr ++; // HEADER SIZE
  
  sscanf (ptr, "N_FILES %d", &Ninput); ptr = strchr (ptr, '\n'); ptr ++; 
  sscanf (ptr, "N_BLOCKS %d", &Nblocks); ptr = strchr (ptr, '\n'); ptr ++; 

  if (Nfile >= Ninput) {
    fprintf (stderr, "file number %d not in ROC\n", Nfile);
    exit (3);
  }

  // the input files
  ALLOCATE (inputName, char *, Ninput);
  ALLOCATE (sizes, off_t, Ninput);
  ALLOCATE (bytes_read, off_t, Ninput);
  for (i = 0; i < Ninput; i++) {
    ALLOCATE (inputName[i], char, 1024);
    sscanf (ptr, "%*s : %s", inputName[i]); ptr = strchr (ptr, '\n'); ptr ++; 
    ptr = strchr (ptr, '\n'); ptr ++; // BASE
    sscanf (ptr, "%*s : "OFF_T_FMT, &sizes[i]); ptr = strchr (ptr, '\n'); ptr ++; 
    ptr = strchr (ptr, '\n'); ptr ++; // MD5
    bytes_read[i] = 0;
  }

  ALLOCATE (input, FILE *, Ninput);
  for (i = 0; i < Ninput; i++) {
    if (i == Nfile) continue;
    input[i] = fopen_harder(inputName[i]);
    myAssert (input[i], "failed to open file");
  }

  output = fopen (outputName, "w");
  myAssert (output, "failed to open output");

  ALLOCATE (inputData, char *, Ninput);
  ALLOCATE (targetData, char, ROC_BLOCKSIZE);
  ALLOCATE (outputData, char, ROC_BLOCKSIZE);
  for (i = 0; i < Ninput; i++) {
    if (i == Nfile) continue;
    ALLOCATE (inputData[i], char, ROC_BLOCKSIZE);
  }

  for (n = 0; n < Nblocks; n++) {
    for (i = 0; i < Ninput; i++) {
      if (i == Nfile) continue;
      Nread = MIN (ROC_BLOCKSIZE, sizes[i] - bytes_read[i]);
      Nbytes = fread (inputData[i], 1, Nread, input[i]);
      myAssert (Nbytes == Nread, "failed to read data");

      if (Nread < ROC_BLOCKSIZE) {
	// if we have reached the end of the file, fill in the rest with NULLs
	memset (&inputData[i][Nread], 0, ROC_BLOCKSIZE - Nread);
      }
      bytes_read[i] += Nread;
    }
    Nread = fread (targetData, 1, ROC_BLOCKSIZE, target);
    myAssert (Nread == ROC_BLOCKSIZE, "failed to read from target");
    
    for (j = 0; j < ROC_BLOCKSIZE; j++) {
      value = targetData[j];
      for (i = 0; i < Ninput; i++) {
	if (i == Nfile) continue;
	value = value ^ inputData[i][j];
      }
      outputData[j] = value;
    }

    Nwrite = MIN (ROC_BLOCKSIZE, sizes[Nfile] - bytes_read[Nfile]);
    if (Nwrite == 0) break;
    Nbytes = fwrite (outputData, 1, Nwrite, output);
    myAssert (Nbytes == Nwrite, "failed to write");
    bytes_read[Nfile] += Nbytes;
  }

  for (i = 0; i < Ninput; i++) {
    if (i == Nfile) continue;
    status = fclose(input[i]);
    myAssert (!status, "failed to close input");
  }
  status = fclose (output);
  myAssert (!status, "failed to close output file");

  status = fclose (target);
  myAssert (!status, "failed to close rocfile");

  exit (0);
}

int roc_validate (int argc, char **argv) {

  int i, j, n, Ninput, Nblocks, header_size, Nbytes, Nread;
  off_t *sizes, *bytes_read;
  char value;
  char *header, *line, *ptr;
  char **inputName, *targetName;
  char **inputData;
  char *targetData;
  FILE **input, *target;

  if (argc != 3) usage();


  /* find the md5 sum for each input file (ADD LATER)
   * find the size of the input files
   * define the output file size = MAX(sizes)
   * create the target file header
   */

  // the output file
  targetName = argv[2];

  target = fopen(targetName, "r");
  myAssert (target, "failed to open roc datafile %s\n",targetName);
  
  ALLOCATE (line, char, 1024);

  scan_line (target, line);
  myAssert (!strncmp(line, "ROC Version 0", strlen("ROC Verion 0")), "invalid ROC file or format in %s",targetName);

  scan_line (target, line);
  sscanf (line, "HEADER SIZE %x", &header_size);

  ALLOCATE (header, char, header_size);

  // read the full header
  fseeko (target, 0, SEEK_SET);
  Nbytes = fread (header, 1, header_size, target);
  myAssert (Nbytes == header_size, "failed to read header data\n");

  ptr = header;
  ptr = strchr (ptr, '\n'); ptr ++; // ROC Version
  ptr = strchr (ptr, '\n'); ptr ++; // HEADER SIZE
  
  sscanf (ptr, "N_FILES %d", &Ninput); ptr = strchr (ptr, '\n'); ptr ++; 
  sscanf (ptr, "N_BLOCKS %d", &Nblocks); ptr = strchr (ptr, '\n'); ptr ++; 

  // the input files
  ALLOCATE (inputName, char *, Ninput);
  ALLOCATE (sizes, off_t, Ninput);
  ALLOCATE (bytes_read, off_t, Ninput);
  for (i = 0; i < Ninput; i++) {
    ALLOCATE (inputName[i], char, 1024);
    sscanf (ptr, "%*s : %s", inputName[i]); ptr = strchr (ptr, '\n'); ptr ++; 
    ptr = strchr (ptr, '\n'); ptr ++; // BASE
    sscanf (ptr, "%*s : "OFF_T_FMT, &sizes[i]); ptr = strchr (ptr, '\n'); ptr ++; 
    ptr = strchr (ptr, '\n'); ptr ++; // MD5
    bytes_read[i] = 0;
  }

  ALLOCATE (input, FILE *, Ninput);
  int error = 0;
  for (i = 0; i < Ninput; i++) {
    //    input[i] = fopen(inputName[i], "r");
    input[i] = fopen_harder(inputName[i]);
    //    myAssert (input[i], "failed to open input: %d data file: %s in %s\n",i,inputName[i],targetName);
    if (!(input[i])) {
      fprintf(stderr,"failed to open input: %d data file: %s in %s\n",i,inputName[i],targetName);
      error += 1;
    }
  }
  myAssert((error == 0), "Assertation that all were fine failed.");

  ALLOCATE (inputData, char *, Ninput);
  ALLOCATE (targetData, char, ROC_BLOCKSIZE);
  for (i = 0; i < Ninput; i++) {
    ALLOCATE (inputData[i], char, ROC_BLOCKSIZE);
  }

  for (n = 0; n < Nblocks; n++) {
    for (i = 0; i < Ninput; i++) {
      Nread = MIN (ROC_BLOCKSIZE, sizes[i] - bytes_read[i]);
      Nbytes = fread (inputData[i], 1, Nread, input[i]);
      myAssert (Nbytes == Nread, "failed to read input: %d data file: %s read: %d expect: %d prev_read: %d block: %d in %s\n",i,inputName[i],Nbytes,Nread,(int) bytes_read[i],n,targetName);
      if (Nread < ROC_BLOCKSIZE) {
	// if we have reached the end of the file, fill in the rest with NULLs
	memset (&inputData[i][Nread], 0, ROC_BLOCKSIZE - Nread);
      }
      bytes_read[i] += Nread;
    }
    Nbytes = fread (targetData, 1, ROC_BLOCKSIZE, target);
    myAssert (Nbytes == ROC_BLOCKSIZE, "failed to read roc file: %s read: %d expect %d\n", targetName, Nbytes, ROC_BLOCKSIZE);
    
    for (j = 0; j < ROC_BLOCKSIZE; j++) {
      value = targetData[j];
      for (i = 0; i < Ninput; i++) {
	value = value ^ inputData[i][j];
      }
      myAssert(value == 0, "validation failed on block %d and byte %d in %s\n",n,j,targetName);
    }

  }

  for (i = 0; i < Ninput; i++) {
    fclose(input[i]);
  }

  fclose (target);

  fprintf (stderr, "validation: %s appears correct.\n",targetName);
  exit (0);
}

FILE *fopen_harder (char *filename) {
  FILE *try;

  fprintf(stderr,"TRYING: %s\n",filename);
  try = fopen(filename,"r");
  if (try) { return(try); }

  char *root = filerootname(filename);
  fprintf(stderr,"TRYING: %s\n",root);
  try = fopen(root,"r");
  if (try) { return(try); }

  
  char *key = malloc(sizeof(char) * (strlen(root) + 6));
  char *token = strtok(root,".");
  sprintf(key,"neb://");
  token = strtok(NULL,":");
  while (token) {
    sprintf(key,"%s/%s",key,token);
    token = strtok(NULL,":");
  }
  sprintf(key,"%s.fits",key);
  fprintf(stderr,"WILL_TRY: %s\n",key);
  
  char *disk_file = NULL;
  nebServer *server = nebServerAlloc("http://ippc08/nebulous");
  disk_file = nebFind(server,key);
  //  nebServerFree(server);
  //  free(key);
  
  if (disk_file) {
    fprintf(stderr,"TRYING: %s\n",disk_file);
    try = fopen(disk_file,"r");
    if (try) { return(try); }
  }

  return(NULL);
}

   
  

int roc_insert (int argc, char **argv) {

  exit (0);
}

int roc_delete (int argc, char **argv) {

  exit (0);
}

/* roc file description:

   header block + data block

   header min length = ROC_HEADER_SIZE (padded with NULLs)

   ROC Version %d
   NFILES %d
   FILE_NNN : full path
   BASE_NNN : basename
   SIZE_NNN : file size
   MD5_NNN  : MD5 checksum of file
   END

   MAX(NNN) = 32?
   32*1024
*/

int usage () {
  fprintf (stderr, "USAGE: roc [mode]\n");
  fprintf (stderr, " -create (target) (input) (input) (input) ... [options]\n");
  fprintf (stderr, " -repair (target) (file #) (output)\n");
  fprintf (stderr, " -validate (target)\n");
  fprintf (stderr, " -insert (target) (input)\n");
  fprintf (stderr, " -delete (target) (file)\n");

  exit (2);
}

int print_block (char *header, int *header_size, int *Noff, char *format,...) {

  char tmp;
  int Nbyte, Nleft;
  va_list argp;  

  va_start (argp, format);
  Nbyte = vsnprintf (&tmp, 0, format, argp);
  va_end (argp);

  Nleft = *header_size - *Noff;

  if (Nbyte >= Nleft) {
    *header_size += ROC_HEADER_BLOCK + Nbyte;
    REALLOCATE (header, char, *header_size);
  }
  
  va_start (argp, format);
  Nbyte = vsnprintf (&header[*Noff], Nleft, format, argp);
  va_end (argp);

  if (Nbyte >= Nleft) {
    return (FALSE);
  }

  *Noff += Nbyte;
  return (Nbyte);
}

