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
#include <stdio.h>
#include <string.h>

namespace gem5
{
namespace RiscvISA
{

// Функция шифрования блока plaintext_128 в блок ciphertext_128 (каждый блок - длины 128 битов)
void kuznechik_encrypt(uint8_t *plaintext_128, uint8_t *master_key, uint8_t *ciphertext_128);

// Функция дешифрования блока
void kuznechik_decrypt(uint8_t *ciphertext_128, uint8_t *master_key, uint8_t *plaintext_128);

} // namespace RiscvISA
} // namespace gem5

#endif // __ARCH_RISCV_INSTS_KUZNECHIK_HH__