/*===---- avx2intrin.h - AVX2 intrinsics -----------------------------------===
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 *===-----------------------------------------------------------------------===
 */

#ifndef __IMMINTRIN_H
#error "Never use <avx2intrin.h> directly; include <immintrin.h> instead."
#endif

/* SSE4 Multiple Packed Sums of Absolute Difference.  */
#define _mm256_mpsadbw_epu8(X, Y, M) __builtin_ia32_mpsadbw256((X), (Y), (M))

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_abs_epi8(__m256i a)
{
    return (__m256i)__builtin_ia32_pabsb256((__v32qi)a);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_abs_epi16(__m256i a)
{
    return (__m256i)__builtin_ia32_pabsw256((__v16hi)a);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_abs_epi32(__m256i a)
{
    return (__m256i)__builtin_ia32_pabsd256((__v8si)a);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_packs_epi16(__m256i a, __m256i b)
{
  return (__m256i)__builtin_ia32_packsswb256((__v16hi)a, (__v16hi)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_packs_epi32(__m256i a, __m256i b)
{
  return (__m256i)__builtin_ia32_packssdw256((__v8si)a, (__v8si)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_packus_epi16(__m256i a, __m256i b)
{
  return (__m256i)__builtin_ia32_packuswb256((__v16hi)a, (__v16hi)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_packus_epi32(__m256i __V1, __m256i __V2)
{
  return (__m256i) __builtin_ia32_packusdw256((__v8si)__V1, (__v8si)__V2);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_add_epi8(__m256i a, __m256i b)
{
  return (__m256i)((__v32qi)a + (__v32qi)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_add_epi16(__m256i a, __m256i b)
{
  return (__m256i)((__v16hi)a + (__v16hi)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_add_epi32(__m256i a, __m256i b)
{
  return (__m256i)((__v8si)a + (__v8si)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_add_epi64(__m256i a, __m256i b)
{
  return a + b;
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_adds_epi8(__m256i a, __m256i b)
{
  return (__m256i)__builtin_ia32_paddsb256((__v32qi)a, (__v32qi)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_adds_epi16(__m256i a, __m256i b)
{
  return (__m256i)__builtin_ia32_paddsw256((__v16hi)a, (__v16hi)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_adds_epu8(__m256i a, __m256i b)
{
  return (__m256i)__builtin_ia32_paddusb256((__v32qi)a, (__v32qi)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_adds_epu16(__m256i a, __m256i b)
{
  return (__m256i)__builtin_ia32_paddusw256((__v16hi)a, (__v16hi)b);
}

#define _mm256_alignr_epi8(a, b, n) __extension__ ({ \
  __m256i __a = (a); \
  __m256i __b = (b); \
  (__m256i)__builtin_ia32_palignr256((__v32qi)__a, (__v32qi)__b, (n)); })

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_and_si256(__m256i a, __m256i b)
{
  return a & b;
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_andnot_si256(__m256i a, __m256i b)
{
  return ~a & b;
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_avg_epu8(__m256i a, __m256i b)
{
  return (__m256i)__builtin_ia32_pavgb256((__v32qi)a, (__v32qi)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_avg_epu16(__m256i a, __m256i b)
{
  return (__m256i)__builtin_ia32_pavgw256((__v16hi)a, (__v16hi)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_blendv_epi8(__m256i __V1, __m256i __V2, __m256i __M)
{
  return (__m256i)__builtin_ia32_pblendvb256((__v32qi)__V1, (__v32qi)__V2,
                                              (__v32qi)__M);
}

#define _mm256_blend_epi16(V1, V2, M) __extension__ ({ \
  __m256i __V1 = (V1); \
  __m256i __V2 = (V2); \
  (__m256i)__builtin_ia32_pblendw256((__v16hi)__V1, (__v16hi)__V2, M); })

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_cmpeq_epi8(__m256i a, __m256i b)
{
  return (__m256i)((__v32qi)a == (__v32qi)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_cmpeq_epi16(__m256i a, __m256i b)
{
  return (__m256i)((__v16hi)a == (__v16hi)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_cmpeq_epi32(__m256i a, __m256i b)
{
  return (__m256i)((__v8si)a == (__v8si)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_cmpeq_epi64(__m256i a, __m256i b)
{
  return (__m256i)((__v4di)a == (__v4di)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_cmpgt_epi8(__m256i a, __m256i b)
{
  return (__m256i)((__v32qi)a > (__v32qi)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_cmpgt_epi16(__m256i a, __m256i b)
{
  return (__m256i)((__v16hi)a > (__v16hi)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_cmpgt_epi32(__m256i a, __m256i b)
{
  return (__m256i)((__v8si)a > (__v8si)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_cmpgt_epi64(__m256i a, __m256i b)
{
  return (__m256i)((__v4di)a > (__v4di)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_hadd_epi16(__m256i a, __m256i b)
{
    return (__m256i)__builtin_ia32_phaddw256((__v16hi)a, (__v16hi)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_hadd_epi32(__m256i a, __m256i b)
{
    return (__m256i)__builtin_ia32_phaddd256((__v8si)a, (__v8si)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_hadds_epi16(__m256i a, __m256i b)
{
    return (__m256i)__builtin_ia32_phaddsw256((__v16hi)a, (__v16hi)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_hsub_epi16(__m256i a, __m256i b)
{
    return (__m256i)__builtin_ia32_phsubw256((__v16hi)a, (__v16hi)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_hsub_epi32(__m256i a, __m256i b)
{
    return (__m256i)__builtin_ia32_phsubd256((__v8si)a, (__v8si)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_hsubs_epi16(__m256i a, __m256i b)
{
    return (__m256i)__builtin_ia32_phsubsw256((__v16hi)a, (__v16hi)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_maddubs_epi16(__m256i a, __m256i b)
{
    return (__m256i)__builtin_ia32_pmaddubsw256((__v32qi)a, (__v32qi)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_madd_epi16(__m256i a, __m256i b)
{
  return (__m256i)__builtin_ia32_pmaddwd256((__v16hi)a, (__v16hi)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_max_epi8(__m256i a, __m256i b)
{
  return (__m256i)__builtin_ia32_pmaxsb256((__v32qi)a, (__v32qi)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_max_epi16(__m256i a, __m256i b)
{
  return (__m256i)__builtin_ia32_pmaxsw256((__v16hi)a, (__v16hi)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_max_epi32(__m256i a, __m256i b)
{
  return (__m256i)__builtin_ia32_pmaxsd256((__v8si)a, (__v8si)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_max_epu8(__m256i a, __m256i b)
{
  return (__m256i)__builtin_ia32_pmaxub256((__v32qi)a, (__v32qi)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_max_epu16(__m256i a, __m256i b)
{
  return (__m256i)__builtin_ia32_pmaxuw256((__v16hi)a, (__v16hi)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_max_epu32(__m256i a, __m256i b)
{
  return (__m256i)__builtin_ia32_pmaxud256((__v8si)a, (__v8si)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_min_epi8(__m256i a, __m256i b)
{
  return (__m256i)__builtin_ia32_pminsb256((__v32qi)a, (__v32qi)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_min_epi16(__m256i a, __m256i b)
{
  return (__m256i)__builtin_ia32_pminsw256((__v16hi)a, (__v16hi)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_min_epi32(__m256i a, __m256i b)
{
  return (__m256i)__builtin_ia32_pminsd256((__v8si)a, (__v8si)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_min_epu8(__m256i a, __m256i b)
{
  return (__m256i)__builtin_ia32_pminub256((__v32qi)a, (__v32qi)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_min_epu16(__m256i a, __m256i b)
{
  return (__m256i)__builtin_ia32_pminuw256 ((__v16hi)a, (__v16hi)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_min_epu32(__m256i a, __m256i b)
{
  return (__m256i)__builtin_ia32_pminud256((__v8si)a, (__v8si)b);
}

static __inline__ int __attribute__((__always_inline__, __nodebug__))
_mm256_movemask_epi8(__m256i a)
{
  return __builtin_ia32_pmovmskb256((__v32qi)a);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_cvtepi8_epi16(__m128i __V)
{
  return (__m256i)__builtin_ia32_pmovsxbw256((__v16qi)__V);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_cvtepi8_epi32(__m128i __V)
{
  return (__m256i)__builtin_ia32_pmovsxbd256((__v16qi)__V);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_cvtepi8_epi64(__m128i __V)
{
  return (__m256i)__builtin_ia32_pmovsxbq256((__v16qi)__V);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_cvtepi16_epi32(__m128i __V)
{
  return (__m256i)__builtin_ia32_pmovsxwd256((__v8hi)__V);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_cvtepi16_epi64(__m128i __V)
{
  return (__m256i)__builtin_ia32_pmovsxwq256((__v8hi)__V);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_cvtepi32_epi64(__m128i __V)
{
  return (__m256i)__builtin_ia32_pmovsxdq256((__v4si)__V);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_cvtepu8_epi16(__m128i __V)
{
  return (__m256i)__builtin_ia32_pmovzxbw256((__v16qi)__V);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_cvtepu8_epi32(__m128i __V)
{
  return (__m256i)__builtin_ia32_pmovzxbd256((__v16qi)__V);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_cvtepu8_epi64(__m128i __V)
{
  return (__m256i)__builtin_ia32_pmovzxbq256((__v16qi)__V);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_cvtepu16_epi32(__m128i __V)
{
  return (__m256i)__builtin_ia32_pmovzxwd256((__v8hi)__V);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_cvtepu16_epi64(__m128i __V)
{
  return (__m256i)__builtin_ia32_pmovzxwq256((__v8hi)__V);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_cvtepu32_epi64(__m128i __V)
{
  return (__m256i)__builtin_ia32_pmovzxdq256((__v4si)__V);
}

static __inline__  __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_mul_epi32(__m256i a, __m256i b)
{
  return (__m256i)__builtin_ia32_pmuldq256((__v8si)a, (__v8si)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_mulhrs_epi16(__m256i a, __m256i b)
{
  return (__m256i)__builtin_ia32_pmulhrsw256((__v16hi)a, (__v16hi)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_mulhi_epu16(__m256i a, __m256i b)
{
  return (__m256i)__builtin_ia32_pmulhuw256((__v16hi)a, (__v16hi)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_mulhi_epi16(__m256i a, __m256i b)
{
  return (__m256i)__builtin_ia32_pmulhw256((__v16hi)a, (__v16hi)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_mullo_epi16(__m256i a, __m256i b)
{
  return (__m256i)((__v16hi)a * (__v16hi)b);
}

static __inline__  __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_mullo_epi32 (__m256i a, __m256i b)
{
  return (__m256i)((__v8si)a * (__v8si)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_mul_epu32(__m256i a, __m256i b)
{
  return __builtin_ia32_pmuludq256((__v8si)a, (__v8si)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_or_si256(__m256i a, __m256i b)
{
  return a | b;
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_sad_epu8(__m256i a, __m256i b)
{
  return __builtin_ia32_psadbw256((__v32qi)a, (__v32qi)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_shuffle_epi8(__m256i a, __m256i b)
{
  return (__m256i)__builtin_ia32_pshufb256((__v32qi)a, (__v32qi)b);
}

#define _mm256_shuffle_epi32(a, imm) __extension__ ({ \
  __m256i __a = (a); \
  (__m256i)__builtin_shufflevector((__v8si)__a, (__v8si)_mm256_set1_epi32(0), \
                                   (imm) & 0x3, ((imm) & 0xc) >> 2, \
                                   ((imm) & 0x30) >> 4, ((imm) & 0xc0) >> 6, \
                                   4 + (((imm) & 0x03) >> 0), \
                                   4 + (((imm) & 0x0c) >> 2), \
                                   4 + (((imm) & 0x30) >> 4), \
                                   4 + (((imm) & 0xc0) >> 6)); })

#define _mm256_shufflehi_epi16(a, imm) __extension__ ({ \
  __m256i __a = (a); \
  (__m256i)__builtin_shufflevector((__v16hi)__a, (__v16hi)_mm256_set1_epi16(0), \
                                   0, 1, 2, 3, \
                                   4 + (((imm) & 0x03) >> 0), \
                                   4 + (((imm) & 0x0c) >> 2), \
                                   4 + (((imm) & 0x30) >> 4), \
                                   4 + (((imm) & 0xc0) >> 6), \
                                   8, 9, 10, 11, \
                                   12 + (((imm) & 0x03) >> 0), \
                                   12 + (((imm) & 0x0c) >> 2), \
                                   12 + (((imm) & 0x30) >> 4), \
                                   12 + (((imm) & 0xc0) >> 6)); })

#define _mm256_shufflelo_epi16(a, imm) __extension__ ({ \
  __m256i __a = (a); \
  (__m256i)__builtin_shufflevector((__v16hi)__a, (__v16hi)_mm256_set1_epi16(0), \
                                   (imm) & 0x3,((imm) & 0xc) >> 2, \
                                   ((imm) & 0x30) >> 4, ((imm) & 0xc0) >> 6, \
                                   4, 5, 6, 7, \
                                   8 + (((imm) & 0x03) >> 0), \
                                   8 + (((imm) & 0x0c) >> 2), \
                                   8 + (((imm) & 0x30) >> 4), \
                                   8 + (((imm) & 0xc0) >> 6), \
                                   12, 13, 14, 15); })

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_sign_epi8(__m256i a, __m256i b)
{
    return (__m256i)__builtin_ia32_psignb256((__v32qi)a, (__v32qi)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_sign_epi16(__m256i a, __m256i b)
{
    return (__m256i)__builtin_ia32_psignw256((__v16hi)a, (__v16hi)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_sign_epi32(__m256i a, __m256i b)
{
    return (__m256i)__builtin_ia32_psignd256((__v8si)a, (__v8si)b);
}

#define _mm256_slli_si256(a, count) __extension__ ({ \
  __m256i __a = (a); \
  (__m256i)__builtin_ia32_pslldqi256(__a, (count)*8); })

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_slli_epi16(__m256i a, int count)
{
  return (__m256i)__builtin_ia32_psllwi256((__v16hi)a, count);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_sll_epi16(__m256i a, __m128i count)
{
  return (__m256i)__builtin_ia32_psllw256((__v16hi)a, (__v8hi)count);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_slli_epi32(__m256i a, int count)
{
  return (__m256i)__builtin_ia32_pslldi256((__v8si)a, count);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_sll_epi32(__m256i a, __m128i count)
{
  return (__m256i)__builtin_ia32_pslld256((__v8si)a, (__v4si)count);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_slli_epi64(__m256i a, int count)
{
  return __builtin_ia32_psllqi256(a, count);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_sll_epi64(__m256i a, __m128i count)
{
  return __builtin_ia32_psllq256(a, count);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_srai_epi16(__m256i a, int count)
{
  return (__m256i)__builtin_ia32_psrawi256((__v16hi)a, count);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_sra_epi16(__m256i a, __m128i count)
{
  return (__m256i)__builtin_ia32_psraw256((__v16hi)a, (__v8hi)count);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_srai_epi32(__m256i a, int count)
{
  return (__m256i)__builtin_ia32_psradi256((__v8si)a, count);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_sra_epi32(__m256i a, __m128i count)
{
  return (__m256i)__builtin_ia32_psrad256((__v8si)a, (__v4si)count);
}

#define _mm256_srli_si256(a, count) __extension__ ({ \
  __m256i __a = (a); \
  (__m256i)__builtin_ia32_psrldqi256(__a, (count)*8); })

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_srli_epi16(__m256i a, int count)
{
  return (__m256i)__builtin_ia32_psrlwi256((__v16hi)a, count);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_srl_epi16(__m256i a, __m128i count)
{
  return (__m256i)__builtin_ia32_psrlw256((__v16hi)a, (__v8hi)count);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_srli_epi32(__m256i a, int count)
{
  return (__m256i)__builtin_ia32_psrldi256((__v8si)a, count);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_srl_epi32(__m256i a, __m128i count)
{
  return (__m256i)__builtin_ia32_psrld256((__v8si)a, (__v4si)count);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_srli_epi64(__m256i a, int count)
{
  return __builtin_ia32_psrlqi256(a, count);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_srl_epi64(__m256i a, __m128i count)
{
  return __builtin_ia32_psrlq256(a, count);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_sub_epi8(__m256i a, __m256i b)
{
  return (__m256i)((__v32qi)a - (__v32qi)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_sub_epi16(__m256i a, __m256i b)
{
  return (__m256i)((__v16hi)a - (__v16hi)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_sub_epi32(__m256i a, __m256i b)
{
  return (__m256i)((__v8si)a - (__v8si)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_sub_epi64(__m256i a, __m256i b)
{
  return a - b;
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_subs_epi8(__m256i a, __m256i b)
{
  return (__m256i)__builtin_ia32_psubsb256((__v32qi)a, (__v32qi)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_subs_epi16(__m256i a, __m256i b)
{
  return (__m256i)__builtin_ia32_psubsw256((__v16hi)a, (__v16hi)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_subs_epu8(__m256i a, __m256i b)
{
  return (__m256i)__builtin_ia32_psubusb256((__v32qi)a, (__v32qi)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_subs_epu16(__m256i a, __m256i b)
{
  return (__m256i)__builtin_ia32_psubusw256((__v16hi)a, (__v16hi)b);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_unpackhi_epi8(__m256i a, __m256i b)
{
  return (__m256i)__builtin_shufflevector((__v32qi)a, (__v32qi)b, 8, 32+8, 9, 32+9, 10, 32+10, 11, 32+11, 12, 32+12, 13, 32+13, 14, 32+14, 15, 32+15, 24, 32+24, 25, 32+25, 26, 32+26, 27, 32+27, 28, 32+28, 29, 32+29, 30, 32+30, 31, 32+31);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_unpackhi_epi16(__m256i a, __m256i b)
{
  return (__m256i)__builtin_shufflevector((__v16hi)a, (__v16hi)b, 4, 16+4, 5, 16+5, 6, 16+6, 7, 16+7, 12, 16+12, 13, 16+13, 14, 16+14, 15, 16+15);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_unpackhi_epi32(__m256i a, __m256i b)
{
  return (__m256i)__builtin_shufflevector((__v8si)a, (__v8si)b, 2, 8+2, 3, 8+3, 6, 8+6, 7, 8+7);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_unpackhi_epi64(__m256i a, __m256i b)
{
  return (__m256i)__builtin_shufflevector(a, b, 1, 4+1, 3, 4+3);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_unpacklo_epi8(__m256i a, __m256i b)
{
  return (__m256i)__builtin_shufflevector((__v32qi)a, (__v32qi)b, 0, 32+0, 1, 32+1, 2, 32+2, 3, 32+3, 4, 32+4, 5, 32+5, 6, 32+6, 7, 32+7, 16, 32+16, 17, 32+17, 18, 32+18, 19, 32+19, 20, 32+20, 21, 32+21, 22, 32+22, 23, 32+23);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_unpacklo_epi16(__m256i a, __m256i b)
{
  return (__m256i)__builtin_shufflevector((__v16hi)a, (__v16hi)b, 0, 16+0, 1, 16+1, 2, 16+2, 3, 16+3, 8, 16+8, 9, 16+9, 10, 16+10, 11, 16+11);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_unpacklo_epi32(__m256i a, __m256i b)
{
  return (__m256i)__builtin_shufflevector((__v8si)a, (__v8si)b, 0, 8+0, 1, 8+1, 4, 8+4, 5, 8+5);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_unpacklo_epi64(__m256i a, __m256i b)
{
  return (__m256i)__builtin_shufflevector(a, b, 0, 4+0, 2, 4+2);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_xor_si256(__m256i a, __m256i b)
{
  return a ^ b;
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm256_stream_load_si256(__m256i *__V)
{
  return (__m256i)__builtin_ia32_movntdqa256((__v4di *)__V);
}

static __inline__ __m128 __attribute__((__always_inline__, __nodebug__))
_mm_broadcastss_ps(__m128 __X)
{
  return (__m128)__builtin_ia32_vbroadcastss_ps((__v4sf)__X);
}

static __inline__ __m256 __attribute__((__always_inline__, __nodebug__))
_mm256_broadcastss_ps(__m128 __X)
{
  return (__m256)__builtin_ia32_vbroadcastss_ps256((__v4sf)__X);
}

static __inline__ __m256d __attribute__((__always_inline__, __nodebug__))
_mm256_broadcastsd_pd(__m128d __X)
{
  return (__m256d)__builtin_ia32_vbroadcastsd_pd256((__v2df)__X);
}

static __inline__ __m256i __attribute__((__always_inline__, __nodebug__))
_mm_broadcastsi128_si256(__m128i const *a)
{
  return (__m256i)__builtin_ia32_vbroadcastsi256(a);
}

#define _mm_blend_epi32(V1, V2, M) __extension__ ({ \
  __m128i __V1 = (V1); \
  __m128i __V2 = (V2); \
  (__m128i)__builtin_ia32_pblendd128((__v4si)__V1, (__v4si)__V2, M); })

#define _mm256_blend_epi32(V1, V2, M) __extension__ ({ \
  __m256i __V1 = (V1); \
  __m256i __V2 = (V2); \
  (__m256i)__builtin_ia32_pblendd256((__v8si)__V1, (__v8si)__V2, M); })
