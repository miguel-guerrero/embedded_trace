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

#ifndef EMB_LOG_ENTRIES
 // each entry is 4 B. Different messages may use different number of entries
 // depending on arguments and size of relative time-stamp (1 being minimum)
 #define EMB_LOG_ENTRIES 256   // 1KB
#endif

// Required call before usage to initialize internal data structures
void emb_log_init();

// Enable/disable log based on argument
void emb_log_set_enable(int on);

// Enable the log after 'cnt' messages have been attempted to log
void emb_log_start_after_cnt_msgs(int cnt);

// Disable log after log has been enabled and 'cnt' messages
// have been logged
void emb_log_stop_after_cnt_capt_msgs(int cnt);

// Dump current log
void emb_log_dump(int format);

// add an entry to the log. Usually io
void emb_log_add(void *msg, int msg_byte_len);

