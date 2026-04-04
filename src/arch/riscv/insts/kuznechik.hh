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

#ifndef __ARCH_RISCV_INSTS_KUZNECHIK_HH__
#define __ARCH_RISCV_INSTS_KUZNECHIK_HH__

#include <cstdint>

#define BLOCK_SIZE 16  // размер блока Кузнечика равен 128 бит = 16 байт (мы будем хранить блок как массив из 16 байт, байт - это unt8_t)

void unlinear_S(const uint8_t *in_data, uint8_t *out_data);
void inv_unlinear_S(const uint8_t *in_data, uint8_t *out_data);

void lshift_R(const uint8_t *arr128, uint8_t *result);
void inv_lshift_R(const uint8_t *arr128, uint8_t *result);

void linear_L(const uint8_t *in_data, uint8_t *out_data);
void inv_linear_L(const uint8_t *in_data, uint8_t *out_data);

void compute_consts();
void compute_values_L();
void expand_key(const uint8_t *master_key);

uint8_t* kuznechik_encrypt(const uint8_t *plaintext_128, const uint8_t *master_key);
void kuznechik_decrypt(const uint8_t *ciphertext_128, uint8_t *plaintext_128);

void fast_linear_L(const uint8_t *in_data, uint8_t *out_data);
void fast_kuznechik_encrypt(const uint8_t *plaintext_128, uint8_t *ciphertext_128);

uint8_t (*get_ptr_iter_C(void))[BLOCK_SIZE];
uint8_t (*get_ptr_iter_key(void))[BLOCK_SIZE];

#endif // __ARCH_RISCV_INSTS_KUZNECHIK_HH__
