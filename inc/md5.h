/*
 * md5.h
 *
 *  Created on: 2014年12月12日
 *      Author: root
 */

#ifndef MD5_H_
#define MD5_H_


typedef struct
{
    unsigned int state[4]; /* state (ABCD) */
    unsigned int count[2]; /* number of bits, modulo 2^64 (lsb first) */
    unsigned char buffer[64]; /* input buffer */
} MD5_CTX, *PMD5_CTX;

void MD5Init(MD5_CTX *context);
void MD5Update(MD5_CTX *context, unsigned char *input, unsigned int inputLen);
void MD5Final(unsigned char* digest, MD5_CTX *context);
int MD5Encode(unsigned char *szEncoded, const unsigned char *szData,
int nSize, unsigned char *szKey, int nKeyLen);
int chap_auth(char *hash, char id, char *pwd, int pwd_size, char *chal, int chal_size);
int MD5_enc(char id, char *pwd, char pwdlen, char *hash);


#endif /* MD5_H_ */
