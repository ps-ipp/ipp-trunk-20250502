# include "pantasks.h"

# define FREEKEYS							\
  if (keys) { for (int ix = 0; ix < Nkeys; ix++) { free (keys[ix]); } free (keys); } \
  for (int ix = 0; ix < setWordN; ix++) { free (setWordList[ix]); free (setWordValue[ix]); } \
  free (setWordValue); free (setWordList);

// USAGE: queue2book (queue) (book)
// convert the named queue into book format
// the output book will use the name supplied as the second argument
// the input queue should either use the ippTools metadata format OR
// the input queue should consist of a set of lines with white-space 
// separated columns of data.  

// **** MD format ****
// In MD format, the queue starts with a line of the form "BOOKNAME MULTI"
// following this, there will be a series of METADATA blocks.  the
// value BOOKNAME in the first line much match the names of the METADATA blocks
// note that the BOOKNAME is not actually used to set the output book name, 
// and is not required to have any specific value other than matching the 
// following METADATA blocks.

// the METADATA blocks are used to create pages within the book (one page per block)
// the key / value pairs within the METADATA blocks are used to set the values 
// words within each page. 

// option: -key (word[:word[:word..]])
// pages are given names of the form 'page.NNN' unless -key options are supplied.
// if one or more -key options are supplied, the value of the associated word is used
// to set the pagename.  If multiple keys are supplied, the words are joined with colons.

// option: -uniq
// if -uniq is supplied, any new pages which match existing pages will be dropped / ignored

// option: -setword
// additional words may be added to the pages of the book when they are created.

// **** Raw Column format ****

// In Raw Column format, each line in the queue is converted to a new page. The white-space separated
// words in each line are interpretted as the values of words on this page.  The names of these
// columns (words) are specified on the first line of the queue.  Thus a valid input must consist of 
// at least two lines.

// we need these in both parsing blocks
int Unique;
int RawColumns;

char **setWordList;
char **setWordValue;
int    setWordN;

int   Nkeys;
char **keys;

int queue2book_rawcolumns (Queue *queue, Book *book);
int queue2book_setpages (Book *book, Page *page);
int queue2book_addwords (Book *book, Page *page);
int queue2book_uniquify (Book *book, Page *page);

int queue2book (int argc, char **argv) {

  int N, onPage;
  char *line, *tmpword, *tmpvalue;
  char pagename[512]; // XXX this should be made dynamic, though it is an unlikey problem

  /* supply additional constant words */
  setWordN = 0;
  int setWordNalloc = 10;
  ALLOCATE (setWordList, char *, setWordNalloc);
  ALLOCATE (setWordValue, char *, setWordNalloc);
  while ((N = get_argument (argc, argv, "-setword"))) {
    remove_argument (N, &argc, argv);
    setWordList[setWordN] = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
    setWordValue[setWordN] = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
    setWordN ++;
    if (setWordN >= setWordNalloc) {
      setWordNalloc += 10;
      REALLOCATE (setWordList, char *, setWordNalloc);
      REALLOCATE (setWordValue, char *, setWordNalloc);
    }      
  }

  Unique = FALSE;
  if ((N = get_argument (argc, argv, "-uniq"))) {
    remove_argument (N, &argc, argv);
    Unique = TRUE;
  }

  RawColumns = FALSE;
  if ((N = get_argument (argc, argv, "-raw-columns"))) {
    remove_argument (N, &argc, argv);
    RawColumns = TRUE;
  }

  // keys are used to define the page names (otherwise page.NNN is used)
  Nkeys = 0;
  keys  = NULL;
  if ((N = get_argument (argc, argv, "-key"))) {
    remove_argument (N, &argc, argv);

    /* parse the key (word:word:word) into constituents */
    int NKEYS = 10;
    ALLOCATE (keys, char *, NKEYS);
    
    char *p = argv[N];
    char *q = p;
    while (q != NULL) {
      q = strchr (p, ':');
      keys[Nkeys] = (q == NULL) ? strcreate (p) : strncreate (p, q - p);
      p = q + 1;
      Nkeys ++;
      CHECK_REALLOCATE (keys, char *, NKEYS, Nkeys, 10);
    }
    remove_argument (N, &argc, argv);
  }

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: queue2book (queue) (book) [-uniq] [-key key[:key..]] [-setword (word) (value)] [-raw-columns] \n");
    gprint (GP_ERR, "       converts the named queue  with ipptool metadata output into book format\n");
    FREEKEYS;
    return (TRUE);
  }

  Queue *queue = FindQueue (argv[1]);
  if (queue == NULL) {
    gprint (GP_ERR, "queue %s not found\n", argv[1]);
    FREEKEYS;
    return (TRUE);
  }
    
  Book *book = CreateBook (argv[2]);

  if (RawColumns) {
    int status = queue2book_rawcolumns (queue, book);
    return status;
  }

  /* the first non-whitespace line of the queue should contain:
     BookName MULTI
  */

  /* scan for first non-emtpy line */
  while (TRUE) {
    line = PopQueue (queue);
    if (!line) {
      FREEKEYS;
      return (TRUE); // no output in queue - not an error?
    }
    stripwhite (line);
    if (line[0] == '#') {
      free (line);
      continue;
    }
    if (*line) break;
    free (line);
  }
  char *bookName = thisword (line);
  tmpword = nextword (line);

  // check for BookName MULTI
  if (!tmpword || strcmp(tmpword, "MULTI")) {
    gprint (GP_ERR, "ERROR: missing metadata output name on first line\n");
    goto escape;
  }
  free (line);
  
  // loop over lines, checking for new pages and words in the page
  onPage = FALSE;
  Page *page = NULL;
  while (TRUE) {
    line = PopQueue (queue);
    if (!line) { // we have reached the end of the queue
      if (onPage) { gprint (GP_ERR, "ERROR: unterminated metadata\n"); }
      free (bookName);
      FREEKEYS;
      return (TRUE);
    }
    stripwhite (line);

    // skip empty lines and commented-out lines
    if (!*line) { free (line); continue; }
    if (line[0] == '#') { free (line); continue; }
    
    // parse the words on this line 
    tmpword = thisword (line);
    if (tmpword == NULL) abort(); // should not happen if line exists

    if (!onPage) {
      if (strcmp(tmpword, bookName)) {
	gprint (GP_ERR, "ERROR: unexpected metadata name %s\n", tmpword);
	free (tmpword);
	goto escape;
      }
      free (tmpword);
      tmpword = thisword (nextword (line));
      if (strcmp(tmpword, "METADATA")) {
	gprint (GP_ERR, "ERROR: missing METADATA tag\n");
	free (tmpword);
	goto escape;
      }
      onPage = TRUE;
      // we don't know the page name until we load its data, so set temporary name.
      // this name will be used if no keys are supplied
      sprintf (pagename, "page.%03d", book[0].Npages);
      page = CreatePage (book, pagename);
      free (line);
      free (tmpword);
      continue;
    } 

    // if we are here, we are within the page

    // if we have hit the end of the page, we have some special work to do
    if (!strcmp(tmpword, "END")) {
      // XXX set the page name based on the contents
      free (tmpword);
      free (line);

      if (!queue2book_setpages (book, page)) return (FALSE);
      queue2book_addwords (book, page);
      queue2book_uniquify (book, page);

      onPage = FALSE;
      continue;
    }

    tmpvalue = thisword (nextword(nextword(line)));
    if (tmpvalue == NULL) {
      gprint (GP_ERR, "ERROR: missing value for %s\n", tmpword);
      free (tmpword);
      goto escape;
    }
    BookSetWord (page, tmpword, tmpvalue);
    free (tmpword);
    free (tmpvalue);
    free (line);
  }    
  FREEKEYS;
  return (TRUE);

