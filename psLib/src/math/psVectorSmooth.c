#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>

#include "psMemory.h"
#include "psError.h"
#include "psLogMsg.h"
#include "psVector.h"
#include "psAssert.h"
#include "psConstants.h"

#include "psVectorSmooth.h"

psVector *psVectorSmooth(psVector *output,
                         const psVector *input,
                         double sigma,
                         double Nsigma
                        )
{
    PS_ASSERT_VECTOR_NON_NULL(input, NULL);
    PS_ASSERT_FLOAT_LARGER_THAN(sigma, 0.0, NULL);
    PS_ASSERT_FLOAT_LARGER_THAN(Nsigma, 0.0, NULL);

    if (output == input) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Cannot smooth vector in-place.");
        return NULL;
    }

    // relevant terms
    long Nrange = sigma*Nsigma + 0.5;   // Extent of smoothing
    long Npixel = 2*Nrange + 1;         // Total size of smoothing kernel
    long Nbin = input->n;               // Number of elements
    double factor = -0.5/(sigma*sigma); // Factor for Gaussian

    if (Nbin < Npixel) {
	// cannot smooth narrow vector
	return NULL;
    }

    #define VECTOR_SMOOTH_CASE(TYPE) \
case PS_TYPE_##TYPE: { \
        output = psVectorRecycle(output, Nbin, PS_TYPE_##TYPE); \
        /* generate normalized gaussian */ \
        psVector *gaussnorm = psVectorAlloc(Npixel, PS_TYPE_##TYPE); \
        double sum = 0.0; \
        for (long i = -Nrange; i < Nrange + 1; i++) { \
            gaussnorm->data.TYPE[i+Nrange] = exp(factor*i*i); \
            sum += gaussnorm->data.TYPE[i+Nrange]; \
        } \
        for (long i = -Nrange; i < Nrange + 1; i++) { \
            gaussnorm->data.TYPE[i+Nrange] /= sum; \
        } \
        ps##TYPE *gauss = &gaussnorm->data.TYPE[Nrange]; \
        \
        /* smooth vector */ \
        psVector *temp = psVectorAlloc(Nbin, PS_TYPE_##TYPE); \
        ps##TYPE *vi = input->data.TYPE; \
        ps##TYPE *vo = temp->data.TYPE; \
        /* smooth first Nrange pixels, with renorm */ \
        /* XXX need to check that this does not run over end for narrow vectors */ \
        for (long i = 0; i < Nrange; i++, vi++, vo++) {	\
            ps##TYPE *vr = vi - i; \
            ps##TYPE *vg = gauss - i; \
            double g = 0; \
            double s = 0; \
            for (int n = -i; n < Nrange + 1; n++, vr++, vg++) { \
                s += *vg * *vr; \
                g += *vg; \
            } \
            *vo = s / g; \
        } \
        /* smooth middle pixels */ \
        for (long i = Nrange; i < Nbin - Nrange; i++, vi++, vo++) { \
            ps##TYPE *vr = vi - Nrange; \
            ps##TYPE *vg = gauss - Nrange; \
            double s = 0; \
            for (int n = -Nrange; n < Nrange + 1; n++, vr++, vg++) { \
                s += *vg * *vr; \
            } \
            *vo = s; \
        } \
        /* smooth last Nrange pixels, with renorm */ \
        /* XXX does this miss the last column? */ \
        for (long i = Nbin - Nrange; i < Nbin; i++, vi++, vo++) { \
            ps##TYPE *vr = vi - Nrange; \
            ps##TYPE *vg = gauss - Nrange; \
            double g = 0; \
            double s = 0; \
            for (int n = -Nrange; n < Nbin - i - 1; n++, vr++, vg++) { \
                s += *vg * *vr; \
                g += *vg; \
            } \
            *vo = s / g; \
        } \
        memcpy(output->data.TYPE, temp->data.TYPE, Nbin*sizeof(ps##TYPE)); \
        psFree(temp); \
        psFree(gaussnorm); \
        break; \
    }

    switch (input->type.type) {
        VECTOR_SMOOTH_CASE(F32);
        VECTOR_SMOOTH_CASE(F64);
    default: {
            char* typeStr;
            PS_TYPE_NAME(typeStr, input->type.type);
            psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Type %s is not valid.", typeStr);
            return NULL;
        }
    }
    return output;
}



psVector *psVectorBoxcar(psVector *output,
                         const psVector *input,
                         int size
                        )
{
    PS_ASSERT_VECTOR_NON_NULL(input, NULL);
    PS_ASSERT_INT_POSITIVE(size, NULL);

    if (output == input) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Cannot smooth vector in-place.");
        return false;
    }

    long num = input->n;                // Number of elements
    output = psVectorRecycle(output, num, input->type.type);
    psVector *nums = psVectorAlloc(num, PS_TYPE_U32); // Number of elements in each bin
    psU32 *numsData = nums->data.U32;   // Dereferenced version

    psVectorInit(output, 0.0);
    psVectorInit(nums, 0);


    #define VECTOR_BOXCAR_CASE(TYPE) \
  case PS_TYPE_##TYPE: { \
      /* Dereference data */ \
      ps##TYPE *outputData = output->data.TYPE; \
      ps##TYPE *inputData = input->data.TYPE; \
      /* Smooth the vector */ \
      for (long i = 0; i < num; i++) { \
          for (long j = PS_MAX(0, i - size); j < PS_MIN(num, i + size + 1); j++) { \
              outputData[j] += inputData[i]; \
              numsData[j]++; \
          } \
      } \
      /* Normalisation */ \
      for (long i = 0; i < num; i++) { \
          outputData[i] /= numsData[i]; \
      } \
      break; \
  }

    switch (input->type.type) {
        VECTOR_BOXCAR_CASE(F32);
        VECTOR_BOXCAR_CASE(F64);
    default: {
            char* typeStr;
            PS_TYPE_NAME(typeStr, input->type.type);
            psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Type %s is not valid.", typeStr);
            psFree(nums);
            return NULL;
        }
    }
    psFree(nums);
    return output;
}
