#!/usr/bin/env python3
# -----------------------------------------------------------------------------
# MIT License
#
# Copyright 2022-Present Miguel A. Guerrero
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal # in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.
# -----------------------------------------------------------------------------
import re
import sys
import argparse


# | TS[32:0] | TS64 | FLAG_VAL |  ID[7:0] |

EMB_LOG_TS_SHIFT = 8
EMB_LOG_TS64_BIT = 7
EMB_LOG_FLAG_VAL_BIT = 6

EMB_LOG_TS64_MASK = 1 << EMB_LOG_TS64_BIT
EMB_LOG_FLAG_VAL_MASK = 1 << EMB_LOG_FLAG_VAL_BIT

# max message id
EMB_LOG_IDX_MAX = EMB_LOG_FLAG_VAL_MASK - 1

# max time-stamp that fits in first word
EMB_LOG_TS_MAX = (1 << (32 - EMB_LOG_TS_SHIFT)) - 1


class MsgInfo:
    def __init__(self):
        self.dec_lst = []
        self.msg_ids = []
        self.msg_type_by_id = dict()
        self.msg_type_by_idx = dict()


# -----------------------------------------------------------------------------
# Return True if the type of the arg indicates is associated with msg id field
# -----------------------------------------------------------------------------
def msg_id_type(x):
    return x == "event" or x == "flag"


# -----------------------------------------------------------------------------
# Process one line of the file that contains the message formats
# -----------------------------------------------------------------------------
def process_msg_fmt_line(msg_info: MsgInfo, fout_hdrs, line):
    def line_to_kv_pairs(line):
        kv_list = []
        for part in re.split(r"\s+", line):
            (name, val) = re.split(r"\s*:\s*", part.strip())
            kv_list.append([name, val])
        return kv_list

    kv_list = line_to_kv_pairs(line)
    msg_id = None
    msg_t = None
    struct_data = []
    level = 1
    for name, typ_or_value in kv_list[::-1]:
        if msg_id_type(typ_or_value):
            msg_id = name
            msg_t = typ_or_value
        elif name == "level":
            level = int(typ_or_value)
        else:
            assert typ_or_value in {"u32"}
            struct_data.append((typ_or_value, name))

    msg_idx = len(msg_info.dec_lst)
    if msg_idx > EMB_LOG_IDX_MAX:
        print(
            f"ERROR: exceeding max number of events allowed {EMB_LOG_IDX_MAX}. "
            "Please increase EMB_LOG_TS_SHIFT",
            file=sys.stderr,
        )
        sys.exit(1)

    msg_info.msg_ids.append(f"{msg_id}={msg_idx:#x}")
    msg_info.dec_lst.append([(n, v) for n, v in kv_list if n != "level"])
    msg_info.msg_type_by_id[msg_id] = msg_t
    msg_info.msg_type_by_idx[msg_idx] = msg_t

    if fout_hdrs:
        assert msg_id is not None
        struct_typ = "log_" + msg_id + "_t"
        struct_str = ""
        for typ, name in struct_data:
            struct_str += "    log_" + typ + " " + name + "_arg;\n"
        typedef = (
            "typedef struct {\n"
            + struct_str
            + "    uint32_t id;\n} "
            + struct_typ
            + ";"
        )
        arg_lst = [
            n for n, v in kv_list if not msg_id_type(v) and n != "level"
        ]

        ext_arg_lst = arg_lst[:]
        if msg_t == "flag":
            ext_arg_lst.insert(0, "flag_val")

        define = "#define EMB_LOG_" + msg_id.upper() + "("
        define += ", ".join(ext_arg_lst)
        define += (
            ")  EMB_LOG_IF(" + str(level) + ", " + struct_typ + " m; \\\n    "
        )
        define += " ".join(
            map(lambda n: "m." + n + "_arg=" + n + "; ", arg_lst)
        )

        if msg_t == "event":
            define += f"m.id={msg_idx:#x};"
        elif msg_t == "flag":
            define += f"m.id=(((flag_val) & 1) << EMB_LOG_FLAG_VAL_BIT) | {msg_idx:#x};"
        else:
            print(f"ERROR: incorrect msg_id type {msg_t}")
            sys.exit(1)

        define += " \\\n    emb_log_add(&m, sizeof(m)))"
        print("//", line, file=fout_hdrs)
        print(f"// id={msg_idx}", file=fout_hdrs)
        print(typedef, file=fout_hdrs)
        print("\n" + define + "\n", file=fout_hdrs)


