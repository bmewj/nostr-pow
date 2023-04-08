#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "sha256.h"

#define NONCE_MAX_LEN 32

/**
 * 
 * compute_nonce()
 * Computes the correct nonce to produce a sha256 hash with a target difficulty.
 * 
 * In pseudo-code it does the following:
 * 
 *     nonce = start
 *     loop:
 *         hash = sha256(prefix_str + str(nonce) + suffix_str))
 *         if difficulty(hash) >= target_difficulty: break
 *         nonce += step
 * 
 * Arguments:
 *   1. prefix_str:        NULL-terminated string that should precede nonce
 *   2. suffix_str:        NULL-terminated string that should follow nonce
 *   3. target_difficulty: the number of bits that should be zero in the resulting hash (max 64)
 *   4. start:             first nonce to check
 *   5. step:              how much to increment the nonce on each iteration
 *   6. interrupt:         pointer to integer that should be 0 by default.
 *                         if set to 1 will break the loop and return early
 * 
 * Returns: the nonce that produces a hash exceeding target difficulty
 *
 */
long compute_nonce(const char* prefix_str, const char* suffix_str, int target_difficulty, int start, int step, int* interrupt) {

    SHA256_CTX ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, (const unsigned char*)prefix_str, strlen(prefix_str));

    size_t suffix_len = strlen(suffix_str);

    // Prepare difficulty mask
    assert(target_difficulty <= 64);
    unsigned long difficulty_mask = 0;
    {
        int n = target_difficulty;
        unsigned char* ptr = (unsigned char*)(&difficulty_mask);
        while (n >= 8) {
            *ptr++ = 0xff;
            n -= 8;
        }
        switch (n) {
            case 7: *ptr = 0xfd; break;
            case 6: *ptr = 0xfb; break;
            case 5: *ptr = 0xf8; break;
            case 4: *ptr = 0xf0; break;
            case 3: *ptr = 0xd0; break;
            case 2: *ptr = 0xb0; break;
            case 1: *ptr = 0x80; break;
        }
    }

    // Prepare nonce string
    long nonce = start;
    char nonce_str[NONCE_MAX_LEN + 1];
    char* nonce_end = nonce_str + NONCE_MAX_LEN;
    int nonce_digits;
    {
        memset(nonce_str, '0', NONCE_MAX_LEN);
        nonce_str[NONCE_MAX_LEN] = '\0';

        char start_str[32];
        sprintf(start_str, "%d", start);
        nonce_digits = (int)strlen(start_str);
        strncpy(nonce_end - nonce_digits, start_str, nonce_digits);
    }

    long i = 0;
    while ((++i % 1000) != 0 || *interrupt == 0) {

        // Make a copy of our SHA256 ctx
        SHA256_CTX ctx_copy = ctx;
        sha256_update(&ctx_copy, (unsigned char*)(nonce_end - nonce_digits), nonce_digits);
        sha256_update(&ctx_copy, (const unsigned char*)suffix_str, suffix_len);

        unsigned char hash[32];
        sha256_final(&ctx_copy, hash);
        if ((*(unsigned long*)hash & difficulty_mask) == 0) {
            break;
        }

        // Update nonce string
        nonce += step;
        int inc = step;
        char* ptr = nonce_end - 1;
        while (1) {
            *ptr += inc;
            if (*ptr <= '9') {
                break;
            }

            // Overflow
            int rem = (*ptr - '0') % 10;
            inc = (*ptr - '0') / 10;
            *ptr = rem + '0';
            --ptr;
        }
        if (nonce_digits < (int)(nonce_end - ptr)) {
            nonce_digits = (int)(nonce_end - ptr);
        }
    }
    
    return nonce;
}
