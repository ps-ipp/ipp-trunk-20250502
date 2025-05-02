# include "dimm.h"

# define DIGITAL_OUT 0x40
# define ANALOG_IN_INIT 0x00
# define ANALOG_IN_HIGH 0xa1
# define ANALOG_IN_LOW  0x91

static char SerialConnected = FALSE;
static unsigned char DigitalOutState = 0x00;
static struct timeval reftime; 
static char reftimeset = FALSE;

# define DTIME(A,B) ((A.tv_sec - B.tv_sec) + 1e-6*(A.tv_usec - B.tv_usec))

int adc (int argc, char **argv) {
  
  struct timeval now;
  int i, N, mode, hi, lo, value;
  unsigned char setbit, output[5], *input;
  double dtime;

  if (N = get_argument (argc, argv, "-reset")) {
    remove_argument (N, &argc, argv);
    gettimeofday (&reftime, (struct timeval *) NULL);
    reftimeset = TRUE;
    return (TRUE);
  }

  if (argc != 4) {
    gprint (GP_ERR, "USAGE: adc ai N (variable)\n");
    gprint (GP_ERR, "USAGE: adc di N (variable)\n");
    gprint (GP_ERR, "USAGE: adc do N mode\n");
    return (FALSE);
  }

  if (!SerialConnected) {
    if (!SerialInit ('b')) {
      gprint (GP_ERR, "error opening serial line\n");
      return (FALSE);
    }
    SerialConnected = TRUE;
  }

  if (!reftimeset) {
    gettimeofday (&reftime, (struct timeval *) NULL);
    reftimeset = TRUE;
  }      

  gettimeofday (&now, (struct timeval *) NULL);
  dtime = DTIME (now, reftime);
  set_variable ("TIME", dtime);
  
  /* set the digital outputs */
  if (!strcmp (argv[1], "do")) {
    N = atof (argv[2]);
    if ((N < 1) || (N > 6)) {
      gprint (GP_ERR, "digital output is between 1 and 6\n");
      return (FALSE);
    }
    if (!strcasecmp (argv[3], "on")) {
      mode = TRUE;
    } else {
      mode = FALSE;
    }
    setbit = (0x01 << N-1);
    if (mode) {
      DigitalOutState |= setbit;
    } else {
      DigitalOutState &= ~setbit;
    }      
    output[0] = (DIGITAL_OUT | DigitalOutState);
    output[1] = 0;
    SerialCommand (output, &input, 50);
    gprint (GP_ERR, "%x (%x, %x) -> %x (%d, %d)\n", 
	     output[0], setbit, DigitalOutState, input[0], 
	     strlen(input), strlen(output));
    free (input);
    return (TRUE);
  }
  /* read the analog inputs */
  if (!strcmp (argv[1], "ai")) {
    N = atof (argv[2]);
    if ((N < 2) || (N > 16)) {
      gprint (GP_ERR, "analog input is between 2 and 16\n");
      gprint (GP_ERR, " (problem with 1 for the moment...)\n");
      return (FALSE);
    }
    /* init A/D converter */
    output[0] = (ANALOG_IN_INIT | (N-1));
    output[1] = 0;
    SerialCommand (output, &input, 50);
    free (input);

    usleep (10000);
    /* get high byte */
    output[0] = ANALOG_IN_HIGH;
    output[1] = 0;
    do {
      SerialCommand (output, &input, 50);
      hi = input[0];
      free (input);
    } while (hi & 0x80);

    output[0] = ANALOG_IN_LOW;
    output[1] = 0;
    SerialCommand (output, &input, 50);
    /* gprint (GP_ERR, "%x -> %x\n", output[0], input[0]); */
    lo = input[0];
    free (input);
    value = ((hi & 0x0f) << 8) | lo;
    if (hi & 0x10) { value = -value; }
    set_variable (argv[3], 0.0001*value);
    return (TRUE);
  }


}