escape:
  free (line);
  free (bookName);
  FREEKEYS;
  return (TRUE);
}

int queue2book_rawcolumns (Queue *queue, Book *book) {

  Page *idxPage = NULL;
  char pagename[512];

  while (TRUE) {
    char *line = PopQueue (queue);
    if (!line) { // we have reached the end of the queue
      FREEKEYS;
      if (idxPage) { DeletePage (book, idxPage); }
      return (TRUE);
    }
    stripwhite (line);

    // skip empty lines and commented-out lines
    if (!*line) { free (line); continue; }
    if (line[0] == '#') { free (line); continue; }
    
    Page *page = NULL;

    // the first non-whitespace line of the queue should contain the column names
    // save those words as the words on the index page

    if (idxPage) {
      sprintf (pagename, "page.%03d", book->Npages);
    } else {
      sprintf (pagename, "page.index");
    }
    page = CreatePage (book, pagename);

    int Nw = 0; // index into the number of the word on the line
    char *ptr = line;
    while (ptr) {
      if (idxPage && (Nw >= idxPage->Nwords)) {
	gprint (GP_ERR, "ERROR: line has too many items: %s\n", line);
	return (FALSE);
      }
	
      char *tmpword = thisword (ptr);
      if (tmpword == NULL) abort(); // should not happen if ptr is not NULL

      // need to use the index page to get the words
      if (idxPage) {
	BookSetWord (page, idxPage->words[Nw], tmpword);
      } else {
	BookSetWord (page, tmpword, "index");
      }

      free (tmpword);

      ptr = nextword (ptr);
      Nw ++;
    }
    if (!queue2book_setpages (book, page)) return (FALSE);
    queue2book_addwords (book, page);
    queue2book_uniquify (book, page);

    if (!idxPage) { idxPage = page; }
  }
  return FALSE; // it should not be possible to reach here
}

// keys, Nkeys are file-globals
int queue2book_setpages (Book *book, Page *page) {

  char pagename[512];

  // set the page name based on the key values
  if (!keys) return TRUE;

  memset (pagename, 0, 512);
  for (int i = 0; i < Nkeys; i++) {
    char *tmpword = BookGetWord (page, keys[i]);
    if (tmpword == NULL) {
      gprint (GP_ERR, "ERROR: missing key %s for ID\n", keys[i]);
      FREEKEYS;
      return FALSE;
    }
    if (i > 0) strcat (pagename, ":");
    strcat (pagename, tmpword);
  }
  free (page[0].name);
  page[0].name = strcreate (pagename);
  return TRUE;
}

// setWordList, setWordValue, setWordN are file-globals
int queue2book_addwords (Book *book, Page *page) {
  // add the fixed words to the page
  for (int i = 0; i < setWordN; i++) {
    BookSetWord (page, setWordList[i], setWordValue[i]);
  }
  return TRUE;
}

// setWordList, setWordValue, setWordN are file-globals
// it would be cleaner to not attach this page to the book until this step is passed
int queue2book_uniquify (Book *book, Page *page) {

  if (!Unique) return TRUE;

  // search for an existing page with this name
  // delete this one if one is found 
  
  int found = FALSE;
  for (int i = 0; !found && (i < book->Npages); i++) {
    if (book->pages[i] == page) continue;
    if (!strcmp (book->pages[i]->name, page->name)) {
      found = TRUE;
    }
  }
  if (found) {
    DeletePage (book, page);
  }
  return TRUE;
}



/* scan through the queue lines.  these should look like:  

   bookName METADATA
   key type value
   key type value
   ...
   END

   bookName METADATA
   key type value
   key type value
   ...
   END
*/
