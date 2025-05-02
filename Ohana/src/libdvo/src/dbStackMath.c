# include "dvo.h"

static int NallocBinary = 0;
static int NallocUnary = 0;

dbStack *dbBinary (dbStack *V1, dbStack *V2, char *op, dbValue *fields) {

  dbStack *OUT;
  
# define SS_FUNC(OP) {							\
    if (!(V1->type & DB_STACK_INT) && !(V2->type & DB_STACK_INT)) {	\
      opihi_flt M1 = (V1->type & DB_STACK_FIELD) ? fields[V1->field].Flt : V1->FltValue; \
      opihi_flt M2 = (V2->type & DB_STACK_FIELD) ? fields[V2->field].Flt : V2->FltValue; \
      OUT[0].type = DB_STACK_VALUE | DB_STACK_TEMP;			\
      OUT[0].FltValue = OP;						\
      break;								\
    }									\
    if ((V1->type & DB_STACK_INT) && !(V2->type & DB_STACK_INT)) {	\
      opihi_int M1 = (V1->type & DB_STACK_FIELD) ? fields[V1->field].Int : V1->IntValue; \
      opihi_flt M2 = (V2->type & DB_STACK_FIELD) ? fields[V2->field].Flt : V2->FltValue; \
      OUT[0].type = DB_STACK_VALUE | DB_STACK_TEMP;			\
      OUT[0].FltValue = OP;						\
      break;								\
    }									\
    if (!(V1->type & DB_STACK_INT) && (V2->type & DB_STACK_INT)) {	\
      opihi_flt M1 = (V1->type & DB_STACK_FIELD) ? fields[V1->field].Flt : V1->FltValue; \
      opihi_int M2 = (V2->type & DB_STACK_FIELD) ? fields[V2->field].Int : V2->IntValue; \
      OUT[0].type = DB_STACK_VALUE | DB_STACK_TEMP;			\
      OUT[0].FltValue = OP;						\
      break;								\
    }									\
    if ((V1->type & DB_STACK_INT) && (V2->type & DB_STACK_INT)) {	\
      opihi_int M1 = (V1->type & DB_STACK_FIELD) ? fields[V1->field].Int : V1->IntValue; \
      opihi_int M2 = (V2->type & DB_STACK_FIELD) ? fields[V2->field].Int : V2->IntValue; \
      OUT[0].type = DB_STACK_VALUE | DB_STACK_TEMP | DB_STACK_INT;	\
      OUT[0].IntValue = OP;						\
      break;								\
    }									\
  }

  NallocBinary ++;
  ALLOCATE (OUT, dbStack, 1);
  OUT->name = NULL;

  // XXX use an enum for the op...
  switch (op[0]) { 
    case '+': SS_FUNC(M1 + M2);
    case '-': SS_FUNC(M1 - M2);
    case '*': SS_FUNC(M1 * M2);
    case '/': SS_FUNC(M1 / M2);
    case '%': SS_FUNC((int) M1 % (int) M2);
    case '^': SS_FUNC(pow (M1, M2));
    case 'D': SS_FUNC(MIN (M1, M2));
    case 'U': SS_FUNC(MAX (M1, M2));
    case '<': SS_FUNC((M1 < M2) ? 1 : 0);
    case '>': SS_FUNC((M1 > M2) ? 1 : 0);
    case '&': SS_FUNC(((int)M1 & (int)M2));
    case '|': SS_FUNC(((int)M1 | (int)M2));
    case 'E': SS_FUNC((M1 == M2) ? 1 : 0);
    case 'N': SS_FUNC((M1 != M2) ? 1 : 0);
    case 'L': SS_FUNC((M1 <= M2) ? 1 : 0);
    case 'G': SS_FUNC((M1 >= M2) ? 1 : 0);
    case 'A': SS_FUNC((M1 && M2) ? 1 : 0);
    case 'O': SS_FUNC((M1 || M2) ? 1 : 0);
  default:
    return (NULL);
  }
# undef SS_FUNC
  
  return (OUT);
}

dbStack *dbUnary (dbStack *V1, char *op, dbValue *fields) {

  dbStack *OUT;
  
# define S_INT_FUNC(OP) {						\
    if (!(V1->type & DB_STACK_INT)) {					\
      /* function takes int type but input value is not int -> cast to opihi_int */ \
      opihi_int M1 = (V1->type & DB_STACK_FIELD) ? (opihi_int) fields[V1->field].Flt : (opihi_int) V1->FltValue; \
      OUT[0].type = DB_STACK_VALUE | DB_STACK_TEMP | DB_STACK_INT;      \
      OUT[0].IntValue = OP;						\
      return (OUT);							\
    } else {								\
      opihi_int M1 = (V1->type & DB_STACK_FIELD) ? fields[V1->field].Int : V1->IntValue; \
      OUT[0].type = DB_STACK_VALUE | DB_STACK_TEMP | DB_STACK_INT;	\
      OUT[0].IntValue = OP;						\
      return (OUT);							\
    }									\
  }

