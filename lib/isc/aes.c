/*
 * Copyright (C) 2014, 2016  Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/* $Id$ */

/*! \file isc/aes.c */

#include "config.h"

#include <isc/assertions.h>
#include <isc/aes.h>
#include <isc/platform.h>
#include <isc/string.h>
#include <isc/types.h>
#include <isc/util.h>

#ifdef ISC_PLATFORM_WANTAES
#if HAVE_OPENSSL_EVP_AES

#include <openssl/evp.h>

void
isc_aes128_crypt(const unsigned char *key, const unsigned char *in,
		 unsigned char *out)
{
	EVP_CIPHER_CTX c;
	int len;

	EVP_CIPHER_CTX_init(&c);
	RUNTIME_CHECK(EVP_EncryptInit(&c, EVP_aes_128_ecb(), key, NULL) == 1);
	EVP_CIPHER_CTX_set_padding(&c, 0);
	RUNTIME_CHECK(EVP_EncryptUpdate(&c, out, &len, in,
					ISC_AES_BLOCK_LENGTH) == 1);
	RUNTIME_CHECK(len == ISC_AES_BLOCK_LENGTH);
	RUNTIME_CHECK(EVP_CIPHER_CTX_cleanup(&c) == 1);
}

void
isc_aes192_crypt(const unsigned char *key, const unsigned char *in,
		 unsigned char *out)
{
	EVP_CIPHER_CTX c;
	int len;

	EVP_CIPHER_CTX_init(&c);
	RUNTIME_CHECK(EVP_EncryptInit(&c, EVP_aes_192_ecb(), key, NULL) == 1);
	EVP_CIPHER_CTX_set_padding(&c, 0);
	RUNTIME_CHECK(EVP_EncryptUpdate(&c, out, &len, in,
					ISC_AES_BLOCK_LENGTH) == 1);
	RUNTIME_CHECK(len == ISC_AES_BLOCK_LENGTH);
	RUNTIME_CHECK(EVP_CIPHER_CTX_cleanup(&c) == 1);
}

void
isc_aes256_crypt(const unsigned char *key, const unsigned char *in,
		 unsigned char *out)
{
	EVP_CIPHER_CTX c;
	int len;

	EVP_CIPHER_CTX_init(&c);
	RUNTIME_CHECK(EVP_EncryptInit(&c, EVP_aes_256_ecb(), key, NULL) == 1);
	EVP_CIPHER_CTX_set_padding(&c, 0);
	RUNTIME_CHECK(EVP_EncryptUpdate(&c, out, &len, in,
					ISC_AES_BLOCK_LENGTH) == 1);
	RUNTIME_CHECK(len == ISC_AES_BLOCK_LENGTH);
	RUNTIME_CHECK(EVP_CIPHER_CTX_cleanup(&c) == 1);
}

#elif HAVE_OPENSSL_AES

#include <openssl/aes.h>

void
isc_aes128_crypt(const unsigned char *key, const unsigned char *in,
		 unsigned char *out)
{
	AES_KEY k;

	RUNTIME_CHECK(AES_set_encrypt_key(key, 128, &k) == 0);
	AES_encrypt(in, out, &k);
}

void
isc_aes192_crypt(const unsigned char *key, const unsigned char *in,
		 unsigned char *out)
{
	AES_KEY k;

	RUNTIME_CHECK(AES_set_encrypt_key(key, 192, &k) == 0);
	AES_encrypt(in, out, &k);
}

void
isc_aes256_crypt(const unsigned char *key, const unsigned char *in,
		 unsigned char *out)
{
	AES_KEY k;

	RUNTIME_CHECK(AES_set_encrypt_key(key, 256, &k) == 0);
	AES_encrypt(in, out, &k);
}

#elif PKCS11CRYPTO

#include <pk11/pk11.h>
#include <pk11/internal.h>

static CK_BBOOL truevalue = TRUE;
static CK_BBOOL falsevalue = FALSE;

static void isc_aes_crypt(const unsigned char *key, CK_ULONG keylen,
			  const unsigned char *in, unsigned char *out);

void
isc_aes128_crypt(const unsigned char *key, const unsigned char *in,
		 unsigned char *out)
{
	isc_aes_crypt(key, ISC_AES128_KEYLENGTH, in, out);
}

void
isc_aes192_crypt(const unsigned char *key, const unsigned char *in,
		 unsigned char *out)
{
	isc_aes_crypt(key, ISC_AES192_KEYLENGTH, in, out);
}

void
isc_aes256_crypt(const unsigned char *key, const unsigned char *in,
		 unsigned char *out)
{
	isc_aes_crypt(key, ISC_AES256_KEYLENGTH, in, out);
}

static void
isc_aes_crypt(const unsigned char *key, CK_ULONG keylen,
	      const unsigned char *in, unsigned char *out)
{
	CK_RV rv;
	CK_MECHANISM mech = { CKM_AES_ECB, NULL, 0 };
	CK_OBJECT_CLASS keyClass = CKO_SECRET_KEY;
	CK_KEY_TYPE keyType = CKK_AES;
	CK_ATTRIBUTE keyTemplate[] =
	{
		{ CKA_CLASS, &keyClass, (CK_ULONG) sizeof(keyClass) },
		{ CKA_KEY_TYPE, &keyType, (CK_ULONG) sizeof(keyType) },
		{ CKA_TOKEN, &falsevalue, (CK_ULONG) sizeof(falsevalue) },
		{ CKA_PRIVATE, &falsevalue, (CK_ULONG) sizeof(falsevalue) },
		{ CKA_ENCRYPT, &truevalue, (CK_ULONG) sizeof(truevalue) },
		{ CKA_VALUE, NULL, keylen }
	};
	CK_ULONG blocklen;
	CK_BYTE_PTR pData;
	pk11_context_t ctx;

	DE_CONST(key, keyTemplate[5].pValue);
	RUNTIME_CHECK(pk11_get_session(&ctx, OP_AES, ISC_TRUE, ISC_FALSE,
				       ISC_FALSE, NULL, 0) == ISC_R_SUCCESS);
	ctx.object = CK_INVALID_HANDLE;
	PK11_FATALCHECK(pkcs_C_CreateObject,
			(ctx.session, keyTemplate,
			 (CK_ULONG) 6, &ctx.object));
	INSIST(ctx.object != CK_INVALID_HANDLE);
	PK11_FATALCHECK(pkcs_C_EncryptInit,
			(ctx.session, &mech, ctx.object));

	DE_CONST(in, pData);
	blocklen = (CK_ULONG) ISC_AES_BLOCK_LENGTH;
	PK11_FATALCHECK(pkcs_C_Encrypt,
			(ctx.session,
			 pData, (CK_ULONG) ISC_AES_BLOCK_LENGTH,
			 out, &blocklen));
	RUNTIME_CHECK(blocklen == (CK_ULONG) ISC_AES_BLOCK_LENGTH);

	(void) pkcs_C_DestroyObject(ctx.session, ctx.object);
	ctx.object = CK_INVALID_HANDLE;
	pk11_return_session(&ctx);

}

#endif
#endif /* ISC_PLATFORM_WANTAES */
