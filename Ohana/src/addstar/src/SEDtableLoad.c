# include "sedstar.h"

// XXX where do the colorP and colorM codes come from?
SEDtable *SEDtableLoad (char *filename) {

  FILE *f;
  char line[1024], name[64], mode[64];
  char colornameP[64], colornameM[64];
  int colorP, colorM, code;
  int i, Nrow, NROW;
  SEDtable *table;
  SEDtableRow *raw;

  ALLOCATE (table, SEDtable, 1);

  // load SED table
  f = fopen (filename, "r");
  if (f == NULL) Shutdown ("failure to open SED table");


  // XXX add error checks for header data
  scan_line (f, line);
  sscanf (line, "%*s %*s %d", &table[0].Nfilter);

  // load SED table photcodes, generate the photcode hashtable
  ALLOCATE (table[0].hashcode, int, 0x10000);
  ALLOCATE (table[0].wavecode, float, table[0].Nfilter);
  ALLOCATE (table[0].vegaToAB, float, table[0].Nfilter);
  ALLOCATE (table[0].mode,     int,   table[0].Nfilter);
  ALLOCATE (table[0].code,     int,   table[0].Nfilter);

  for (i = 0; i < 0x10000; i++) table[0].hashcode[i] = -1;
  for (i = 0; i < table[0].Nfilter; i++) {
    scan_line (f, line);
    sscanf (line, "%*s %s %f %f %s", name, &table[0].wavecode[i], &table[0].vegaToAB[i], mode);
    code = GetPhotcodeCodebyName (name);
    table[0].code[i] = code;
    if (code == 0) Shutdown ("undefined photcode in SED table");
    table[0].hashcode[code] = i;

    table[0].mode[i] = -1;     
    if (!strcasecmp(mode, "fit")) table[0].mode[i] = SED_FIT; 
    if (!strcasecmp(mode, "req")) table[0].mode[i] = SED_REQ; 
    if (!strcasecmp(mode, "model")) table[0].mode[i] = SED_MODEL; 
    if (!strcasecmp(mode, "sample")) table[0].mode[i] = SED_SAMPLE; 
    if (table[0].mode[i] == -1) Shutdown ("invalid photcode mode in SED table");
  }

  // load color key
  scan_line (f, line);

  // define the selection color codes
  sscanf (line, "# %s - %s", colornameP, colornameM);
  colorP = GetPhotcodeCodebyName (colornameP);
  colorM = GetPhotcodeCodebyName (colornameM);
  table[0].codeP = table[0].hashcode[colorP];
  table[0].codeM = table[0].hashcode[colorM];
  if (table[0].codeP == -1) Shutdown ("missing positive color filter");
  if (table[0].codeM == -1) Shutdown ("missing positive color filter");
    
  // load the SED raw table rows
  Nrow = 0;
  NROW = 100;
  ALLOCATE (raw, SEDtableRow, NROW);
  while (scan_line(f, line) != EOF) {
    stripwhite (line);
    if (line[0] == '#') continue;
    fparse (&raw[Nrow].Temp, 1, line);
    fparse (&raw[Nrow].Av, 2, line);
    ALLOCATE (raw[Nrow].mags, float, table[0].Nfilter);
    for (i = 0; i < table[0].Nfilter; i++) {
      fparse (&raw[Nrow].mags[i], i + 3, line);
    }
    raw[Nrow].color = raw[Nrow].mags[table[0].codeP] - raw[Nrow].mags[table[0].codeM];
    Nrow ++;
    CHECK_REALLOCATE (raw, SEDtableRow, NROW, Nrow, 100);
  }      

  // sort the SEDtable by the reference colors
  table[0].row = sort_SEDtable (raw, Nrow);
  table[0].Nrow = Nrow;
  return (table);
}
