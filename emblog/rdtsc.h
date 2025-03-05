// -----------------------------------------------------------------------------
// MIT License
//
// Copyright 2022-Present Miguel A. Guerrero
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal # in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
// -----------------------------------------------------------------------------
#pragma once

#include <stdint.h>

// get x86 internal tick counter (64 bit)
static __inline__ uint64_t rdtsc(void) __attribute__ ((no_instrument_function));

#if defined(__i386__)

static __inline__ uint64_t rdtsc(void)
{
    uint64_t int x;
    __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
    return x;
}

#elif defined(__x86_64__)

static __inline__ uint64_t rdtsc(void)
{
    uint32_t hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)lo) | (((uint64_t)hi) << 32);
}

#elif defined(__powerpc__)

static __inline__ uint64_t rdtsc(void)
{
    uint32_t hi, lo, tmp;
    __asm__ volatile(
        "0:\n"
        "\tmftbu   %0\n"
        "\tmftb    %1\n"
        "\tmftbu   %2\n"
        "\tcmpw    %2,%0\n"
        "\tbne     0b\n"
        : "=r"(hi), "=r"(lo), "=r"(tmp));
    return ((uint64_t)lo) | (((uint64_t)hi) << 32);
}

#elif defined(__ARM_ARCH_ISA_A64)

// see https://stackoverflow.com/questions/40454157/is-there-an-equivalent-instruction-to-rdtsc-in-arm

// Adapted from: https://github.com/cloudius-systems/osv/blob/master/arch/aarch64/arm-clock.cc
static inline uint64_t rdtsc(void) {
    //Please note we read CNTVCT cpu system register which provides
    //the accross-system consistent value of the virtual system counter.
    uint64_t cntvct;
    asm volatile ("mrs %0, cntvct_el0; " : "=r"(cntvct) :: "memory");
    return cntvct;
}

#else

#error "No implementation of rdtsc() available for current architecture"

#endif