# -----------------------------------------------------------------------------
# Decode the hex log and put it in formatted list (most recent last)
# -----------------------------------------------------------------------------
def process_hex_log(msg_info: MsgInfo, msg_formats, hex_dump, formated, i):

    # decode the message ID word
    def unpack_msg_id(h):
        msg = int(h, 16)
        msg_idx = msg & EMB_LOG_IDX_MAX
        is_flag = msg_info.msg_type_by_idx[msg_idx] == "flag"
        flag_val = 1 if msg & EMB_LOG_FLAG_VAL_MASK else 0
        flag_ts64 = 1 if msg & EMB_LOG_TS64_MASK else 0
        delta_ts = (msg >> EMB_LOG_TS_SHIFT) & EMB_LOG_TS_MAX
        return msg_idx, is_flag, flag_val, flag_ts64, delta_ts

    dump_len = len(hex_dump)

    # skip entries that look invalid
    invalid = True
    while invalid:
        msg_idx, is_flag, flag_val, flag_ts64, delta_ts = unpack_msg_id(
            hex_dump[i]
        )
        i += 1
        if delta_ts == EMB_LOG_TS_MAX:
            if i >= dump_len:
                return i
            delta_ts = int(hex_dump[i], 16)
            i += 1
        if flag_ts64 == 1:
            if i >= dump_len:
                return i
            delta_ts |= int(hex_dump[i], 16) << 32
            i += 1
        invalid = msg_idx >= len(msg_formats)
        if invalid:
            print("Skipping entry", file=sys.stderr)

    fmt = msg_formats[msg_idx]

    id, _ = fmt[0]

    xargs = []
    for j in range(1, len(fmt)):
        if i >= dump_len:
            return i
        h = int(hex_dump[i], 16)
        xargs.append(f"{fmt[j][0]}={h:#x}")
        i += 1

    formated.insert(0, [delta_ts, id, flag_val if is_flag else -1, xargs])
    return i


# -----------------------------------------------------------------------------
# Capture hex file dump from circular buffer
# -----------------------------------------------------------------------------
def capture_hex_buffer(hex_log):
    hex_dump = []
    with open(hex_log) as fin_hex:
        capturing = False
        for line in fin_hex:
            line = line.strip()
            if line == "":
                continue
            if capturing:
                if line.startswith("=== End buffer dump"):
                    capturing = False
                else:
                    parts = re.split(r"\s+", line)
                    hex_dump.extend(parts)
            elif line.startswith("=== Start buffer dump"):
                hex_dump = []
                capturing = True
    return hex_dump


def extract_hex_msgs(msg_info: MsgInfo, hex_dump):
    formated = []
    k = 0
    while k < len(hex_dump):
        k = process_hex_log(msg_info, msg_info.dec_lst, hex_dump, formated, k)
    return formated


def dump_human_rpt(msg_info: MsgInfo, hex_dump, freq_in_mhz, file_out):

    with open(file_out, "w") as fout:

        # print to fout
        def dump(*args, **kwargs):
            print(*args, **kwargs, file=fout)

        # given the cycle count frequency, convert a time-stamp count
        # into uSec time
        def cycles_to_us(cycles):
            return cycles / (1.0 * freq_in_mhz)

        formated = extract_hex_msgs(msg_info, hex_dump)

        # dump header depending on output style
        dump(
            "   n :        cycle         uSecs  delta-uSecs  event_name args"
        )
        dump(
            "==============================================================="
        )

        # dump trace
        abs_ts = 0
        for cnt, msg in enumerate(formated):
            delta_ts, id, flag_val, xargs = msg
            abs_ts += delta_ts
            dump(
                "%4d : %12d    %10.3f   %10.3f  %s"
                % (
                    cnt,
                    abs_ts,
                    cycles_to_us(abs_ts),
                    cycles_to_us(delta_ts),
                    id,
                ),
                end="",
            )
            if flag_val != -1:
                dump("(%d)" % flag_val, end="")
            if len(xargs) > 0:
                dump(" %s" % (", ".join(xargs)), end="")
            dump()


def dump_internal_trace(msg_info: MsgInfo, hex_dump, freq_in_mhz, file_out):
    formated = extract_hex_msgs(msg_info, hex_dump)

    with open(file_out, "w") as fout:

        # print to fout
        def dump(*args, **kwargs):
            print(*args, **kwargs, file=fout)

        dump(f"0 FREQ_IN_HZ {1e6*freq_in_mhz}")

        # dump header depending on output style
        flag_ids = [
            k for k, v in msg_info.msg_type_by_id.items() if msg_id_type(v)
        ]
        for k in flag_ids:
            dump("0 PUSH_VAR %s bit" % (k))

        # dump trace
        abs_ts = 0
        for cnt, msg in enumerate(formated):
            delta_ts, id, flag_val, xargs = msg
            abs_ts += delta_ts
            type_ = msg_info.msg_type_by_id[id]
            if type_ == "event":
                dump("%d EVENT %s" % (abs_ts, id))
            elif type_ == "flag":
                dump("%d TRACE_VAR %s %d" % (abs_ts, id, flag_val))
            else:
                dump(
                    f"ERROR: unexpected type {type_} for id {id}",
                    file=sys.stderr,
                )
                sys.exit(1)


