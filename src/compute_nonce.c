#include <string.h>
#include <assert.h>
#include "sha256.h"

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

    int num_digits = 1;
    long next_digit = 10;
    char buffer[32];
    unsigned char hash[32];

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

    long nonce = start;
    long i = 0;
    while ((++i % 1000) != 0 || *interrupt == 0) {

        // Convert the nonce to a string
        {
            while (nonce >= next_digit) {
                num_digits++;
                next_digit *= 10;
            }
            long rem = nonce;
            buffer[num_digits] = '\0';
            for (int digit = 0; digit < num_digits; ++digit) {
                buffer[num_digits - 1 - digit] = '0' + (rem % 10);
                rem /= 10;
            }
        }

        // Make a copy of our SHA256 ctx
        SHA256_CTX ctx_copy = ctx;
        sha256_update(&ctx_copy, (unsigned char*)buffer, num_digits);
        sha256_update(&ctx_copy, (const unsigned char*)suffix_str, suffix_len);
        sha256_final(&ctx_copy, hash);

        if ((*(unsigned long*)hash & difficulty_mask) == 0) {
            break;
        }

        nonce += step;
    }
    
    return nonce;
}
