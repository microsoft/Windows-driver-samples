/*
 * aes_ocb.h
 *
 * Author:  Ted Krovetz (tdk@acm.org)
 * History: 1 April 2000 - first release (TK) - version 0.9
 *
 * OCB-AES-n reference code based on NIST submission "OCB Mode"
 * (dated 1 April 2000), submitted by Phillip Rogaway, with
 * auxiliary submitters Mihir Bellare, John Black, and Ted Krovetz.
 *
 * This code is freely available, and may be modified as desired.
 * Please retain the authorship and change history.
 * Note that OCB mode itself is patent pending.
 *
 * This code is NOT optimized for speed; it is only
 * designed to clarify the algorithm and to provide a point
 * of comparison for other implementations.
 *
 * Limitiations:  Assumes a 4-byte integer and pointers are
 * 32-bit aligned. Acts on a byte string of less than 2^{36} - 16 bytes.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __OCB__H
#define __OCB__H

#ifndef OCB_AES_KEY_BITLEN
#define OCB_AES_KEY_BITLEN   128  /* Must be 128, 192, 256 */
#endif

#if ((OCB_AES_KEY_BITLEN != 128) && \
     (OCB_AES_KEY_BITLEN != 192) && \
     (OCB_AES_KEY_BITLEN != 256))
#error Bad -- OCB_AES_KEY_BITLEN must be one of 128, 192 or 256!!
#endif



/* 
 * This implementation precomputes L(-1), L(0), L(1), L(PRE_COMP_BLOCKS),
 * where L(0) = L and L(-1) = L/x and L(i) = x*L(i) for i>0.  
 * Normally, one would select PRE_COMP_BLOCKS to be a small number
 * (like 0-6) and compute any larger L(i) values "on the fly", when they
 * are needed.  This saves space in _keystruct and needn't adversely
 * impact running time.  But in this implementation, to keep things as 
 * simple as possible, we compute all the L(i)-values we might ever see.
 */ 
#define PRE_COMP_BLOCKS 31     /* Must be between 0 and 31 */

#define OCB_AES_ROUNDS (OCB_AES_KEY_BITLEN / 32 + 6)

#ifdef REMOVE_PACK
#pragma pack(1)
#endif

typedef unsigned char rblock[16];

struct _keystruct {
    u4Byte rek[4*(OCB_AES_ROUNDS+1)]; /* AES encryption key                */
    u4Byte rdk[4*(OCB_AES_ROUNDS+1)]; /* AES decryption key                */
    u4Byte tag_len;               /* Sizeof tags to generate/validate  */
    rblock L[PRE_COMP_BLOCKS+1];     /* Precomputed L(i) values, L[0] = L */
    rblock L_inv;                    /* Precomputed L/x value             */
};

/* Opaque forward declaration of key structure */ 
typedef struct _keystruct keystruct;


#ifdef REMOVE_PACK
#pragma pack()
#endif

/*
 * "ocb_aes_init" optionally creates an ocb keystructure in memory
 * and then initializes it using the supplied "enc_key". "tag_len"
 * specifies the length of tags that will subsequently be generated
 * and verified. If "key" is NULL a new structure will be created, but
 * if "key" is non-NULL, then it is assumed that it points to a previously
 * allocated structure, and that structure is initialized. "ocb_aes_init"
 * returns a pointer to the initialized structure, or NULL if an error
 * occurred.
 */
keystruct *                         /* Init'd keystruct or NULL      */
ocb_aes_init(void      *enc_key,    /* AES key                       */
             ULONG		tag_len,    /* Length of tags to be used     */
             keystruct *key);       /* OCB key structure. NULL means */
                                    /* Allocate/init new, non-NULL   */
                                    /* means init existing structure */

/* "ocb_done deallocates a key structure and returns NULL */
keystruct *
ocb_done(keystruct *key);
                              
/*
 * "ocb_aes_encrypt takes a key structure, four buffers and a length
 * parameter as input. "pt_len" bytes that are pointed to by "pt" are
 * encrypted and written to the buffer pointed to by "ct". A tag of length
 * "tag_len" (set in ocb_aes_init) is written to the "tag" buffer. "nonce"
 * must be a 16-byte buffer which changes for each new message being
 * encrypted. "ocb_aes_encrypt" always returns a value of 1.
 */
void
ocb_aes_encrypt(keystruct *key,    /* Initialized key struct           */
                void      *nonce,  /* 16-byte nonce                    */
                void      *pt,     /* Buffer for (incoming) plaintext  */
                ULONG	  pt_len, /* Byte length of pt                */
                void      *ct,     /* Buffer for (outgoing) ciphertext */
                void      *tag);   /* Buffer for generated tag         */

                              
/*
 * "ocb_aes_decrypt takes a key structure, four buffers and a length
 * parameter as input. "ct_len" bytes that are pointed to by "ct" are
 * decrypted and written to the buffer pointed to by "pt". A tag of length
 * "tag_len" (set in ocb_aes_init) is read from the "tag" buffer. "nonce"
 * must be a 16-byte buffer which changes for each new message being
 * encrypted. "ocb_aes_decrypt" returns 0 if the supplied
 * tag is not correct for the supplied message, otherwise 1 is returned if
 * the tag is correct.
 */
int                                /* Returns 0 iff tag is incorrect   */
ocb_aes_decrypt(keystruct *key,    /* Initialized key struct           */
                void      *nonce,  /* 16-byte nonce                    */
                void      *ct,     /* Buffer for (incoming) ciphertext */
                ULONG     ct_len, /* Byte length of ct                */
                void      *pt,     /* Buffer for (outgoing) plaintext  */
                void      *tag);   /* Tag to be verified               */
                     



void
pmac_aes (keystruct *key,    /* Initialized key struct           */
          void      *in,     /* Buffer for (incoming) message    */
          ULONG     in_len, /* Byte length of message           */
          void      *tag);    /* 16-byte buffer for generated tag */

#endif /* __OCB__H */
