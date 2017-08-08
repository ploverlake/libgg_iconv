/**
 *  Copyright (c) 2017, Russell
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "gg_wchar_ex.h"
#include <errno.h>

#define OOB(c,b) (((((b)>>3)-0x10)|(((b)>>3)+((int32_t)(c)>>26))) & ~7)
#define R(a,b) ((uint32_t)((a==0x80 ? 0x40-b : -a) << 23))

#define SA 0xc2u
#define SB 0xf4u

#define C(x) ( x<2 ? -1 : ( R(0x80,0xc0) | x ) )
#define D(x) C((x+16))
#define E(x) ( ( x==0 ? R(0xa0,0xc0) : \
                 x==0xd ? R(0x80,0xa0) : \
                 R(0x80,0xc0) ) \
             | ( R(0x80,0xc0) >> 6 ) \
             | x )
#define F(x) ( ( x>=5 ? 0 : \
                 x==0 ? R(0x90,0xc0) : \
                 x==4 ? R(0x80,0xa0) : \
                 R(0x80,0xc0) ) \
             | ( R(0x80,0xc0) >> 6 ) \
             | ( R(0x80,0xc0) >> 12 ) \
             | x )

static const uint32_t gg_wchar_ex_bittab[] = {
    C(0x2),C(0x3),C(0x4),C(0x5),C(0x6),C(0x7),
    C(0x8),C(0x9),C(0xa),C(0xb),C(0xc),C(0xd),C(0xe),C(0xf),
    D(0x0),D(0x1),D(0x2),D(0x3),D(0x4),D(0x5),D(0x6),D(0x7),
    D(0x8),D(0x9),D(0xa),D(0xb),D(0xc),D(0xd),D(0xe),D(0xf),
    E(0x0),E(0x1),E(0x2),E(0x3),E(0x4),E(0x5),E(0x6),E(0x7),
    E(0x8),E(0x9),E(0xa),E(0xb),E(0xc),E(0xd),E(0xe),E(0xf),
    F(0x0),F(0x1),F(0x2),F(0x3),F(0x4)
};

int gg_wctomb_utf8(char *s, wchar_t wc)
{
    if (!s) {
        return 0;
    }
    return gg_wcrtomb_utf8(s, wc, NULL);
}

size_t gg_wcrtomb_utf8(char *restrict s, wchar_t wc, mbstate_t *restrict st)
{
    if (!s) return 1;
    if ((unsigned)wc < 0x80) {
        *s = wc;
        return 1;
    } else if ((unsigned)wc < 0x800) {
        *s++ = 0xc0 | (wc>>6);
        *s = 0x80 | (wc&0x3f);
        return 2;
    } else if ((unsigned)wc < 0xd800 || (unsigned)wc-0xe000 < 0x2000) {
        *s++ = 0xe0 | (wc>>12);
        *s++ = 0x80 | ((wc>>6)&0x3f);
        *s = 0x80 | (wc&0x3f);
        return 3;
    } else if ((unsigned)wc-0x10000 < 0x100000) {
        *s++ = 0xf0 | (wc>>18);
        *s++ = 0x80 | ((wc>>12)&0x3f);
        *s++ = 0x80 | ((wc>>6)&0x3f);
        *s = 0x80 | (wc&0x3f);
        return 4;
    }
    errno = EILSEQ;
    return -1;
}

size_t gg_mbrtowc_utf8(wchar_t *restrict wc, const char *restrict src, size_t n, mbstate_t *restrict st)
{
    static unsigned internal_state;
    unsigned c;
    const unsigned char *s = (const void *)src;
    const unsigned N = n;

    if (!st) st = (void *)&internal_state;
    c = *(unsigned *)st;

    if (!s) {
        if (c) goto ilseq;
        return 0;
    } else if (!wc) wc = (void *)&wc;

    if (!n) return -2;
    if (!c) {
        if (*s < 0x80) return !!(*wc = *s);
        if (*s-SA > SB-SA) goto ilseq;
        c = gg_wchar_ex_bittab[*s++-SA]; n--;
    }

    if (n) {
        if (OOB(c,*s)) goto ilseq;
    loop:
        c = c<<6 | *s++-0x80; n--;
        if (!(c&(1U<<31))) {
            *(unsigned *)st = 0;
            *wc = c;
            return N-n;
        }
        if (n) {
            if (*s-0x80u >= 0x40) goto ilseq;
            goto loop;
        }
    }

    *(unsigned *)st = c;
    return -2;
ilseq:
    *(unsigned *)st = 0;
    errno = EILSEQ;
    return -1;
}
