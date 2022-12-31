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
// this example implentation uses putchar from stdio.h 
// on an embedded system this may be different
#include <stdio.h> 

void DEBUG_init()
{
  // include here any one time code you may need to execute 
  // to setup a serial port for instance


}   

void DEBUG_wait_for_tx()
{
    // place holder
}

void DEBUG_put_char(char ch)
{
    putchar(ch);
}

int DEBUG_rx_avail()
{
    return 1;
} 

char DEBUG_rx_data()
{
    // not implemented
    return 0;
}
