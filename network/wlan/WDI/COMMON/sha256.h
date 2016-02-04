/*
 * SHA256 hash implementation and interface functions
 * Copyright (c) 2003-2006, Jouni Malinen <j@w1.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 * See README and COPYING for more details.
 */

#ifndef SHA256_H
#define SHA256_H

#define SHA256_MAC_LEN 32

// debug hpfan
void sha256_vector2(int num_elem, const u8 *addr, const int *len,
		 u8 *mac);

void hmac_sha256_vector(const u8 *key, int  key_len, int num_elem,
		      const u8 *addr[], const int *len, u8 *mac);
void hmac_sha256(const u8 *key, int key_len, const u8 *data,
		 int data_len, u8 *mac);
void sha256_prf(const u8 *key, int key_len, const char *label,
	      const u8 *data, int data_len, u8 *buf, int buf_len);


#endif /* SHA256_H */
