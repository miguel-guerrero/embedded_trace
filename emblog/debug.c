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
#include "debug.h"
#include "debug_hw_specific.h"

static int init = 0;

void DEBUG_putchar(char ch)
{
    if (!init) {
        DEBUG_init();
        init = 1;
    }

    DEBUG_put_char(ch);
}

void DEBUG_print(const char *msg)
{
    if (!init) {
        DEBUG_init();
        init = 1;
    }

    while (*msg) {
       DEBUG_put_char(*msg++);    
    }
}

void DEBUG_println(const char *msg)
{
    DEBUG_print(msg);
    DEBUG_print("\n");
}

#define BCD2HEX(x) ((x) >= 10 ? (x) - 10 + 'A' : (x) + '0')

void DEBUG_print_hex(uint32_t u)
{
    int i;
    for (i=7; i>=0; i--) {
        char c = (u >> 28) & 0xF;
        DEBUG_put_char(BCD2HEX(c));
        u <<= 4;
    }
}

void DEBUG_print_dec(uint32_t u)
{
    char buf[sizeof(u)*3];
    int i=0;
    do {
        uint32_t next_u = u / 10;
        uint32_t ls_digit = u - 10*next_u;
        buf[i++] = '0' + ls_digit;
        u = next_u;
    } while (u>0);
    while (i) {
        DEBUG_put_char(buf[--i]);
    }
}

char DEBUG_get_char()
{
    while (!DEBUG_rx_avail()) {}
    return DEBUG_rx_data();
}

