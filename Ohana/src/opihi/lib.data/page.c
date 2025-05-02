# include "data.h"

void InitPage (Page *page, char *name) {

    page[0].name = strcreate (name);

    page[0].Nwords = 0;
    page[0].NWORDS = 16;
    ALLOCATE (page[0].words, char *, page[0].NWORDS);
    ALLOCATE (page[0].value, char *, page[0].NWORDS);
}

void FreePage (Page *page) {

    int i;

    free (page[0].name);
    for (i = 0; i < page[0].Nwords; i++) {
      free (page[0].words[i]);
      free (page[0].value[i]);
    }
    free (page[0].words);
    free (page[0].value);
    free (page);
}

/* return the given page */
Page *GetPage (Book *book, int where) {

  if (where < 0) where += book[0].Npages;
  if (where < 0) return NULL;
  if (where >= book[0].Npages) return NULL;
  return (book[0].pages[where]);
}

/* return the given page with key restrictions */
Page *GetPageRestricted (Book *book, int where, char *keyName, char *keyValue) {

  int i;
  int N, Nout;
  char *value;

  if (where < 0) where += book[0].Npages;
  if (where < 0) return NULL;
  if (where >= book[0].Npages) return NULL;

  Nout = -1;
  if (where >= 0) {
    N = -1;
    for (i = 0; (i < book[0].Npages) && (N < where); i++) {
      value = BookGetWord (book[0].pages[i], keyName);
      if ((value == NULL) && !strcmp (keyValue, "NULL")) {
	N++;
	Nout = i;
      } 
      if ((value != NULL) && !strcmp (keyValue, value)) {
	N++;
	Nout = i;
      }
    }
  } else {
    N = 0;
    for (i = book[0].Npages - 1; (i >= 0) && (N > where); i--) {
      value = BookGetWord (book[0].pages[i], keyName);
      if ((value == NULL) && !strcmp (keyValue, "NULL")) {
	N--;
      } 
      if ((value != NULL) && !strcmp (keyValue, value)) {
	N--;
      }
    }
  }

  if (N != where) return NULL;

  return (book[0].pages[Nout]);
}

/* return the given page */
/* XXX use index to find more quickly */
Page *FindPage (Book *book, char *name) {

  int i;

  for (i = 0; i < book[0].Npages; i++) {
    if (!strcmp (book[0].pages[i][0].name, name)) {
      return (book[0].pages[i]);
    }
  }
  return (NULL);
}

/* make a new named page */
Page *CreatePage (Book *book, char *name) {

  int N;
  Page *page;

  page = FindPage (book, name);
  if (page != NULL) return (page);

  N = book[0].Npages;
  book[0].Npages ++;
  if (book[0].Npages >= book[0].NPAGES) {
    book[0].NPAGES += 16;
    REALLOCATE (book[0].pages, Page *, book[0].NPAGES);
    REALLOCATE (book[0].pageIDs, char *, book[0].NPAGES);
    // REALLOCATE (book[0].index, int, book[0].NPAGES);
  }
  ALLOCATE (page, Page, 1);
  InitPage (page, name);
  book[0].pages[N] = page;
  book[0].pageIDs[N] = strcreate(name);
  // book[0].index[N] = N;
  
  /* at this point, I should sort the index */

  return (page);
}

void sortpages (int *seq, Page **pages, char **pageIDs, int N) {

# define SWAPFUNC(A,B){ Page *tmpPage; char *tmpID; \
  tmpPage = pages[A];   pages[A]   = pages[B];   pages[B] = tmpPage; \
  tmpID   = pageIDs[A]; pageIDs[A] = pageIDs[B]; pageIDs[B] = tmpID; \
}
# define COMPARE(A,B)(seq[A] < seq[B])

  OHANA_SORT (N, COMPARE, SWAPFUNC);

# undef SWAPFUNC
# undef COMPARE
}

/* make a new named page */
int ShufflePages (Book *book) {

  int i, N, *seq;

  N = book[0].Npages;

  // generate a random index
  ALLOCATE (seq, int, N);
  for (i = 0; i < N; i++) {
    seq[i] = N * drand48();
  }

  // sort the pages by the random index
  sortpages (seq, book[0].pages, book[0].pageIDs, N);
  free (seq);

  return (TRUE);
}

/* delete a page in a book */
int DeletePage (Book *book, Page *page) {

  char *pageID;
  int i, N, NPAGES_2;

  /* find page in page list */
  N = -1;
  for (i = 0; i < book[0].Npages; i++) {
    if (book[0].pages[i] == page) {
      N = i;
      break;
    }
  }
  if (N == -1) return (FALSE);

  pageID = book[0].pageIDs[i];

  for (i = N; i < book[0].Npages - 1; i++) {
    book[0].pages[i] = book[0].pages[i + 1];
    book[0].pageIDs[i] = book[0].pageIDs[i + 1];
    // book[0].index[i] = book[0].index[i + 1];
  }
  book[0].Npages --;
  NPAGES_2 = MAX (16, book[0].NPAGES / 2);
  if (book[0].Npages < NPAGES_2) {
    book[0].NPAGES = NPAGES_2;
    REALLOCATE (book[0].pages, Page *, book[0].NPAGES);
    REALLOCATE (book[0].pageIDs, char *, book[0].NPAGES);
    // REALLOCATE (book[0].index, int, book[0].NPAGES);
  }

  FreePage (page);
  free (pageID);
  return (TRUE);
}

void ListPages (Book *book) {

  int i;

  for (i = 0; i < book[0].Npages; i++) {
    gprint (GP_ERR, "%-15s %3d\n", book[0].pages[i][0].name, book[0].pages[i][0].Nwords);
  }
  return;
}

void ListWords (Page *page) {

  int i;

  for (i = 0; i < page[0].Nwords; i++) {
    gprint (GP_ERR, "%-15s %15s\n", page[0].words[i], page[0].value[i]);
  }
  return;
}

int BookSetWord (Page *page, char *word, char *value) {

  int i;

  for (i = 0; i < page[0].Nwords; i++) {
    if (!strcmp (page[0].words[i], word)) {
      free (page[0].value[i]);
      page[0].value[i] = strcreate (value);
      return TRUE;
    }
  }

  page[0].Nwords ++;
  if (page[0].Nwords >= page[0].NWORDS) {
    page[0].NWORDS += 16;
    REALLOCATE (page[0].words, char *, page[0].NWORDS);
    REALLOCATE (page[0].value, char *, page[0].NWORDS);
  }      
  page[0].words[i] = strcreate (word);
  page[0].value[i] = strcreate (value);
  return (TRUE);
}

char *BookGetWord (Page *page, char *word) {
  int i;

  for (i = 0; i < page[0].Nwords; i++) {
    if (!strcmp (page[0].words[i], word)) {
      return (page[0].value[i]);
    }
  }
  return NULL;
}