# -----------------------------------------------------------------------------
# read-up a msgs.txt file and fillup a MsgInfo data structur with it
# if a fout_hdrs is passed (not None) messages are dumped as macros on that
# file
# -----------------------------------------------------------------------------
def process_msgs_file(msgs_filename, fout_hdrs=None):
    msg_info = MsgInfo()
    with open(msgs_filename) as fin_msgs:
        for line in fin_msgs:
            line = line.strip()
            if line.startswith("#") or line == "":
                continue
            process_msg_fmt_line(msg_info, fout_hdrs, line)
    return msg_info


# -----------------------------------------------------------------------------
# main program
# -----------------------------------------------------------------------------
def main():
    parser = argparse.ArgumentParser(
        "gen_log.py", formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    parser.add_argument(
        "--hex_log",
        help="dump file to generate the log from",
    )
    parser.add_argument(
        "--hdrs",
        help="header file to generate for c inclusion",
    )
    parser.add_argument(
        "--msgs",
        default="msgs.txt",
        help="msg definition file",
    )
    parser.add_argument(
        "--output_style",
        choices=["rpt", "vcd"],
        default="vcd",
        help="rpt: readable trace, vcd: VCD waves",
    )
    parser.add_argument(
        "--out_rpt",
        type=str,
        default="/dev/stdout",
        help="output file name for reports",
    )
    parser.add_argument(
        "--freq_in_mhz",
        default=1000.0,
        type=float,
        help="Frequency of timestamp ticks in MHz",
    )
    parser.add_argument(
        "--dbg_level",
        default=1,
        type=int,
        help="messages with level equal or above this will be dumpled",
    )
    group = parser.add_mutually_exclusive_group()
    group.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        dest="verb",
        help="verbose",
    )
    group.add_argument(
        "-q",
        "--quiet",
        action="store_true",
        help="quiet",
    )
    args = parser.parse_args()

    if args.verb:
        print(args.hex_log)
        print(args.hdrs)
        print(args.msgs)
        print("output_style", args.output_style)
        print(args.freq_in_mhz)
        print(args.dbg_level)

    if args.hdrs is None and args.hex_log is None:
        print(
            "ERROR: must specify at least one of -hrds / -hex_log",
            file=sys.stderr,
        )
        exit(1)

    # if we need to generate c-header file
    if args.hdrs:
        print("Generating c-header file", file=sys.stderr)
        with open(args.hdrs, "w") as fout_hdrs:

            def emit(msg):
                print(msg, file=fout_hdrs)

            emit("// Auto-generated by gen_log.py. Plese do not edit")
            emit("#pragma once")
            emit("#include <stdint.h>\n")
            emit("#ifndef EMB_LOG_DBG_LVL")
            emit(f"# define EMB_LOG_DBG_LVL {args.dbg_level}")
            emit("#endif\n")
            emit("#ifndef EMB_LOG_DISABLED")
            emit(
                "# define EMB_LOG_IF(lvl,x) do { if ((lvl) <= EMB_LOG_DBG_LVL) "
                "do { x; } while(0); } while(0)"
            )
            emit("#else")
            emit("# define EMB_LOG_IF(lvl,x) do {} while(0)")
            emit("#endif\n")
            emit("#define EMB_LOG_TS_SHIFT %d" % EMB_LOG_TS_SHIFT)
            emit("#define EMB_LOG_TS64_BIT %d" % EMB_LOG_TS64_BIT)
            emit("#define EMB_LOG_TS64_MASK %d" % EMB_LOG_TS64_MASK)
            emit("#define EMB_LOG_FLAG_VAL_BIT %d" % EMB_LOG_FLAG_VAL_BIT)
            emit("#define EMB_LOG_TS_MAX 0x%x\n" % EMB_LOG_TS_MAX)
            emit("typedef uint32_t log_u32;\n")
            msg_info = process_msgs_file(args.msgs, fout_hdrs)
    else:
        msg_info = process_msgs_file(args.msgs)

    # if there is an input log to process
    if args.hex_log:
        print("Processing log file", args.hex_log, file=sys.stderr)
        hex_dump = capture_hex_buffer(args.hex_log)

        # dump report depending on output style
        if args.output_style == "rpt":
            dump_human_rpt(msg_info, hex_dump, args.freq_in_mhz, args.out_rpt)
        else:
            dump_internal_trace(
                msg_info, hex_dump, args.freq_in_mhz, args.out_rpt
            )


if __name__ == "__main__":
    main()
