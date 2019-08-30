#  Copyright (C) 2008 by Josh Matthews <josh@joshmatthews.net>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the
#  Free Software Foundation, Inc.,
#  59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

"""
    cdinfo.py - a minimal CDDB server replica

    This script is designed to be run before ripping the Heroes of Might & Magic II CD to disk.
    Most ripping programs allow you to specify the address for the CDDB lookup, in which case
    you should set the address to 'localhost', and the port to 8880, and the ripper should then
    have all the proper track information that the fheroes2 music system wants.  Then rip to OGG
    format, and place the files in the data/ subdirectory of fheroes2.
    
    For reference, the final ripped music files should follow this pattern:
      02 Battle (1).ogg
      ...
      10 Wizard Castle.ogg
      ...
      25 25.ogg
      ...
      43 Scenario Victory.ogg
"""

import socket
import sys
import time

offsets = []
disclength = ""

tracks = [
    "02 Battle (1)",
    "03 Battle (2)",
    "04 Battle (3)",
    "05 Barbarian Castle",
    "06 Sorceress Castle",
    "07 Warlock Castle",
    "08 Wizard Castle",
    "09 Necromancer Castle",
    "10 Knight Castle",
    "11 Lava Theme",
    "12 Wasteland Theme",
    "13 Desert Theme",
    "14 Snow Theme",
    "15 Swamp Theme",
    "16 Ocean Theme",
    "17 Dirt Theme",
    "18 Grass Theme",
    "19 Lost Game",
    "20 Week (1)",
    "21 Week (2) Month (1)",
    "22 Month (2)",
    "23 Map Puzzle",
    "24 Roland's Campaign",
    "25",
    "26",
    "27",
    "28",
    "29",
    "30",
    "31",
    "32",
    "33",
    "34",
    "35",
    "36",
    "37",
    "38",
    "39",
    "40",
    "41",
    "42 Main Menu",
    "43 Scenario Victory"
]

discinfo = {
    "DTITLE" : "New World Computing / Heroes of Might & Magic II Soundtrack",
    "DYEAR" : 1996,
    "DGENRE" : "Soundtrack",
    "PLAYORDER" : ""
}

def debug(*args):
    for arg in args:
        print arg,
    print '\n',
    sys.stdout.flush()

class Client:
    def __init__(self, sock, addr):
        self.sock = sock
        self.addr = addr
        self.data = ""
        self.http = False
    
    def send(self, data):
        self.data += data
        self.data += "\n"
    
    def httpFlush(self):
        response = "HTTP/1.1 200 OK\r\n"
        response += "Content-Type: text/plain\r\n"
        response += "Content-Length: " + str(len(self.data)) + "\r\n"
        response += self.data + "\r\n"
        debug("sending response (%s)" % response)
        self.sock.send(response)
    
    def flush(self):
        if not self.data:
            return
    
        if not self.http:
            debug("sending response (%s)" % self.data.strip())
            self.sock.send(self.data)
        else:
            self.httpFlush()
        self.data = ""
    
    def close(self):
        self.sock.close()
    
    def recv(self, size):
        return self.sock.recv(size)

def actOnAccept(client):
    client.send("201 %s CDDBP server v1.0PL0 ready at %s" % (socket.gethostname(), time.strftime("%a %b %H:%M:%S %Y")))
    return True

def actOnQuery(client, data):
    global disclength
    elements = data.split(' ')
    track = 0
    while track < int(elements[3]):
        offsets.append(elements[4 + track].strip())
        track += 1
    disclength = elements[len(elements) - 1].strip()
    client.send("200 soundtrack %s %s" % (elements[2].strip(), discinfo["DTITLE"]))
    return True

def actOnRead(client, data):
    global disclength
    elements = data.split(' ')
    client.send("210 soundtrack %s CD database entry follows (until terminating `.')" % elements[3].strip())
    client.send("# xmcd 2.0 CD database file")
    client.send("# Track frame offsets:")
    for offset in offsets:
        client.send("# " + offset)
    client.send("#")
    client.send("# Disc length: " + disclength + " seconds")
    client.send("# Revision: 0")
    client.send("# Submitted via: jmatthews v1.0PL0");
    client.send("DISCID="+elements[3].strip())
    for key, value in discinfo.items():
        client.send(key + "=" + str(discinfo[key]))
    for tracknum, track in enumerate(tracks):
        client.send("TTITLE" + str(tracknum + 1) + "=" + track)
    client.send('.')
    return True

def actOnQuit(client, data):
    client.send("%s Closing connection.  Goodbye." % client.addr[0])
    return False

def actOnHello(client, data):
    elements = data.split(' ')
    elements = map(lambda x: x.strip(), elements)
    client.send("200 hello and welcome %s@%s running %s %s" % (elements[2], elements[3],
                                                               elements[4], elements[5]))
    return True

def actOnProto(client, data):
    elements = data.split(' ')
    elements = map(lambda x: x.strip(), elements)
    client.send("201 OK, protocol version now: %s" % elements[1])
    return True

def actHTTPcmd(client, data):
    elements = data.split(' ')
    parts = elements[1].split('&')
    parts[0] = parts[0][5:]
    cmd = parts[0].replace('+', ' ')
    client.http = True
    #return parseMessage(client, cmd)
    return False

# Based on http://ftp.freedb.org/pub/freedb/latest/CDDBPROTO

def parseMessage(client, data):
    if not data:
        return False
    
    handled = False
    ret = True
    handlers = {
        "GET"        : actHTTPcmd,
        "cddb hello" : actOnHello,
        "cddb query" : actOnQuery,
        "cddb read"  : actOnRead,
        "quit"       : actOnQuit,
        "proto"      : actOnProto
    }
    for key, handler in handlers.items():
        if data.startswith(key):
            debug("handling message (%s)" % data.strip())
            ret = handler(client, data)
            client.flush()
            if client.http:
                ret = False
            handled = True
            break
    if not handled:
        debug("Received unexpected message (%s)" % data.strip())
        client.send("500 error")
    return ret

def main(argc, argv):
    host = ''
    port = 8880
    if argc == 2:
        port = int(argv[1])
    size = 1024
    s = None
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.bind((host,port))
        s.listen(1)
    except socket.error, (value, message):
        if s:
            s.close()
            debug("Socket error:", message)
            return 1
    debug("Started server on port %s" % port)
    while 1:
        time.sleep(0.01)
        try:
            sock, address = s.accept()
            debug("Connection by", address)
            client = Client(sock, address)
            actOnAccept(client)
            client.flush()
            running = True
            while running:
                time.sleep(0.01)
                data = client.recv(size)
                running = parseMessage(client, data)
            client.close()
        except socket.error, (value, message):
            debug("Socket error:", message)
            client.close()
        except KeyboardInterrupt:
            debug("Shutting down server")
            s.close();
            break
    return 0

if __name__ == "__main__":
    sys.exit(main(len(sys.argv), sys.argv))