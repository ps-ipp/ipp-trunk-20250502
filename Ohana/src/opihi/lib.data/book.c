# include "data.h"

Book **books;   /* book to store the list of all books */
int    Nbooks;   /* number of currently defined books */
int    NBOOKS;   /* number of currently allocated books */

void InitBooks () {
  Nbooks = 0;
  NBOOKS = 16;
  ALLOCATE (books, Book *, NBOOKS); 
}

void FreeBooks () {

  int i;

  for (i = 0; i < Nbooks; i++) {
    FreeBook (books[i]);
  }
  free (books);
}

void InitBook (Book *book, char *name) {

    book[0].name = strcreate (name);

    book[0].Npages = 0;
    book[0].NPAGES = 16;
    ALLOCATE (book[0].pages, Page *, book[0].NPAGES);
    ALLOCATE (book[0].pageIDs, char *, book[0].NPAGES);
    // ALLOCATE (book[0].index, int, book[0].NPAGES);
}

void FreeBook (Book *book) {

    int i;

    free (book[0].name);
    for (i = 0; i < book[0].Npages; i++) {
	FreePage (book[0].pages[i]);
	free (book[0].pageIDs[i]);
    }
    free (book[0].pages);
    free (book[0].pageIDs);
    // free (book[0].index);
    free (book);
}

/* return the given book */
Book *GetBook (int where) {

  if (where < 0) where += Nbooks;
  if (where < 0) return NULL;
  if (where >= Nbooks) return NULL;
  return (books[where]);
}

/* return the given book */
Book *FindBook (char *name) {

  int i;

  for (i = 0; i < Nbooks; i++) {
    if (!strcmp (books[i][0].name, name)) {
      return (books[i]);
    }
  }
  return (NULL);
}

/* make a new named book */
Book *CreateBook (char *name) {

  int N;
  Book *book;

  book = FindBook (name);
  if (book != NULL) return (book);

  N = Nbooks;
  Nbooks ++;
  CHECK_REALLOCATE (books, Book *, NBOOKS, Nbooks, 16);
  ALLOCATE (book, Book, 1);
  InitBook (book, name);
  books[N] = book;
  return (book);
}

/* delete a book */
int DeleteBook (Book *book) {

  int i, N, NBOOKS_2;

  /* find book in book list */
  N = -1;
  for (i = 0; i < Nbooks; i++) {
    if (books[i] == book) {
      N = i;
      break;
    }
  }
  if (N == -1) return (FALSE);

  for (i = N; i < Nbooks - 1; i++) {
    books[i] = books[i + 1];
  }
  Nbooks --;
  NBOOKS_2 = MAX (16, NBOOKS / 2);
  if (Nbooks < NBOOKS_2) {
    NBOOKS = NBOOKS_2;
    REALLOCATE (books, Book *, NBOOKS);
  }

  FreeBook (book);
  return (TRUE);
}

/* list known books */
void ListBooks () {

  int i;

  for (i = 0; i < Nbooks; i++) {
    gprint (GP_ERR, "%-15s %3d\n", books[i][0].name, books[i][0].Npages);
  }
  return;
}
