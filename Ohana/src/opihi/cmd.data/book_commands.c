# include "data.h"

int book_list (int argc, char **argv) {
  OHANA_UNUSED_PARAM(argv);
  if (argc != 1) {
    gprint (GP_ERR, "USAGE: book list\n");
    return FALSE;
  }

  ListBooks();
  return TRUE;
}

int book_create (int argc, char **argv) {
  if (argc != 2) {
    gprint (GP_ERR, "USAGE: book create (book)\n");
    return FALSE;
  }

  CreateBook (argv[1]);
  return TRUE;
}

int book_delete (int argc, char **argv) {

  int status;
  Book *book;

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: book delete (book)\n");
    return FALSE;
  }

  book = FindBook (argv[1]);
  if (book == NULL) {
    gprint (GP_ERR, "book %s not found\n", argv[1]);
    return FALSE;
  }

  status = DeleteBook (book);
  if (!status) abort ();
  return TRUE;
}

int book_init (int argc, char **argv) {

  int status;
  Book *book;

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: book init (book)\n");
    return FALSE;
  }

  book = FindBook (argv[1]);
  if (book != NULL) {
      status = DeleteBook (book);
      if (!status) abort ();
  }

  CreateBook (argv[1]);
  return TRUE;
}

int book_listbook (int argc, char **argv) {

  Book *book;

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: book listbook (book)\n");
    return FALSE;
  }

  book = FindBook (argv[1]);
  if (book == NULL) {
    gprint (GP_ERR, "book %s not found\n", argv[1]);
    return FALSE;
  }

  ListPages (book);
  return TRUE;
}

int book_npages (int argc, char **argv) {

  int i, N, Npages;
  Book *book;
  char *varName;
  char *Key, *Value, *value;

  varName = NULL;
  if ((N = get_argument (argc, argv, "-var"))) {
    remove_argument (N, &argc, argv);
    varName = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  Key = NULL;
  Value = NULL;
  if ((N = get_argument (argc, argv, "-key"))) {
    remove_argument (N, &argc, argv);
    Key = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
    Value = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: book npages (book) [-var result] [-key name value]\n");
    gprint (GP_ERR, "  reports the number of pages (optionally limited to those matching the key)\n");
    FREE (varName);
    return FALSE;
  }

  book = FindBook (argv[1]);
  if (book == NULL) {
    gprint (GP_ERR, "book %s not found\n", argv[1]);
    FREE (varName);
    return FALSE;
  }

  Npages = book[0].Npages;
  if (Key) {
    /* count only matching key */
    Npages = 0;
    for (i = 0; i < book[0].Npages; i++) {
      value = BookGetWord (book[0].pages[i], Key);
      if (value == NULL) {
	if (!strcmp(Value, "NULL")) { 
	  Npages ++;
	} else {
	  continue;
	}
      } else {
	if (!strcmp(value, Value)) Npages ++;
      }
    }
  }

  if (varName) {
    set_int_variable (varName, Npages);
  } else {
    gprint (GP_ERR, "%d pages\n", Npages);
  }
  FREE (Key);
  FREE (Value);
  FREE (varName);
  return TRUE;
}

int book_getbook (int argc, char **argv) {

  int where, N;
  char *varName;
  Book *book;

  varName = NULL;
  if ((N = get_argument (argc, argv, "-var"))) {
    remove_argument (N, &argc, argv);
    varName = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: book getbook (where) [-var var]\n");
    FREE (varName);
    return FALSE;
  }

  where = atoi (argv[1]);
  book = GetBook (where);
  if (book == NULL) {
    if (varName) {
      set_str_variable (varName, "NULL");
      FREE (varName);
      return TRUE;
    } 

    gprint (GP_ERR, "book %s not found\n", argv[1]);
    FREE (varName);
    return FALSE;
  }

  if (varName) {
    set_str_variable (varName, book[0].name);
  } else {
    gprint (GP_LOG, "%s\n", book[0].name);
  }
  FREE (varName);
  return TRUE;
}

int book_newpage (int argc, char **argv) {

  Book *book;

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: book newpage (book) (page)\n");
    return FALSE;
  }

  book = FindBook (argv[1]);
  if (book == NULL) {
    gprint (GP_ERR, "book %s not found\n", argv[1]);
    return FALSE;
  }
  
  CreatePage (book, argv[2]);
  return TRUE;
}

int book_delpage (int argc, char **argv) {

  int i, N;
  Page *page;
  Book *book;
  char *Key, *Value, *value;

  Key = NULL;
  Value = NULL;
  if ((N = get_argument (argc, argv, "-key"))) {
    remove_argument (N, &argc, argv);
    Key = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
    Value = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if ((argc != 3) && (argc != 2)) {
    gprint (GP_ERR, "USAGE: book delpage (book) (page)\n");
    gprint (GP_ERR, "USAGE: book delpage (book) -key name value\n");
    FREE (Key);
    FREE (Value);
    return FALSE;
  }

  book = FindBook (argv[1]);
  if (book == NULL) {
    gprint (GP_ERR, "book %s not found\n", argv[1]);
    FREE (Key);
    FREE (Value);
    return FALSE;
  }
  
  if (Key) {
    /* delete by matching key */
    for (i = 0; i < book[0].Npages; i++) {
      value = BookGetWord (book[0].pages[i], Key);
      if (value == NULL) continue;
      if (!strcmp(value, Value)) {
	DeletePage (book, book[0].pages[i]);
	i--; /* if we delete this page, don't advance the counter */
      }
    }
    FREE (Key);
    FREE (Value);
    return TRUE;
  }

  page = FindPage (book, argv[2]);
  if (page == NULL) {
    gprint (GP_ERR, "page %s in book %s not found\n", argv[2], argv[1]);
    FREE (Key);
    FREE (Value);
    return FALSE;
  }

  DeletePage (book, page);
  FREE (Key);
  FREE (Value);
  return TRUE;
}

int book_listpage (int argc, char **argv) {

  Page *page;
  Book *book;

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: book listpage (book) (page)\n");
    return FALSE;
  }

  book = FindBook (argv[1]);
  if (book == NULL) {
    gprint (GP_ERR, "book %s not found\n", argv[1]);
    return FALSE;
  }
  
  page = FindPage (book, argv[2]);
  if (page == NULL) {
    gprint (GP_ERR, "page %s in book %s not found\n", argv[2], argv[1]);
    return FALSE;
  }

  ListWords (page);
  return TRUE;
}

