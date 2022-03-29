#!/usr/bin/env python3

import datetime
import re
import subprocess
import sys

CURRENT_YEAR = datetime.datetime.now().date().year

COPYRIGHT_HDR_FULL = """
/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) {YR}                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
""".strip().replace("{YR}", f"{CURRENT_YEAR}")

COPYRIGHT_HDR_TMPL = """
/***************************************************************************
 *   Free Heroes of Might and Magic II: https://github.com/ihhub/fheroes2  *
 *   Copyright (C) {Y1} - {Y2}                                             *
 *                                                                         *
""".strip()

def main():
    copyright_hdr_re_tmpl = COPYRIGHT_HDR_TMPL

    for ch_to_replace in "*()":
        copyright_hdr_re_tmpl = copyright_hdr_re_tmpl.replace(ch_to_replace, "\\" + ch_to_replace)

    copyright_hdr_year_re = re.compile("^" + copyright_hdr_re_tmpl.replace("{Y1} - {Y2}",
                                                                           "([0-9]{4})       "))
    copyright_hdr_yrng_re = re.compile("^" + copyright_hdr_re_tmpl.replace("{Y1}", "([0-9]{4})")
                                                                  .replace("{Y2}", "([0-9]{4})"))

    generic_hdr_re = re.compile("^/\\*.*?\\*/", re.DOTALL)

    for file_name in sys.argv[1:]:
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
                    hdr = COPYRIGHT_HDR_TMPL.replace("{Y1}", f"{year_re_match.group(1)}") \
                                            .replace("{Y2}", f"{CURRENT_YEAR}")
                    src = copyright_hdr_year_re.sub(hdr, src)
                elif yrng_re_match:
                    hdr = COPYRIGHT_HDR_TMPL.replace("{Y1}", f"{yrng_re_match.group(1)}") \
                                            .replace("{Y2}", f"{CURRENT_YEAR}")
                    src = copyright_hdr_yrng_re.sub(hdr, src)
                elif generic_hdr_re.match(src):
                    src = generic_hdr_re.sub(COPYRIGHT_HDR_FULL, src)
                else:
                    src = COPYRIGHT_HDR_FULL + "\n\n" + src.lstrip()

                with open(file_name + ".tmp", "w", encoding="latin_1") as tmp_file:
                    tmp_file.write(src)

                with subprocess.Popen(["diff", "-u", file_name, file_name + ".tmp"]) as diff_proc:
                    diff_proc.wait()

    return 0

if __name__ == "__main__":
    sys.exit(main())
