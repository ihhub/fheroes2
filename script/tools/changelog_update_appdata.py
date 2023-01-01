#!/usr/bin/env python3

import argparse
import datetime
import re
import sys
from xml.sax.saxutils import escape

#
# Appstream file could be validated with 'appstream-util validate io.github.ihhub.Fheroes2.appdata.xml'
#

class CustomArgumentParser(argparse.ArgumentParser):
    def error(self, message):
        self.print_usage(sys.stderr)
        self.exit(1, "%(prog)s: error: %(message)s\n" %
                  {"prog": self.prog, "message": message})

def parse_arguments():
    parser = CustomArgumentParser()
    parser.add_argument("changelog_file", help="changelog.txt file")
    parser.add_argument("appdata_file", help="*.appdata.xml file")
    return parser.parse_args()

def main():
    args = parse_arguments()
    
    changelog = open(args.changelog_file, encoding='utf-8').read()
    appdata = open(args.appdata_file, encoding='utf-8').read()
    
    tpl = '''
    <release date="{}" version="v{}">
      <url>https://github.com/ihhub/fheroes2/releases/tag/{}</url>
      <description>
        <p>Changes in v{} ({}):</p>
        <ul>
{}
        </ul>
      </description>
    </release>\n'''.lstrip("\r\n")
    tmp = ''
    
    regex = r"version ([\d.]+) \(([\w ]+)\)\n(.*?)[\n]{2}"
    for match in re.findall(regex, changelog, re.MULTILINE | re.DOTALL):
        tmp += tpl.format(
            datetime.datetime.strptime(match[1], '%d %B %Y').strftime('%Y-%m-%d'),
            match[0],
            match[0],
            match[0],
            match[1],
            re.sub(r'- (.*)', r'          <li>\1</li>', escape(match[2]))
        )
        
    new_appdata = re.sub(r'<releases>(.*?)</releases>', r'<releases>\n' + tmp + '  </releases>',
        appdata,
        flags=re.MULTILINE | re.DOTALL
    )
    
    open(args.appdata_file, 'w', encoding='utf-8').write(new_appdata)

if __name__ == "__main__":
    main()
