#!/usr/bin/env python3

import datetime
import os
import re
import subprocess
import sys

CURRENT_YEAR = datetime.datetime.now().date().year

def main():
    if len(sys.argv) < 4:
        print(f"Syntax: {os.path.basename(sys.argv[0])} full_header_file" +
                                                      " header_template_file" +
                                                      " source_file ...",
              file=sys.stderr)
        return 1

    with open(sys.argv[1], "r", encoding="latin_1") as full_header_file:
        copyright_hdr_full = full_header_file.read().strip().replace("{YR}", f"{CURRENT_YEAR}")
    with open(sys.argv[2], "r", encoding="latin_1") as header_template_file:
        copyright_hdr_tmpl = header_template_file.read().strip()

    copyright_hdr_re_tmpl = copyright_hdr_tmpl

    for ch_to_replace in "*()":
        copyright_hdr_re_tmpl = copyright_hdr_re_tmpl.replace(ch_to_replace, "\\" + ch_to_replace)

    copyright_hdr_year_re = re.compile("^" + copyright_hdr_re_tmpl.replace("{Y1} - {Y2}",
                                                                           "([0-9]{4})       "))
    copyright_hdr_yrng_re = re.compile("^" + copyright_hdr_re_tmpl.replace("{Y1}", "([0-9]{4})")
                                                                  .replace("{Y2}", "([0-9]{4})"))

    generic_hdr_re = re.compile("^/\\*.*?\\*/", re.DOTALL)

    for file_name in sys.argv[3:]:
        if not os.path.exists(file_name):
            continue
        with open(file_name, "r", encoding="latin_1") as src_file:
            src = src_file.read()

            year_re_match = copyright_hdr_year_re.match(src)
            yrng_re_match = copyright_hdr_yrng_re.match(src)

            src_is_ok = False

            if year_re_match:
                if year_re_match.group(1) == f"{CURRENT_YEAR}":
                    src_is_ok = True
            elif yrng_re_match:
                if yrng_re_match.group(2) == f"{CURRENT_YEAR}":
                    src_is_ok = True

            if not src_is_ok:
                if year_re_match:
                    hdr = copyright_hdr_tmpl.replace("{Y1}", f"{year_re_match.group(1)}") \
                                            .replace("{Y2}", f"{CURRENT_YEAR}")
                    src = copyright_hdr_year_re.sub(hdr, src)
                elif yrng_re_match:
                    hdr = copyright_hdr_tmpl.replace("{Y1}", f"{yrng_re_match.group(1)}") \
                                            .replace("{Y2}", f"{CURRENT_YEAR}")
                    src = copyright_hdr_yrng_re.sub(hdr, src)
                elif generic_hdr_re.match(src):
                    src = generic_hdr_re.sub(copyright_hdr_full, src)
                else:
                    src = copyright_hdr_full + "\n\n" + src.lstrip()

                with open(file_name + ".tmp", "x", encoding="latin_1") as tmp_file:
                    tmp_file.write(src)

                with subprocess.Popen(["diff", "-u", file_name, file_name + ".tmp"]) as diff_proc:
                    diff_proc.wait()

                os.remove(file_name + ".tmp")

    return 0

if __name__ == "__main__":
    sys.exit(main())
