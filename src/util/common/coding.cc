//  Copyright (c) 2014, Facebook, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.
//
#include "coding.h"

#include <algorithm>

#include "include/Slice.h"

namespace rocketspeed {

char* EncodeVarint32(char* dst, uint32_t v) {
  // Operate on characters as unsigneds
  unsigned char* ptr = reinterpret_cast<unsigned char*>(dst);
  static const int B = 128;
  if (v < (1 << 7)) {
    *(ptr++) = static_cast<char>(v);
  } else if (v < (1 << 14)) {
    *(ptr++) = static_cast<char>(v | B);
    *(ptr++) = static_cast<char>(v >> 7);
  } else if (v < (1 << 21)) {
    *(ptr++) = static_cast<char>(v | B);
    *(ptr++) = static_cast<char>((v >> 7) | B);
    *(ptr++) = static_cast<char>(v >> 14);
  } else if (v < (1 << 28)) {
    *(ptr++) = static_cast<char>(v | B);
    *(ptr++) = static_cast<char>((v >> 7) | B);
    *(ptr++) = static_cast<char>((v >> 14) | B);
    *(ptr++) = static_cast<char>(v >> 21);
  } else {
    *(ptr++) = static_cast<char>(v | B);
    *(ptr++) = static_cast<char>((v >> 7) | B);
    *(ptr++) = static_cast<char>((v >> 14) | B);
    *(ptr++) = static_cast<char>((v >> 21) | B);
    *(ptr++) = static_cast<char>(v >> 28);
  }
  return reinterpret_cast<char*>(ptr);
}

const char* GetVarint32PtrFallback(const char* p, const char* limit,
                                   uint32_t* value) {
  uint32_t result = 0;
  for (uint32_t shift = 0; shift <= 28 && p < limit; shift += 7) {
    uint32_t byte = *(reinterpret_cast<const unsigned char*>(p));
    p++;
    if (byte & 128) {
      // More bytes are present
      result |= ((byte & 127) << shift);
    } else {
      result |= (byte << shift);
      *value = result;
      return reinterpret_cast<const char*>(p);
    }
  }
  return nullptr;
}

const char* GetVarint64Ptr(const char* p, const char* limit, uint64_t* value) {
  uint64_t result = 0;
  for (uint32_t shift = 0; shift <= 63 && p < limit; shift += 7) {
    uint64_t byte = *(reinterpret_cast<const unsigned char*>(p));
    p++;
    if (byte & 128) {
      // More bytes are present
      result |= ((byte & 127) << shift);
    } else {
      result |= (byte << shift);
      *value = result;
      return reinterpret_cast<const char*>(p);
    }
  }
  return nullptr;
}

void BitStreamPutInt(char* dst, size_t dstlen, size_t offset,
                     uint32_t bits, uint64_t value) {
  RS_ASSERT((offset + bits + 7)/8 <= dstlen);
  RS_ASSERT(bits <= 64);

  unsigned char* ptr = reinterpret_cast<unsigned char*>(dst);

  size_t byteOffset = offset / 8;
  size_t bitOffset = offset % 8;

  // This prevents unused variable warnings when compiling.
#ifndef NO_RS_ASSERT
  // Store truncated value.
  uint64_t origValue = (bits < 64)?(value & (((uint64_t)1 << bits) - 1)):value;
  uint32_t origBits = bits;
#endif

  while (bits > 0) {
    size_t bitsToGet = std::min<size_t>(bits, 8 - bitOffset);
    unsigned char mask = static_cast<unsigned char>((1 << bitsToGet) - 1);

    ptr[byteOffset] =
      static_cast<unsigned char>((ptr[byteOffset] & ~(mask << bitOffset)) +
                                 ((value & mask) << bitOffset));

    value >>= bitsToGet;
    byteOffset += 1;
    bitOffset = 0;
    bits -= static_cast<uint32_t>(bitsToGet);
  }

#ifndef NO_RS_ASSERT
  RS_ASSERT(origValue == BitStreamGetInt(dst, dstlen, offset, origBits));
#endif
}

uint64_t BitStreamGetInt(const char* src, size_t srclen, size_t offset,
                         uint32_t bits) {
  RS_ASSERT((offset + bits + 7)/8 <= srclen);
  RS_ASSERT(bits <= 64);

  const unsigned char* ptr = reinterpret_cast<const unsigned char*>(src);

  uint64_t result = 0;

  size_t byteOffset = offset / 8;
  size_t bitOffset = offset % 8;
  size_t shift = 0;

  while (bits > 0) {
    size_t bitsToGet = std::min<size_t>(bits, 8 - bitOffset);
    unsigned char mask = static_cast<unsigned char>((1 << bitsToGet) - 1);

    result += (uint64_t)((ptr[byteOffset] >> bitOffset) & mask) << shift;

    shift += bitsToGet;
    byteOffset += 1;
    bitOffset = 0;
    bits -= static_cast<uint32_t>(bitsToGet);
  }

  return result;
}

void BitStreamPutInt(std::string* dst, size_t offset, uint32_t bits,
                     uint64_t value) {
  RS_ASSERT((offset + bits + 7)/8 <= dst->size());

  const size_t kTmpBufLen = sizeof(value) + 1;
  char tmpBuf[kTmpBufLen];

  // Number of bytes of tmpBuf being used
  const size_t kUsedBytes = (offset%8 + bits)/8;

  // Copy relevant parts of dst to tmpBuf
  for (size_t idx = 0; idx <= kUsedBytes; ++idx) {
    tmpBuf[idx] = (*dst)[offset/8 + idx];
  }

  BitStreamPutInt(tmpBuf, kTmpBufLen, offset%8, bits, value);

  // Copy tmpBuf back to dst
  for (size_t idx = 0; idx <= kUsedBytes; ++idx) {
    (*dst)[offset/8 + idx] = tmpBuf[idx];
  }

  // Do the check here too as we are working with a buffer.
  RS_ASSERT(((bits < 64)?(value & (((uint64_t)1 << bits) - 1)):value) ==
         BitStreamGetInt(dst, offset, bits));
}

}  // namespace rocketspeed
