#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <string.h>

#include "md5.h"

#include "psAssert.h"
#include "psMemory.h"
#include "psString.h"
#include "psVector.h"
#include "psImage.h"
#include "psError.h"
#include "psMD5.h"


#define MD5_DIGEST_LENGTH 16            // Length of an MD5 digest, in bytes

psVector *psStringMD5(const char *string)
{
    psVector *hash = psVectorAlloc(MD5_DIGEST_LENGTH, PS_TYPE_U8); // The resultant MD5 hash
    md5_state_t buffer;                 // Calculation buffer
    md5_init(&buffer);
    md5_append(&buffer, (psU8*)string, strlen(string));
    md5_finish(&buffer, &hash->data.U8[0]);
    return hash;
}

psVector *psVectorMD5(const psVector *vector)
{
    PS_ASSERT_VECTOR_NON_NULL(vector, NULL);

    psVector *hash = psVectorAlloc(MD5_DIGEST_LENGTH, PS_TYPE_U8); // The resultant MD5 hash
    md5_state_t buffer;                 // Calculation buffer
    md5_init(&buffer);
    md5_append(&buffer, vector->data.U8, vector->n * PSELEMTYPE_SIZEOF(vector->type.type));
    md5_finish(&buffer, &hash->data.U8[0]);
    return hash;
}


psVector *psImageMD5(const psImage *image)
{
    PS_ASSERT_IMAGE_NON_NULL(image, NULL);

    psVector *hash = psVectorAlloc(MD5_DIGEST_LENGTH, PS_TYPE_U8); // The resultant MD5 hash
    md5_state_t buffer;                 // Calculation buffer
    md5_init(&buffer);
    if (!image->parent) {
        // No parent means image is contiguous
        md5_append(&buffer, image->data.U8[0],
                   image->numCols * image->numRows * PSELEMTYPE_SIZEOF(image->type.type));
    } else {
        for (int row = 0; row < image->numRows; row++) {
            md5_append(&buffer, image->data.U8[row],
                       image->numCols * PSELEMTYPE_SIZEOF(image->type.type));
        }
    }
    md5_finish(&buffer, &hash->data.U8[0]);

    return hash;
}


psString psMD5toString(const psVector *hash)
{
    PS_ASSERT_VECTOR_NON_NULL(hash, NULL);
    PS_ASSERT_VECTOR_SIZE(hash, (long int)MD5_DIGEST_LENGTH, NULL);
    PS_ASSERT_VECTOR_TYPE(hash, PS_TYPE_U8, NULL);

    psString string = psStringAlloc(MD5_DIGEST_LENGTH * 2 + 1); // String to return
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        sprintf(string + i * 2, "%02x", hash->data.U8[i]);
    }
    return string;
}
