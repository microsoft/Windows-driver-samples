/*
 * AES functions
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

#ifndef Dot11RAES_H
#define Dot11RAES_H

void *Dot11R_aes_encrypt_init(const u8 *key, int len , u32 *keyOut );
void Dot11R_aes_encrypt(void *ctx, const u8 *plain, u8 *crypt);
void Dot11R_aes_encrypt_deinit(void *ctx);
int  omac1_aes_128_vector(const u8 *key, int num_elem, const u8 *addr[], const int *len, u8 *mac);
int  omac1_aes_128(const u8 *key, const u8 *data, int data_len, u8 *mac);

//
// Now input data total length can't over 512 bytes !!
//
int wpa_ft_mic(const u8 *kck, const u8 *sta_addr, const u8 *ap_addr,
	       u8 transaction_seqnum, const u8 *mdie, int mdie_len,
	       const u8 *ftie, int ftie_len,
	       const u8 *rsnie, int rsnie_len,
	       const u8 *ric, int ric_len, u8 *mic);


int wpa_ft_mic_setup(const u8 *kck, const u8 *sta_addr, const u8 *ap_addr,
	       u8 transaction_seqnum, const u8 *linkidie, int linkidie_len,
	       const u8 *ftie, int ftie_len,
	       const u8 *rsnie, int rsnie_len,
	       const u8 *tie, int tie_len,
	       const u8 *ric, int ric_len, u8 *mic);

int wpa_ft_mic_teardown(const u8 *kck,
	       u8 transaction_seqnum, const u8 *linkidie, int linkidie_len,
	       const u8 *ftie, int ftie_len,
	       u16 reason_code, u8 dialog_token, u8 *mic);

#endif /* Dot11RAES_H */
