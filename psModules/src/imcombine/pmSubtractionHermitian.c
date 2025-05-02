#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <pslib.h>

#include "pmSubtractionTypes.h"
#include "pmSubtractionHermitian.h"

double p_pmSubtractionHermitianPolynomial (double x, int order) {
  double value;
    switch (order) {
      case 0:
	value = p_pmSubtractionHermitianPolynomial_00(x);
	break;
      case 1:
	value = p_pmSubtractionHermitianPolynomial_01(x);
	break;
      case 2:
	value = p_pmSubtractionHermitianPolynomial_02(x);
	break;
      case 3:
	value = p_pmSubtractionHermitianPolynomial_03(x);
	break;
      case 4:
	value = p_pmSubtractionHermitianPolynomial_04(x);
	break;
      case 5:
	value = p_pmSubtractionHermitianPolynomial_05(x);
	break;
      case 6:
	value = p_pmSubtractionHermitianPolynomial_06(x);
	break;
      case 7:
	value = p_pmSubtractionHermitianPolynomial_07(x);
	break;
      case 8:
	value = p_pmSubtractionHermitianPolynomial_08(x);
	break;
      case 9:
	value = p_pmSubtractionHermitianPolynomial_09(x);
	break;
      case 10:
	value = p_pmSubtractionHermitianPolynomial_10(x);
	break;
      default:
	value = NAN;
	break;
    }
    return value;
}

double p_pmSubtractionHermitianPolynomial_00(double x) {
    double value;
    // H_0(x) = 1
    value = 1;
    return value;
}
double p_pmSubtractionHermitianPolynomial_01(double x) {
    double value;
    // H_1(x) = x
    value = x;
    return value;
}
double p_pmSubtractionHermitianPolynomial_02(double x) {
    double value, x2;
    // H_2(x) = x^2-1
    x2 = x*x;
    value = x2 - 1.0;
    return value;
}
double p_pmSubtractionHermitianPolynomial_03(double x) {
    double value, x2;
    // H_3(x) = x^3-3x
    x2 = x*x;
    value = x*(x2 - 3.0);
    return value;
}
double p_pmSubtractionHermitianPolynomial_04(double x) {
    double value, x2;
    // H_4(x) = x^4-6x^2+3
    x2 = x*x;
    value = (x2 - 6.0)*x2 + 3.0;
    return value;
}
double p_pmSubtractionHermitianPolynomial_05(double x) {
    double value, x2;
    // H_5(x) = x^5-10x^3+15x
    x2 = x*x;
    value = ((x2 - 10.0)*x2 + 15.0)*x;
    return value;
}
double p_pmSubtractionHermitianPolynomial_06(double x) {
    double value, x2;
    // H_6(x) = x^6-15x^4+45x^2-15
    x2 = x*x;
    value = (((x2 - 15.0)*x2 + 45.0)*x2) - 15.0;
    return value;
}
double p_pmSubtractionHermitianPolynomial_07(double x) {
    double value, x2;
    // H_7(x) = x^7-21x^5+105x^3-105x
    x2 = x*x;
    value = (((x2 - 21.0)*x2+105.0)*x2 - 105.0)*x;
    return value;
}
double p_pmSubtractionHermitianPolynomial_08(double x) {
    double value, x2;
    // H_8(x) = x^8-28x^6+210x^4-420x^2+105
    x2 = x*x;
    value = ((((x2 - 28.0)*x2 + 210.0)*x2 - 420.0)*x2 + 105.0);
    return value;
}
double p_pmSubtractionHermitianPolynomial_09(double x) {
    double value, x2;
    // H_9(x) = x^9-36x^7+378x^5-1260x^3+945x
    x2 = x*x;
    value = ((((x2 - 36.0)*x2 + 378.0)*x2 - 1260.0)*x2 + 945.0)*x;
    return value;
}
double p_pmSubtractionHermitianPolynomial_10(double x) {
    double value, x2;
    // H_{10}(x) = x^{10}-45x^8+630x^6-3150x^4+4725x^2-945 
    x2 = x*x;
    value = (((((x2 - 45.0)*x2 + 630.0)*x2 - 3150.0)*x2 + 4725.0)*x2 - 945.0);
    return value;
}
