#!/usr/bin/env python3

###########################################################################
#   fheroes2: https://github.com/ihhub/fheroes2                           #
#   Copyright (C) 2023 - 2024                                             #
#                                                                         #
#   This program is free software; you can redistribute it and/or modify  #
#   it under the terms of the GNU General Public License as published by  #
#   the Free Software Foundation; either version 2 of the License, or     #
#   (at your option) any later version.                                   #
#                                                                         #
#   This program is distributed in the hope that it will be useful,       #
#   but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#   GNU General Public License for more details.                          #
#                                                                         #
#   You should have received a copy of the GNU General Public License     #
#   along with this program; if not, write to the                         #
#   Free Software Foundation, Inc.,                                       #
#   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             #
###########################################################################

# pylint: disable=missing-module-docstring

import argparse
import datetime
import re
import sys

from xml.sax.saxutils import escape

#
# Appstream file can be validated using the following command:
# appstream-util validate io.github.ihhub.Fheroes2.metainfo.xml
#


class CustomArgumentParser(
    argparse.ArgumentParser
):  # pylint: disable=missing-class-docstring
    def error(self, message):
        self.print_usage(sys.stderr)
        self.exit(
            1,
            "%(prog)s: error: %(message)s\n" % {"prog": self.prog, "message": message},
        )


def parse_arguments():  # pylint: disable=missing-function-docstring
    parser = CustomArgumentParser()

    parser.add_argument("changelog_file", help="changelog.txt file")
    parser.add_argument("metainfo_file", help="*.metainfo.xml file")

    return parser.parse_args()


def main():  # pylint: disable=missing-function-docstring
    args = parse_arguments()

    with open(args.changelog_file, encoding="utf-8") as changelog_file:
        changelog = changelog_file.read()

    with open(args.metainfo_file, encoding="utf-8") as metainfo_file:
        metainfo = metainfo_file.read()

    tpl = """
    <release date="{}" version="v{}">
      <url>https://github.com/ihhub/fheroes2/releases/tag/{}</url>
      <description>
        <p>Changes in v{} ({}):</p>
        <ul>
{}
        </ul>
      </description>
    </release>\n""".lstrip(
        "\r\n"
    )
    tmp = ""

    regex = r"version ([\d.]+) \(([\w ]+)\)\n(.*?)\n{2}"
    for match in re.findall(regex, changelog, re.MULTILINE | re.DOTALL):
        tmp += tpl.format(
            datetime.datetime.strptime(match[1], "%d %B %Y").strftime("%Y-%m-%d"),
            match[0],
            match[0],
            match[0],
            match[1],
            re.sub(r"- (.*)", r"          <li>\1</li>", escape(match[2])),
        )

    new_metainfo = re.sub(
        r"<releases>(.*?)</releases>",
        r"<releases>\n" + tmp + "  </releases>",
        metainfo,
        flags=re.MULTILINE | re.DOTALL,
    )

    with open(args.metainfo_file, "w", encoding="utf-8") as metainfo_file:
        metainfo_file.write(new_metainfo)


if __name__ == "__main__":
    main()
