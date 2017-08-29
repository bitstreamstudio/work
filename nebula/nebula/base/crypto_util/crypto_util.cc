/*
 *  Copyright (c) 2016, https://github.com/nebula-im/nebula
 *  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "nebula/base/crypto_util/crypto_util.h"

#include <stdlib.h>
#include <algorithm>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <openssl/bn.h>
#include <openssl/pem.h>
#include <openssl/aes.h>

#include <openssl/rsa.h>
#include <openssl/err.h>
#include <openssl/bio.h>

#include <memory.h>

#include <stdexcept>
#include <glog/logging.h>
#include <folly/Format.h>

// bytes
// |  0-3  |  4-7   |    8-55    |     56-59    | 60-63 |
// |  val  |  val2  |            | 0xefefefefef |       |
//
// temp
// |    0 ~ 47       |
// | 55 ~ 8 (bytes)  |
//
// encrypt_key_: 8  ~ 39 (bytes)
// encrypt_iv_ : 40 ~ 55 (bytes)
// decrypt_key_: 0  ~ 31 (temp)
// decrypt_iv_ : 32 ~ 47 (temp)
//
AuthKeyGenerator::AuthKeyGenerator() {
  // 生成nonce
  do {
    RAND_bytes(nonce_.data(), 64);
    uint32_t val = (nonce_[3] << 24) | (nonce_[2] << 16) | (nonce_[1] << 8) | (nonce_[0]);
    uint32_t val2 = (nonce_[7] << 24) | (nonce_[6] << 16) | (nonce_[5] << 8) | (nonce_[4]);
    if (nonce_[0] != 0xef  &&
        val  != 0x44414548 &&
        val  != 0x54534f50 &&
        val  != 0x20544547 &&
        val  != 0x4954504f &&
        val  != 0xeeeeeeee &&
        val2 != 0x00000000) {
      nonce_[56] = nonce_[57] = nonce_[58] = nonce_[59] = 0xef;
      break;
    }
  } while(1);
  
  // 生成decrypt_key
  for (int i = 0; i < 48; ++i) {
    nonce_[64+i] = nonce_[55 - i];
  }
}

AuthKeyGenerator::AuthKeyGenerator(folly::ByteRange key) {
  if (key.size() != 64) {
    // TODO(@wubenqi): 打印日志
    throw std::out_of_range("underflow");
  }
  
  // 检查key有效性
  uint32_t val = (key[3] << 24) | (key[2] << 16) | (key[1] << 8) | (key[0]);
  uint32_t val2 = (key[7] << 24) | (key[6] << 16) | (key[5] << 8) | (key[4]);
  if (key[0]  == 0xef    ||
      val  == 0x44414548 ||
      val  == 0x54534f50 ||
      val  == 0x20544547 ||
      val  == 0x4954504f ||
      val  == 0xeeeeeeee ||
      val2 == 0x00000000 ||
      key[56]  != 0xef   ||
      key[57]  != 0xef   ||
      key[58]  != 0xef   ||
      key[59]  != 0xef) {
    LOG(ERROR) << "AuthKeyGenerator - throw exception: invalid key: " << ToHexStr(key);
    throw std::runtime_error("invalid key");
  }
  
  // 生成nonce
  memcpy(nonce_.data(), key.data(), key.size());
  
  // 生成decrypt_key
  for (int i = 0; i < 48; ++i) {
    nonce_[64+i] = nonce_[55 - i];
  }
}

AesCtrEncrypt::AesCtrEncrypt(folly::ByteRange key, folly::ByteRange iv) {
  // 检测key和iv的长度
  if (UNLIKELY(key.size() != 32 ||
               iv.size() != 16)) {
    throw std::out_of_range("underflow");
  }

  AES_set_encrypt_key(key.data(), 256, &aes_key_);
  memcpy(aes_ctr_state_.ivec, iv.data(), 16);
}

uint32_t AesCtrEncrypt::Encrypt(const uint8_t* in, uint8_t* out, uint32_t len) {
  AES_ctr128_encrypt(in, out, len, &aes_key_, aes_ctr_state_.ivec, aes_ctr_state_.ecount, &aes_ctr_state_.num);
  return len;
}

uint32_t AesCtrEncrypt::Encrypt(folly::IOBuf* io_buf, uint32_t len) {
  uint64_t new_length = 0;

  if (io_buf->isChained()) {
    new_length = len > io_buf->length() ? io_buf->length() : len;
    Encrypt(io_buf->data(), io_buf->writableData(), static_cast<uint32_t>(new_length));
  } else {
    
    folly::IOBuf* end = io_buf;
    do {
      if (len-new_length > end->length()) {
        Encrypt(end->data(), end->writableData(), static_cast<uint32_t>(end->length()));
        new_length += end->length();
        end = end->next();
      } else {
        Encrypt(end->data(), end->writableData(), static_cast<uint32_t>(len-new_length));
        new_length = len;
        break;
      }
    } while (end != io_buf);
  }
  
  return static_cast<uint32_t>(new_length);
}

void AesCtrEncrypt::Encrypt(folly::IOBuf* io_buf) {
  if (io_buf->isChained()) {
    Encrypt(io_buf->data(), io_buf->writableData(), static_cast<uint32_t>(io_buf->length()));
  } else {
    
    folly::IOBuf* end = io_buf;
    do {
        Encrypt(end->data(), end->writableData(), static_cast<uint32_t>(end->length()));
        end = end->next();
    } while (end != io_buf);
  }
}

std::string ToHexStr(folly::ByteRange sp) {
  std::string out;
  for (auto ch : sp) {
    out.append(folly::sformat("{:02x}", (uint8_t) ch));
  }
  return out;
}
