# include "data.h"

int book_list (int argc, char **argv);
int book_init (int argc, char **argv);
int book_create (int argc, char **argv);
int book_delete (int argc, char **argv);
int book_getbook (int argc, char **argv);
int book_listbook (int argc, char **argv);
int book_shuffle (int argc, char **argv);
int book_npages (int argc, char **argv);
int book_newpage (int argc, char **argv);
int book_getpage (int argc, char **argv);
int book_delpage (int argc, char **argv);
int book_listpage (int argc, char **argv);
int book_setword (int argc, char **argv);
int book_getword (int argc, char **argv);

static Command book_commands[] = {
  {1, "list",     book_list,     "list books"},
  {1, "init",     book_init,     "initialize a book"},
  {1, "create",   book_create,   "create a book"},
  {1, "delete",   book_delete,   "delete a book"},
  {1, "getbook",  book_getbook,  "get book name by location"},
  {1, "listbook", book_listbook, "list pages in a book"},
  {1, "shuffle",  book_shuffle,  "shuffle pages in a book"},
  {1, "npages",   book_npages,   "return number of pages in a book"},
  {1, "newpage",  book_newpage,  "create a new page in a book"},
  {1, "getpage",  book_getpage,  "get page name by location"},
  {1, "delpage",  book_delpage,  "delete a page in a book"},
  {1, "listpage", book_listpage, "list a page in a book"},
  {1, "setword",  book_setword,  "set the value of a word in a page"},
  {1, "getword",  book_getword,  "set the value of a word from a page"},
};

int book_command (int argc, char **argv) {

  int i, N, status;

  if (argc < 2) {
    gprint (GP_ERR, "USAGE: book (command)\n");
    gprint (GP_ERR, "    book list                                  : list books\n");
    gprint (GP_ERR, "    book init     (book)                       : removes all pages from book\n");
    gprint (GP_ERR, "    book create   (book)                       : create a book\n");
    gprint (GP_ERR, "    book delete   (book)                       : delete a book\n");
    gprint (GP_ERR, "    book getbook  (where) [-var var]           : get book name\n");
    gprint (GP_ERR, "    book listbook (book)                       : list a book\n");
    gprint (GP_ERR, "    book shuffle  (book)                       : randomize pages in a book\n");
    gprint (GP_ERR, "    book npages   (book) [-var var] [-key key] : return number of pages in a book\n");
    gprint (GP_ERR, "    book newpage  (book) (page)                : create a new page in a book\n");
    gprint (GP_ERR, "    book getpage  (book) (where) [-var var] [-key key value] : get page name in a book\n");
    gprint (GP_ERR, "    book delpage  (book) (page) [-key key]     : delete a page in a book\n");
    gprint (GP_ERR, "    book delpage  (book) -key name value       : delete a page in a book\n");
    gprint (GP_ERR, "    book listpage (book) (page)                : list a page in a book\n");
    gprint (GP_ERR, "    book setword  (book) (page) (word) (value) : set the value of a word in a page\n");
    gprint (GP_ERR, "    book getword  (book) (page) (word) [-var var] : set the value of a word from a page\n");
    return (FALSE);
  }

  N = sizeof (book_commands) / sizeof (Command);

  /* find the book sub-command which matches */
  for (i = 0; i < N; i++) {
    if (!strcmp (book_commands[i].name, argv[1])) {
      status = (*book_commands[i].func) (argc - 1, argv + 1);
      return (status);
    }
  }

  gprint (GP_ERR, "unknown book command %s\n", argv[1]);
  return (FALSE);
}

/* book is called with the command "book".  
   the command line word "book" is meant to be followed the one of several 
   possible options:
   
   book create
   book delete
   book list
   book edit
   book read
   book write

*/
