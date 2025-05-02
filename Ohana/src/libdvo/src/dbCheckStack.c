# include "dvo.h"

// XXX we are using a fairly bogus set of char values to distinguish a few cases:
// 's', 'S' : value is a scalar (upper == float; lower == int)
// 'f', 'F' : value is a field (upper == float; lower == int)
// 't', 'T' : value is a temp scalar (upper == float; lower == int)
// these would be better done using bit values to test for field? temp? float?
//

int dbCheckStack (dbStack *stack, int Nstack, int table, dbField **inFields, int *inNfields) {

  int i, j, status, NFIELDS, Nfields;
  char *c1, *c2;
  dbField *fields;

  fields = *inFields;
  Nfields = *inNfields;

  NFIELDS = Nfields + 10;
  REALLOCATE (fields, dbField, NFIELDS);

  for (i = 0; i < Nstack; i++) {

    // BINARY operators
    if ((stack[i].type >= DB_STACK_LOGIC) && (stack[i].type <= DB_STACK_POWER)) {
      if (i < 2) {
	gprint (GP_ERR, "syntax error for field %s (binary operator missing operand)\n", stack[i].name);
	goto failure;
      }
    }

    // UNARY operators
    if (stack[i].type == DB_STACK_UNARY) {
      if (i < 1) {
	gprint (GP_ERR, "syntax error for field %s (unary operator missing operand)\n", stack[i].name);
	goto failure;
      }
    }

    if (stack[i].type == DB_STACK_VALUE) {

      /** if this is a number, put it on the list of scalars and move on.  assume value is
       * an int unless proven otherwise **/
      stack[i].FltValue = strtod (stack[i].name, &c1);
      stack[i].IntValue = strtoll (stack[i].name, &c2, 0);
      if (c2 == stack[i].name + strlen (stack[i].name)) {
	stack[i].type  |= DB_STACK_INT;
	continue;
      } 
      if (c1 == stack[i].name + strlen (stack[i].name)) {
	// this is a float value
	continue;
      } 

      // the value might be a special type of number: RA,DEC in sexigesimal or 
      // time in YYYY/MM/DD, etc
      // status = ohana_str_to_time (stack[i].name, &seconds);
      // keep as time_t
      // status = ohana_dms_to_ddd (&ddd, stack[i].name);
      // keep as either hours (RA) or deg (DEC).

      // strings protected by double quotes should parse to be special types of numbers
      // (photcodes, dates, ra / dec)
      if (stack[i].name[0] == '"') {
	  if (stack[i].name[strlen(stack[i].name) - 1] != '"') {
	      gprint (GP_ERR, "syntax error for field %s\n", stack[i].name);
	      goto failure;
	  }
	  char *tmpstring;
	  tmpstring = strncreate (&stack[i].name[1], strlen(stack[i].name) - 2);

	  // attempt to parse the string as a special word:
	  PhotCode *code;
	  code = GetPhotcodebyName (tmpstring);
	  if (code) {
	      stack[i].IntValue = code->code;
	      stack[i].type = DB_STACK_INT;
	      continue;
	  } 

	  // add in tests for sexigesimal, date/time
	  // TimeRefPM = ohana_date_to_sec ("2000/01/01");
	  
	  gprint (GP_ERR, "syntax error for field %s\n", stack[i].name);
	  goto failure;
      }

      // this must be a field : is it already in the list?
      for (j = 0; (j < Nfields) && strcasecmp (stack[i].name, fields[j].name); j++);
      if (j < Nfields) {
	stack[i].field = j;
	stack[i].type |= DB_STACK_FIELD;
	if (fields[j].type == OPIHI_INT) {
	  stack[i].type |= DB_STACK_INT;
	}
	stack[i].name  = NULL;
	stack[i].FltValue = 0.0;
	stack[i].IntValue =   0;
	continue;
      }

      // we are generating a new field (fields[Nfields]):
      dbInitField (&fields[Nfields]);

      // this must be a field : is it a valid name?
      status = FALSE;
      if (table == DVO_TABLE_MEASURE) {
	status = ParseMeasureField (&fields[Nfields], stack[i].name);
      } 
      if (table == DVO_TABLE_AVERAGE) {
	status = ParseAverageField (&fields[Nfields], stack[i].name);
      } 
      if (table == DVO_TABLE_IMAGE) {
	status = ParseImageField (&fields[Nfields], stack[i].name);
      } 
      if (!status) {
	goto failure;
      }
      stack[i].field = Nfields;
      stack[i].type |= DB_STACK_FIELD;
      if (fields[Nfields].type == OPIHI_INT) {
	stack[i].type |= DB_STACK_INT;
      }
      stack[i].name  = NULL;
      stack[i].FltValue = 0.0; // 'F'
      stack[i].IntValue =   0; // 'f'

      Nfields ++;
      CHECK_REALLOCATE (fields, dbField, NFIELDS, Nfields, 10);
    }
  }

  *inNfields = Nfields;
  *inFields = fields;
  return (TRUE);

failure:
  *inNfields = Nfields;
  *inFields = fields;
  return (FALSE);
}

/* check stack identifies the data elements as plain scalars or table fields
   operators have already been identified.  
   check stack returns the total stack dimensionality (0,1,2)
   on error, check stack returns FALSE
*/
