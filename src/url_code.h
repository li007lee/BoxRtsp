/*
 * url_code.h
 *
 *  Created on: 2018年1月30日
 *      Author: root
 */

#ifndef SRC_URL_CODE_H_
#define SRC_URL_CODE_H_

#include "my_include.h"

int url_encode(const char* source, int srcBytes, char* target, int tgtBytes);
int url_decode(const char* source, int srcBytes, char* target, int tgtBytes);

#endif /* SRC_URL_CODE_H_ */
