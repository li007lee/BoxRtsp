/*
 * md5gen.c
 *
 *  Created on: 2014年12月12日
 *      Author: root
 */

//#include "my_iptable.h"
#include <string.h>
#include "md5.h"

#define HASHLEN 16
typedef unsigned char HASH[HASHLEN];
#define HASHHEXLEN 32
typedef char HASHHEX[HASHHEXLEN+1];
static const char hex_chars[] = "0123456789abcdef";

static char int2hex(char c)
{
	return hex_chars[(c & 0x0F)];
}

static void CvtHex(HASH Bin, HASHHEX Hex)
{
	unsigned short i;

	for (i = 0; i < HASHLEN; i++) {
		Hex[i*2] = int2hex((Bin[i] >> 4) & 0xf);
		Hex[i*2+1] = int2hex(Bin[i] & 0xf);
	}
	Hex[HASHHEXLEN] = '\0';
}

/*
* 密码比较，相等返回0，不等返回－1

static int password_compare(char *password, const char *pw)
{
	MD5_CTX Md5Ctx;
	char md5_pw[256];
	HASH HA1;

	MD5Init(&Md5Ctx);	//初始化
	MD5Update(&Md5Ctx, (unsigned char *)password, strlen(password)); //md5加密
	MD5Final(HA1, &Md5Ctx); //将加密后的密文放到HA1
	CvtHex(HA1, md5_pw); //将HA1转换为字符串存储

//	printf("md5pw = %s\n", md5_pw);
//	printf("pw = %s\n", pw);

	if (0 == strcmp(pw, md5_pw)){
		return 0;
	}
	else {
		return -1;
	}

	return 0;
}
*/
int md5_packages_string(char *desc_str, char *src_str, int src_str_len)
{
	MD5_CTX Md5Ctx;
//	char md5_pw[256];
	HASH HA1;

	MD5Init(&Md5Ctx);	//初始化
	MD5Update(&Md5Ctx, (unsigned char *)src_str, src_str_len); //md5加密
	MD5Final(HA1, &Md5Ctx); //将加密后的密文放到HA1
	CvtHex(HA1, desc_str); //将HA1转换为字符串存储
    return 1;
}


