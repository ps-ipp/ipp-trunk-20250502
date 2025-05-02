# include <stdio.h>

main (argc, argv) 
int argc;
char **argv;
{

  int i;
  FILE *f;

  if (argc != 2) {
    fprintf (stderr, "USAGE: %s testfile \n", argv[0]);
    exit (0);
  }

  fprintf (stderr, "char:   %d bytes\n", sizeof (char));
  fprintf (stderr, "short:  %d bytes\n", sizeof (short));
  fprintf (stderr, "int:    %d bytes\n", sizeof (int));
  fprintf (stderr, "long:   %d bytes\n", sizeof (long)); 
  fprintf (stderr, "float:  %d bytes\n", sizeof (float));
  fprintf (stderr, "double: %d bytes\n", sizeof (double));

  f = fopen (argv[1], "w");
  
# define TYPE char
  { 
    TYPE I[20];
    
    I[0] = 1;
    for (i = 1; i < 20; i++) {
      I[i] = I[i-1] * 2;
    }
    fprintf (f, "20 of type TYPE, %d bytes each\n", sizeof(TYPE));
    fwrite (I, sizeof(TYPE), 20, f);
  }

# define TYPE short
  { 
    TYPE I[20];
    
    I[0] = 1;
    for (i = 1; i < 20; i++) {
      I[i] = I[i-1] * 2;
    }
    fprintf (f, "20 of type TYPE, %d bytes each\n", sizeof(TYPE));
    fwrite (I, sizeof(TYPE), 20, f);
  }

# define TYPE int
  { 
    TYPE I[20];
    
    I[0] = 1;
    for (i = 1; i < 20; i++) {
      I[i] = I[i-1] * 2;
    }
    fprintf (f, "20 of type TYPE, %d bytes each\n", sizeof(TYPE));
    fwrite (I, sizeof(TYPE), 20, f);
  }

# define TYPE long
  { 
    TYPE I[20];
    
    I[0] = 1;
    for (i = 1; i < 20; i++) {
      I[i] = I[i-1] * 2;
    }
    fprintf (f, "20 of type TYPE, %d bytes each\n", sizeof(TYPE));
    fwrite (I, sizeof(TYPE), 20, f);
  }

# define TYPE float
  { 
    TYPE I[20];
    
    I[0] = 1;
    for (i = 1; i < 20; i++) {
      I[i] = I[i-1] * 2;
    }
    fprintf (f, "20 of type TYPE, %d bytes each\n", sizeof(TYPE));
    fwrite (I, sizeof(TYPE), 20, f);
  }

# define TYPE double
  { 
    TYPE I[20];
    
    I[0] = 1;
    for (i = 1; i < 20; i++) {
      I[i] = I[i-1] * 2;
    }
    fprintf (f, "20 of type TYPE, %d bytes each\n", sizeof(TYPE));
    fwrite (I, sizeof(TYPE), 20, f);
  }


}