# define S_FLT_FUNC(OP) {						\
    if (!(V1->type & DB_STACK_INT)) {					\
      opihi_flt M1 = (V1->type & DB_STACK_FIELD) ? fields[V1->field].Flt : V1->FltValue; \
      OUT[0].type = DB_STACK_VALUE | DB_STACK_TEMP;			\
      OUT[0].FltValue = OP;						\
      return (OUT);							\
    } else {								\
      /* function takes non-int type but input value is int -> cast to opihi_flt */ \
      opihi_flt M1 = (V1->type & DB_STACK_FIELD) ? (opihi_flt) fields[V1->field].Int : (opihi_flt) V1->IntValue; \
      OUT[0].type = DB_STACK_VALUE | DB_STACK_TEMP;			\
      OUT[0].FltValue = OP;						\
      return (OUT);							\
    }									\
  }

  NallocUnary ++;
  ALLOCATE (OUT, dbStack, 1);
  OUT->name = NULL;

  if (!strcmp (op, "="))      S_FLT_FUNC(M1);
  if (!strcmp (op, "abs"))    S_FLT_FUNC(fabs(M1));
  if (!strcmp (op, "int"))    S_INT_FUNC((int)(M1));
  if (!strcmp (op, "exp"))    S_FLT_FUNC(exp (M1));
  if (!strcmp (op, "ten"))    S_FLT_FUNC(pow (10.0,M1));
  if (!strcmp (op, "log"))    S_FLT_FUNC(log10 (M1));
  if (!strcmp (op, "ln"))     S_FLT_FUNC(log (M1));
  if (!strcmp (op, "sqrt"))   S_FLT_FUNC(sqrt (M1));
  if (!strcmp (op, "erf"))    S_FLT_FUNC(erf (M1));
  if (!strcmp (op, "sinh"))   S_FLT_FUNC(sinh (M1));
  if (!strcmp (op, "cosh"))   S_FLT_FUNC(cosh (M1));
  if (!strcmp (op, "asinh"))  S_FLT_FUNC(asinh (M1));
  if (!strcmp (op, "acosh"))  S_FLT_FUNC(acosh (M1));
  if (!strcmp (op, "lgamma")) S_FLT_FUNC(lgamma (M1));
  if (!strcmp (op, "sin"))    S_FLT_FUNC(sin (M1));
  if (!strcmp (op, "cos"))    S_FLT_FUNC(cos (M1));
  if (!strcmp (op, "tan"))    S_FLT_FUNC(tan (M1));
  if (!strcmp (op, "dsin"))   S_FLT_FUNC(sin (M1*RAD_DEG));
  if (!strcmp (op, "dcos"))   S_FLT_FUNC(cos (M1*RAD_DEG));
  if (!strcmp (op, "dtan"))   S_FLT_FUNC(tan (M1*RAD_DEG));
  if (!strcmp (op, "asin"))   S_FLT_FUNC(asin (M1));
  if (!strcmp (op, "acos"))   S_FLT_FUNC(acos (M1));
  if (!strcmp (op, "atan"))   S_FLT_FUNC(atan (M1));
  if (!strcmp (op, "dasin"))  S_FLT_FUNC(asin (M1)*DEG_RAD);
  if (!strcmp (op, "dacos"))  S_FLT_FUNC(acos (M1)*DEG_RAD);
  if (!strcmp (op, "datan"))  S_FLT_FUNC(atan (M1)*DEG_RAD);
  if (!strcmp (op, "rnd"))    S_FLT_FUNC(M1*0.0 + drand48());
  if (!strcmp (op, "not"))    S_INT_FUNC(!(M1));
  if (!strcmp (op, "--"))     S_INT_FUNC(- (M1));
  if (!strcmp (op, "isinf"))  S_FLT_FUNC(!finite(M1));
  if (!strcmp (op, "isnan"))  S_FLT_FUNC(isnan(M1));

  return (OUT);
}

int dbStackAllocPrint () {

  fprintf (stderr, "dbAllocBinary: %d\n", NallocBinary);
  fprintf (stderr, "dbAllocUnary:  %d\n", NallocUnary);
  return (TRUE);
}

int dbStackAllocReset () {

  NallocBinary = 0;
  NallocUnary = 0;
  return (TRUE);
}
