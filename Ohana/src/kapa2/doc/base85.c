/* base85.c : encode numbers using two different techniques for Base-85 
 * PUBLIC DOMAIN - Jon Mayo - September 10, 2008 */
#include <assert.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>

/* define this to use PDF/adobe Ascii85 encoding
 * default is to use an encoding derived from RFC1924, except 32-bits at a time instead of 128-bits
 * yes, I realize that RFC1924 is a joke RFC
 */
#define USE_ASCII85

#define NR(x) (sizeof(x)/sizeof*(x))

#define BASETYPE unsigned int

#define BASE85_DIGITS	5	 /* log85 (2^32) is 4.9926740807112 */
#ifndef USE_ASCII85
/* rfc1924 :) */
static const unsigned char base85[85] =  "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!#$%&()*+-;<=>?@^_`{|}~";
#else
/* ascii85 (adobe) */
static const unsigned char base85[85] =  "!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstu";
#endif
static signed char decode_table[UCHAR_MAX];

/* create a look up table suitable for convering characters to base85 digits */
static void base85_init(void) {
	unsigned char ch;

	assert((sizeof base85 == 85) && (base85[84] != 0)); /* make sure the array is exactly the right size */

	for(ch=0;ch<UCHAR_MAX;ch++) {
		decode_table[ch]=-1;
	}
	for(ch=0;ch<85;ch++) {
		decode_table[base85[ch]]=ch;
	}
}

/* convert a list of 32-bit values into a base85 string.
 * if you wish to encode 8-bit values, load them into 32-bit values in Big Endian order
 * example:
 *   input: "Lion"
 *   ascii85: 9PJE_
 *   base85: !aflO
 */
static int base85_encode(char *out, size_t max, const BASETYPE *data, size_t count) {
	size_t i;
	BASETYPE n;

	fprintf (stderr, "data[0]: %x, %d\n", data[0], (int)(data[0] % 85));

	while(count) {
		if(max<1) return 0; /* failure */
		n=*data++;
		count--;
#ifndef USE_ASCII85
		/* rfc1924 :) */
		for(i=0;max>0 && i<BASE85_DIGITS;i++) {
			*out++=base85[n%85];
			max--;
			n/=85;
		}
#else
		/* Ascii85 (adobe) */
		if(n==0) {
			*out++='z'; /* this is a special zero character */
			max--;
		} else {
			if(max<5) return 0; /* no room */
			for(i=BASE85_DIGITS;i--;) {
				out[i]=base85[n%85];
				n/=85;
			}
			max-=5;
			out+=5;
		}
#endif
	}
	*out=0;
	return 1; /* success */
}

/* convert a base85 string into a list of 32-bit values
 * treats string as if it were padded with 0s */
static int base85_decode(BASETYPE *out, size_t out_count, const char *in) {
	unsigned in_count;
	BASETYPE n, k;

	if(*in==0) return 0; /* nothing to decode */
	while(*in) {
		if(out_count<=0) return 0; /* failure - not enough space in destination */
		n=0;
#ifndef USE_ASCII85
		/* rfc1924 :) */
		for(in_count=0,k=1;*in && in_count<BASE85_DIGITS;in_count++) {
			signed d; /* digit */
			d=decode_table[(unsigned char)*in++];
			if(d<0) return 0; /* failure - invalid character */
			n+=k*d;
			k*=85;
		}
#else
		/* Ascii85 (adobe) */
		/* 'z' is a special way to encode 4 bytes of 0s */
		if(*in=='z') {
			in++;
		} else for(in_count=0,k=1;*in && in_count<BASE85_DIGITS;in_count++) {
			signed d; /* digit */
			d=decode_table[(unsigned char)*in++];
			if(d<0)  return 0; /* failure - invalid character */
			n=n*85+d;
		}

#endif
		*out++=n;
		out_count--;
	}
	return 1; /* success */
}

// ~0UL is long long, so probably not valid
static int base85_test(int verbose) {
	const BASETYPE testdata[] = {
		0x4c696f6e, 0x0ddba11, 0xba5eba11, 0xbeef, 0xcafe, 0xb00b, 0xdeadbea7, 0xdefec8, 0xbedabb1e, 0xf01dab1e, 0xf005ba11, 0xb01dface,
		0x5ca1ab1e, 0xcab005e, 0xdeadfa11, 0x1eadf007, 0xdefea7, 0, 1, (unsigned)-8, (unsigned)-9, 0x4d616e20, 0x206e614d, 0xffffffff, ~0UL,
	};
	char buf[5*NR(testdata)+1];
	BASETYPE testout[NR(testdata)];
	size_t i;

	base85_init();

	if(!base85_encode(buf, sizeof buf, testdata, NR(testdata))) return 0;

	if(verbose) printf("base85 = %s\n", buf);
	if(verbose) fwrite ((char *) testdata, 1, sizeof(testdata), stdout);

	if(!base85_decode(testout, NR(testout), buf)) return 0;

	for(i=0;i<NR(testdata);i++) {
		if(verbose) {
			printf("in[%zu] = %u 0x%x\n", i, testdata[i], testdata[i]);
			printf("out[%zu] = %u 0x%x\n", i, testout[i], testout[i]);
		}
		if(testdata[i]!=testout[i]) {
			return 0; /* failure */
		}
	}

	if(!base85_decode(&testout[0], 1, "Ll100")) return 0;
	if(!base85_decode(&testout[1], 1, "Ll1")) return 0;
	if(!base85_decode(&testout[2], 1, "00Ll1")) return 0;
	for(i=0;i<3;i++) {
		if(verbose) printf("misc[%zu] = %u 0x%x\n", i, testout[i], testout[i]);
	}
	
	return 1; /* pass */
}

int main(void) {
  if(base85_test(1)) {
    printf("base85 passed\n");
  } else {
    printf("base85 failed\n");
  }

  // char quote[] = "Man is distinguished, not only by his reason, but by this singular passion from other animals, which is a lust of the mind, that by a perseverance of delight in the continued and indefatigable generation of knowledge, exceeds the short vehemence of any carnal pleasure.";
  // int Nout = 5*((int)(strlen(quote)/4));

  // char quote[16];
  // memset (quote, 0, 16);
  // strcpy (quote, "noiL");

  // fprintf (stderr, "sizeof input %d\n", (int) sizeof(input));

  char quote[5];
  strcpy (quote, "noiL");

  int Nout = 5;
  char output[Nout];

  base85_init();
  if (!base85_encode (output, Nout, (BASETYPE *) quote, 1)) { fprintf (stderr, "failure\n"); }
  fwrite (output, 1, Nout, stdout);

  return 0;
}