int book_getpage (int argc, char **argv) {

  int where, N;
  char *varName, *keyName, *keyValue;
  Book *book;
  Page *page;

  varName = NULL;
  if ((N = get_argument (argc, argv, "-var"))) {
    remove_argument (N, &argc, argv);
    varName = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  keyValue = keyName = NULL;
  if ((N = get_argument (argc, argv, "-key"))) {
    remove_argument (N, &argc, argv);
    keyName = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
    keyValue = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 3) {
    gprint (GP_ERR, "USAGE: book getpage (book) (where) [-var var] [-key key value]\n");
    FREE (varName);
    FREE (keyName);
    FREE (keyValue);
    return FALSE;
  }

  where = atoi (argv[2]);

  book = FindBook (argv[1]);
  if (book == NULL) {
    if (varName) {
      set_str_variable (varName, "NULL");
      FREE (varName);
      FREE (keyName);
      FREE (keyValue);
      return TRUE;
    } 
    gprint (GP_ERR, "book %s not found\n", argv[1]);
    FREE (varName);
    FREE (keyName);
    FREE (keyValue);
    return FALSE;
  }

  if (keyName == NULL) {
    page = GetPage (book, where);
  } else {
    page = GetPageRestricted (book, where, keyName, keyValue);
  }

  if (page == NULL) {
    if (varName) {
      set_str_variable (varName, "NULL");
      FREE (varName);
      FREE (keyName);
      FREE (keyValue);
      return TRUE;
    } 
    gprint (GP_ERR, "page %d not found in %s\n", where, argv[1]);
    FREE (varName);
    FREE (keyName);
    FREE (keyValue);
    return FALSE;
  }

  if (varName) {
    set_str_variable (varName, page[0].name);
  } else {
    gprint (GP_LOG, "%s\n", page[0].name);
  }
  FREE (varName);
  FREE (keyName);
  FREE (keyValue);
  return TRUE;
}

int book_shuffle (int argc, char **argv) {

  Book *book;

  if (argc != 2) {
    gprint (GP_ERR, "USAGE: book shuffle (book)\n");
    return FALSE;
  }

  book = FindBook (argv[1]);
  if (book == NULL) {
    gprint (GP_ERR, "book %s not found\n", argv[1]);
    return FALSE;
  }

  ShufflePages (book);
  return TRUE;
}

int book_setword (int argc, char **argv) {

  Page *page;
  Book *book;

  if (argc != 5) {
    gprint (GP_ERR, "USAGE: book setword (book) (page) (word) (value)\n");
    return FALSE;
  }

  book = FindBook (argv[1]);
  if (book == NULL) {
    gprint (GP_ERR, "book %s not found\n", argv[1]);
    return FALSE;
  }
  
  page = FindPage (book, argv[2]);
  if (page == NULL) {
    gprint (GP_ERR, "page %s in book %s not found\n", argv[2], argv[1]);
    return FALSE;
  }

  BookSetWord (page, argv[3], argv[4]);
  return TRUE;
}

int book_getword (int argc, char **argv) {

  int N;
  Page *page;
  Book *book;
  char *value, *varName;

  varName = NULL;
  if ((N = get_argument (argc, argv, "-var"))) {
    remove_argument (N, &argc, argv);
    varName = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 4) {
    gprint (GP_ERR, "USAGE: book getword (book) (page) (word)\n");
    FREE (varName);
    return FALSE;
  }

  book = FindBook (argv[1]);
  if (book == NULL) {
    if (varName) {
      set_str_variable (varName, "NULL");
      FREE (varName);
      return TRUE;
    } 
    gprint (GP_ERR, "book %s not found\n", argv[1]);
    FREE (varName);
    return FALSE;
  }
  
  page = FindPage (book, argv[2]);
  if (page == NULL) {
    if (varName) {
      set_str_variable (varName, "NULL");
      FREE (varName);
      return TRUE;
    } 
    gprint (GP_ERR, "page %s in book %s not found\n", argv[2], argv[1]);
    FREE (varName);
    return FALSE;
  }

  value = BookGetWord (page, argv[3]);
  if (value == NULL) {
    if (varName) {
      set_str_variable (varName, "NULL");
      FREE (varName);
      return TRUE;
    }
    gprint (GP_ERR, "value %s on page %s in book %s not found\n", argv[3], argv[2], argv[1]);
    FREE (varName);
    return FALSE;
  }

  if (varName) {
    set_str_variable (varName, value);
  } else {
    gprint (GP_LOG, "%s\n", value);
  }
    
  FREE (varName);
  return TRUE;
}
