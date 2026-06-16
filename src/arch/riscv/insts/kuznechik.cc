/*
*  Copyright (c) 2026, Volkov Roman A. <4475211@gmail.com>
*  All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met: redistributions of source code must retain the above copyright
* notice, this list of conditions and the following disclaimer;
* redistributions in binary form must reproduce the above copyright
* notice, this list of conditions and the following disclaimer in the
* documentation and/or other materials provided with the distribution;
* neither the name of the copyright holders nor the names of its
* contributors may be used to endorse or promote products derived from
* this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
* A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
* OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
* THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "arch/riscv/utility.hh"
#include "arch/riscv/insts/kuznechik.hh"
#include <cstdint>
#include <stdio.h>
#include <string.h>


namespace gem5
{

namespace RiscvISA
{

constexpr int BLOCK_SIZE = 16;

const unsigned char kuzn_Pi[256] = {
    0xFC, 0xEE, 0xDD, 0x11, 0xCF, 0x6E, 0x31, 0x16, 
    0xFB, 0xC4, 0xFA, 0xDA, 0x23, 0xC5, 0x40, 0x4D, 
    0xE9, 0x77, 0xF0, 0xDB, 0x93, 0x2E, 0x99, 0xBA, 
    0x17, 0x36, 0xF1, 0xBB, 0x14, 0xCD, 0x5F, 0xC1, 
    0xF9, 0x18, 0x65, 0x5A, 0xE2, 0x5C, 0xEF, 0x21, 
    0x81, 0x1C, 0x3C, 0x42, 0x8B, 0x10, 0x8E, 0x4F, 
    0x50, 0x84, 0x20, 0xAE, 0xE3, 0x6A, 0x8F, 0xA0, 
    0x60, 0xB0, 0xED, 0x98, 0x7F, 0xD4, 0xD3, 0x1F, 
    0xEB, 0x34, 0x2C, 0x51, 0xEA, 0xC8, 0x48, 0xAB, 
    0xF2, 0x2A, 0x68, 0xA2, 0xFD, 0x3A, 0xCE, 0xCC, 
    0xB5, 0x70, 0xE0, 0x56, 0x80, 0xC0, 0x76, 0x12, 
    0xBF, 0x72, 0x13, 0x47, 0x9C, 0xB7, 0x5D, 0x87, 
    0x15, 0xA1, 0x96, 0x29, 0x10, 0x7B, 0x9A, 0xC7, 
    0xF3, 0x91, 0x78, 0x6F, 0x9D, 0x9E, 0xB2, 0xB1, 
    0x32, 0x75, 0x19, 0x3D, 0xFF, 0x35, 0x8A, 0x7E, 
    0x6D, 0x54, 0xC6, 0x80, 0xC3, 0xBD, 0xD0, 0x57, 
    0xDF, 0xF5, 0x24, 0xA9, 0x3E, 0xA8, 0x43, 0xC9, 
    0xD7, 0x79, 0xD6, 0xF6, 0x7C, 0x22, 0xB9, 0x30, 
    0xE0, 0xF0, 0xEC, 0xDE, 0x7A, 0x94, 0xB0, 0xBC, 
    0xDC, 0xE8, 0x28, 0x50, 0x4E, 0x33, 0xA0, 0x4A, 
    0xA7, 0x97, 0x60, 0x73, 0x1E, 0x00, 0x62, 0x44, 
    0x1A, 0xB8, 0x38, 0x82, 0x64, 0x9F, 0x26, 0x41, 
    0xAD, 0x45, 0x46, 0x92, 0x27, 0x5E, 0x55, 0x2F, 
    0x8C, 0xA3, 0xA5, 0x7D, 0x69, 0xD5, 0x95, 0x3B, 
    0x70, 0x58, 0xB3, 0x40, 0x86, 0xAC, 0x1D, 0xF7, 
    0x30, 0x37, 0x6B, 0xE4, 0x88, 0xD9, 0xE7, 0x89, 
    0xE1, 0x1B, 0x83, 0x49, 0x4C, 0x3F, 0xF8, 0xFE, 
    0x8D, 0x53, 0xAA, 0x90, 0xCA, 0xD8, 0x85, 0x61, 
    0x20, 0x71, 0x67, 0xA4, 0x2D, 0x2B, 0x90, 0x5B, 
    0xCB, 0x9B, 0x25, 0xD0, 0xBE, 0xE5, 0x6C, 0x52, 
    0x59, 0xA6, 0x74, 0xD2, 0xE6, 0xF4, 0xB4, 0xC0, 
    0xD1, 0x66, 0xAF, 0xC2, 0x39, 0x4B, 0x63, 0xB6
};

const unsigned char kuzn_reverse_Pi[256] = {
    0xA5, 0x2D, 0x32, 0x8F, 0x0E, 0x30, 0x38, 0xC0,
    0x54, 0xE6, 0x9E, 0x39, 0x55, 0x7E, 0x52, 0x91,
    0x64, 0x03, 0x57, 0x5A, 0x1C, 0x60, 0x07, 0x18,
    0x21, 0x72, 0xA8, 0xD1, 0x29, 0xC6, 0xA4, 0x3F,
    0xE0, 0x27, 0x8D, 0x0C, 0x82, 0xEA, 0xAE, 0xB4,
    0x9A, 0x63, 0x49, 0xE5, 0x42, 0xE4, 0x15, 0xB7,
    0xC8, 0x06, 0x70, 0x9D, 0x41, 0x75, 0x19, 0xC9,
    0xAA, 0xFC, 0x4D, 0xBF, 0x2A, 0x73, 0x84, 0xD5,
    0xC3, 0xAF, 0x2B, 0x86, 0xA7, 0xB1, 0xB2, 0x5B,
    0x46, 0xD3, 0x9F, 0xFD, 0xD4, 0x0F, 0x9C, 0x2F,
    0x9B, 0x43, 0xEF, 0xD9, 0x79, 0xB6, 0x53, 0x7F,
    0xC1, 0xF0, 0x23, 0xE7, 0x25, 0x5E, 0xB5, 0x1E,
    0xA2, 0xDF, 0xA6, 0xFE, 0xAC, 0x22, 0xF9, 0xE2,
    0x4A, 0xBC, 0x35, 0xCA, 0xEE, 0x78, 0x05, 0x6B,
    0x51, 0xE1, 0x59, 0xA3, 0xF2, 0x71, 0x56, 0x11,
    0x6A, 0x89, 0x94, 0x65, 0x8C, 0xBB, 0x77, 0x3C,
    0x7B, 0x28, 0xAB, 0xD2, 0x31, 0xDE, 0xC4, 0x5F,
    0xCC, 0xCF, 0x76, 0x2C, 0xB8, 0xD8, 0x2E, 0x36,
    0xDB, 0x69, 0xB3, 0x14, 0x95, 0xBE, 0x62, 0xA1,
    0x3B, 0x16, 0x66, 0xE9, 0x5C, 0x6C, 0x6D, 0xAD,
    0x37, 0x61, 0x4B, 0xB9, 0xE3, 0xBA, 0xF1, 0xA0,
    0x85, 0x83, 0xDA, 0x47, 0xC5, 0xB0, 0x33, 0xFA,
    0x96, 0x6F, 0x6E, 0xC2, 0xF6, 0x50, 0xFF, 0x5D,
    0xA9, 0x8E, 0x17, 0x1B, 0x97, 0x7D, 0xEC, 0x58,
    0xF7, 0x1F, 0xFB, 0x7C, 0x09, 0x0D, 0x7A, 0x67,
    0x45, 0x87, 0xDC, 0xE8, 0x4F, 0x1D, 0x4E, 0x04,
    0xEB, 0xF8, 0xF3, 0x3E, 0x3D, 0xBD, 0x8A, 0x88,
    0xDD, 0xCD, 0x0B, 0x13, 0x98, 0x02, 0x93, 0x80,
    0x90, 0xD0, 0x24, 0x34, 0xCB, 0xED, 0xF4, 0xCE,
    0x99, 0x10, 0x44, 0x40, 0x92, 0x3A, 0x01, 0x26,
    0x12, 0x1A, 0x48, 0x68, 0xF5, 0x81, 0x8B, 0xC7,
    0xD6, 0x20, 0x0A, 0x08, 0x00, 0x4C, 0xD7, 0x74
};

const unsigned char kuzn_L_vect[16] = {
    1, 148, 32, 133, 16, 194, 192, 1,
    251, 1, 192, 194, 16, 133, 32, 148
};

const char *kuzn_iter_C_hex[32] = {
    "6ea276726c487ab85d27bd10dd849401",
    "dc87ece4d890f4b3ba4eb92079cbeb02",
    "b2259a96b4d88e0be7690430a44f7f03",
    "7bcd1b0b73e32ba5b79cb140f2551504",
    "156f6d791fab511deabb0c502fd18105",
    "a74af7efab73df160dd208608b9efe06",
    "c9e8819dc73ba5ae50f5b570561a6a07",
    "f6593616e6055689adfba18027aa2a08",
    "98fb40648a4d2c31f0dc1c90fa2ebe09",
    "2adedaf23e95a23a17b518a05e61c10a",
    "447cac8052ddd8824a92a5b083e5550b",
    "8d942d1d95e67d2c1a6710c0d5ff3f0c",
    "e3365b6ff9ae07944740add0087bab0d",
    "5113c1f94d76899fa029a9e0ac34d40e",
    "3fb1b78b213ef327fd0e14f071b0400f",
    "2fb26c2c0f0aacd1993581c34e975410",
    "41101a5e6342d669c4123cd39313c011",
    "f33580c8d79a5862237b38e3375cbf12",
    "9d97f6babbd222da7e5c85f3ead82b13",
    "547f77277ce987742ea93083bcc24114",
    "3add015510a1fdcc738e8d936146d515",
    "88f89bc3a47973c794e789a3c509aa16",
    "e65aedb1c831097fc9c034b3188d3e17",
    "d9eb5a3ae90ffa5834ce2043693d7e18",
    "b7492c48854780e069e99d53b4b9ea19",
    "056cb6de319f0eeb8e80996310f6951a",
    "6bcec0ac5dd77453d3a72473cd72011b",
    "a22641319aecd1fd835291039b686b1c",
    "cc843743f6a4ab45de752c1346ecff1d",
    "7ea1add5427c254e391c2823e2a3801e",
    "1003dba72e345ff6643b95333f27141f",
    "5ea7d8581e149b61f16ac1459ceda820"
}; // Итерационные константы C


void kuzn_xor_arrays(const uint8_t *a, const uint8_t *b, uint8_t *c)
{
    int i;
    for (i = 0; i < BLOCK_SIZE; i++)
        c[i] = a[i] ^ b[i];
}

void kuzn_unlinear_S(const uint8_t *in_data, uint8_t *out_data)
{
    int i;
    for (i = 0; i < BLOCK_SIZE; i++)
        out_data[i] = kuzn_Pi[in_data[i]];
}

void kuzn_inv_unlinear_S(const uint8_t *in_data, uint8_t *out_data)
{
    int i;
    for (i = 0; i < BLOCK_SIZE; i++)
           out_data[i] = kuzn_reverse_Pi[in_data[i]];
}

uint8_t kuzn_multiply_in_F(uint8_t a, uint8_t b)
{
    uint8_t c = 0;
    uint8_t hi_bit;
    int i;
    for (i = 0; i < 8; i++)
    {
        if (b & 1)
            c ^= a;
        hi_bit = a & 0x80;
        a <<= 1;
        if (hi_bit)
            a ^= 0xc3; // Полином x^8 + x^7 + x^6 + x + 1
        b >>= 1;
    }
    return c;
}

void kuzn_lshift_R(uint8_t *arr)
{
    int i;
    uint8_t a_15 = 0;
    uint8_t temp[BLOCK_SIZE];
    for (i = 15; i >= 1; i--)
    {
        temp[i - 1] = arr[i]; // Двигаем байты в сторону младшего разряда
        a_15 ^= kuzn_multiply_in_F(arr[i], kuzn_L_vect[i]);
    }
    a_15 ^= kuzn_multiply_in_F(arr[0], kuzn_L_vect[0]);
    // Пишем в последний байт результат сложения
    temp[15] = a_15;
    memcpy(arr, temp, BLOCK_SIZE);
}

void kuzn_inv_lshift_R(uint8_t *state)
{
    int i;
    uint8_t a_0;
    a_0 = state[15];
    uint8_t internal[BLOCK_SIZE];
    internal[0] = 0;
    a_0 ^= kuzn_multiply_in_F(internal[0], kuzn_L_vect[0]);
    for (i = 1; i < 16; i++)
    {
        internal[i] = state[i - 1]; // Двигаем все на старые места
        a_0 ^= kuzn_multiply_in_F(internal[i], kuzn_L_vect[i]);
    }
    internal[0] = a_0;
    memcpy(state, internal, BLOCK_SIZE);
}

void kuzn_linear_L(const uint8_t *in_data, uint8_t *out_data)
{
    int i;
    uint8_t internal[BLOCK_SIZE];
    memcpy(internal, in_data, BLOCK_SIZE);
    for (i = 0; i < 16; i++)
        kuzn_lshift_R(internal);
    memcpy(out_data, internal, BLOCK_SIZE);
}

void kuzn_inv_linear_L(const uint8_t *in_data, uint8_t *out_data)
{
    int i;
    uint8_t internal[BLOCK_SIZE];
    memcpy(internal, in_data, BLOCK_SIZE);
    for (i = 0; i < 16; i++)
        kuzn_inv_lshift_R(internal);
    memcpy(out_data, internal, BLOCK_SIZE);
}

void kuzn_get_next_keys(const uint8_t *in_key_1, const uint8_t *in_key_2,
                        uint8_t *out_key_1, uint8_t *out_key_2,
                        uint8_t *iter_const) // в стандарте обозначается буквой F, не путать с умножением в поле F!
{
    uint8_t internal[BLOCK_SIZE];
    memcpy(out_key_2, in_key_1, BLOCK_SIZE);
    kuzn_xor_arrays(in_key_1, iter_const, internal);
    kuzn_unlinear_S(internal, internal);
    kuzn_linear_L(internal, internal);
    kuzn_xor_arrays(internal, in_key_2, out_key_1);
}

void kuznechik_expand_keys(const uint8_t *key, uint8_t (*iter_keys)[16])
{
    uint8_t iter_C[32][BLOCK_SIZE];
    uint8_t iter_key[10][BLOCK_SIZE];

    for (int i = 0; i < 32; i++) {
        kuzn_hexstr_to_array(kuzn_iter_C_hex[i], iter_C[i], BLOCK_SIZE);
    }
    // Предыдущая пара ключей
    uint8_t iter_1[BLOCK_SIZE];
    uint8_t iter_2[BLOCK_SIZE];

    // Текущая пара ключей
    uint8_t iter_3[BLOCK_SIZE];
    uint8_t iter_4[BLOCK_SIZE];

    // Первые два итерационных ключа равны половинкам мастер-ключа
    memcpy(iter_key[1], key, BLOCK_SIZE);  // именно в таком порядке, так как половинки отсчитываются с начала записи master_key
    memcpy(iter_key[0], key + BLOCK_SIZE, BLOCK_SIZE);  // (а запись в российских Гостах производится со старшего бита)
    memcpy(iter_1, iter_key[0], BLOCK_SIZE);
    memcpy(iter_2, iter_key[1], BLOCK_SIZE);

    for (int i = 0; i < 4; i++) {  // делаем по формуле (11) со страницы 8 документации
        kuzn_get_next_keys(iter_1, iter_2, iter_3, iter_4, iter_C[0 + 8 * i]);
        kuzn_get_next_keys(iter_3, iter_4, iter_1, iter_2, iter_C[1 + 8 * i]);
        kuzn_get_next_keys(iter_1, iter_2, iter_3, iter_4, iter_C[2 + 8 * i]);
        kuzn_get_next_keys(iter_3, iter_4, iter_1, iter_2, iter_C[3 + 8 * i]);
        kuzn_get_next_keys(iter_1, iter_2, iter_3, iter_4, iter_C[4 + 8 * i]);
        kuzn_get_next_keys(iter_3, iter_4, iter_1, iter_2, iter_C[5 + 8 * i]);
        kuzn_get_next_keys(iter_1, iter_2, iter_3, iter_4, iter_C[6 + 8 * i]);
        kuzn_get_next_keys(iter_3, iter_4, iter_1, iter_2, iter_C[7 + 8 * i]);
        memcpy(iter_key[2 * i + 2], iter_1, BLOCK_SIZE);
        memcpy(iter_key[2 * i + 3], iter_2, BLOCK_SIZE);
    }
    memcpy(iter_keys, iter_key, 10 * BLOCK_SIZE);
}

void kuznechik_encrypt(const uint8_t *blk, uint8_t *key, uint8_t *out_blk)
{
    int i;
    memcpy(out_blk, blk, BLOCK_SIZE);

    uint8_t iter_key[10][BLOCK_SIZE];

    kuznechik_expand_keys(key, iter_key);

    for(i = 0; i < 9; i++)
    {
        kuzn_xor_arrays(iter_key[i], out_blk, out_blk);
        kuzn_unlinear_S(out_blk, out_blk);
        kuzn_linear_L(out_blk, out_blk);
    }
    kuzn_xor_arrays(out_blk, iter_key[9], out_blk);
}

void kuznechik_decrypt(const uint8_t *blk,  uint8_t *key, uint8_t *out_blk)
{
    int i;
    memcpy(out_blk, blk, BLOCK_SIZE);

    uint8_t iter_key[10][BLOCK_SIZE];

    kuznechik_expand_keys(key, iter_key);

    kuzn_xor_arrays(out_blk, iter_key[9], out_blk);
    for(i = 8; i >= 0; i--)
    {
        kuzn_inv_linear_L(out_blk, out_blk);
        kuzn_inv_unlinear_S(out_blk, out_blk);
        kuzn_xor_arrays(iter_key[i], out_blk, out_blk);
    }
}


void kuzn_hexstr_to_array(const char *hexstring, uint8_t *array, size_t bytes) {
    const char *pos = hexstring;
    for (size_t i = 0; i < bytes; i++) {
        sscanf(pos, "%2hhx", &array[bytes - i - 1]);
        pos += 2;
    }
}

void kuzn_array_to_hexstr(const uint8_t *array, size_t bytes, char *hexstring) {
    for (size_t i = 0; i < bytes; i++) {
        sprintf(hexstring + 2 * i, "%02x", array[bytes - 1 - i]);
    }
    hexstring[2 * bytes] = '\0';
}

} // namespace RiscvISA
} // namespace gem5
