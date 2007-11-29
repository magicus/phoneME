#!/usr/bin/python

#
# Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
# DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License version
# 2 only, as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License version 2 for more details (a copy is
# included at /legal/license.txt).
#
# You should have received a copy of the GNU General Public License
# version 2 along with this work; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
# 02110-1301 USA
#
# Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
# Clara, CA 95054 or visit www.sun.com if you need additional
# information or have any questions.
#

# Common module for provisioning server

import sys
import xml.sax
import xml.sax.handler
import mimetypes
import os

from xml.sax.saxutils import XMLGenerator
from xml.sax.saxutils import escape
from os.path import realpath, join

# ------------ GLOBAL VARIABLES ------------
SERVER_ROOT = "../"
REPOSITORY_CONFIG = "repository/repconf.xml"

HTML_HEADER = "Content-Type: text/html\n"
XML_HEADER = "Content-Type: text/xml\n"
TEXT_HEADER = "Content-Type: text/plain\n"

# ------------ CLASSES ------------ 

class Server:
    def __init__(self):
        self.path = os.path.normpath(join(os.getcwd(), SERVER_ROOT))+"/"
        self.name = "Provisioning Server"
        self.url = "/"
        self.description = ""
        # Array of Repository objects
        self.content = []
    
    def showbundles(self):
        return join(self.url, "bin/showbundles.py")
    def admin_console(self):
        return join(self.url, "bin/admin/admin_console.py")
    def styles(self):
        return join(self.url, "styles/")

class Repository:
    def __init__(self):
        self.name = "repository"
        self.url = "repository/"
        self.description = ""
        self.path = "repository/"
        self.title = "Unknown Repository"
        self.web = "/"

# Handler for configuration file parsing (repconf.xml)
class RepXMLHandler(xml.sax.handler.ContentHandler):
    def __init__(self):
        self.reps = []
        self.rep = Repository()
        self.path = [];
        self.chars = "";

    def startElement(self, name, attrs):
        self.path.append(name.lower())
        if ["server", "repository"] == self.path:
            self.rep.name = attrs.get("name", "")

    def endElement(self, name):
        path = self.path
        self.chars = self.chars.strip()
        if ["server"] == path: SERVER.content = self.reps
        elif ["server", "repository"] == path:
            self.rep.web = SERVER.showbundles() + "?repository="+self.rep.name
            self.rep.path = os.path.normpath(join(SERVER.path, join("repository/", self.rep.name)))
            self.rep.url = join(SERVER.url, join("repository/", self.rep.name))
            # We're trying to prohibit modification of files out of the repository
            if self.rep.path.startswith(join(SERVER.path, "repository/")):
                self.reps.append(self.rep)
            self.rep = Repository()
        elif ["server", "url"] == path: SERVER.url = self.chars
        elif ["server", "title"] == path: SERVER.name = self.chars
        elif ["server", "description"] == path: SERVER.description = self.chars
        elif ["server", "repository", "title"] == path: self.rep.title = self.chars
        elif ["server", "repository", "description"] == path: self.rep.description = self.chars
        self.chars = ""
        self.path.pop()

    def characters(self, s):
        self.chars = self.chars + s

# Handler for download descriptor parsing. Generates a map of xml (path:content)
class MyXMLHandler(xml.sax.handler.ContentHandler):
    def __init__(self):
        self.d = {}
        self.path = []
        self.chars = ""

    def startElement(self, name, attrs):
        self.path.append(name)

    def endElement(self, name):
        self.d['/'.join(self.path)] = self.chars.strip()
        self.chars = ""
        self.path.pop()

    def characters(self, s):
        self.chars = self.chars + s

# Returns repository object by name of repository
def get_rep_by_name(rep_name):
    if len(SERVER.content)>0:
        repository = SERVER.content[0]
        for rep in SERVER.content:
            if rep.name == rep_name:
                repository = rep
                break
    else: repository = Repository()
    return repository

# Reading server configuration, initializing MIME module
def init(server_root):
    try:
        global SERVER_ROOT
        SERVER_ROOT = server_root
        global SERVER
        SERVER = Server()

        mimetypes.init()
        repxml = RepXMLHandler()
        xml.sax.parse(join(SERVER_ROOT, REPOSITORY_CONFIG), repxml)

    except Exception, e:
        sys.stderr.write("Cannot initialize provisioning server: %s, %s" % (Exception ,e) )

# Generates HTML header for HTML output
def print_html_header(title):
    print HTML_HEADER
    
    # --- Title
    print "<html><head><title>%s</title>" % (escape(title))
    print """<link type=text/css rel=StyleSheet href="%sstyles.css" media=screen></link>
        <link type=text/css rel=StyleSheet href="%sstyles_print.css" media=print></link>
        </head><body>""" % (escape(SERVER.styles()), escape(SERVER.styles()))
    
    # --- Header
    print "<div id=hr6></div>"
    print """<h1>%s</h1> <h2>%s</h2>""" % (escape(SERVER.name), escape(title))

# Shows horizontal rule and closes HTML document
def print_html_footer():
    print """
    <div id=hr6></div>
    <div>Copyright  2007 Sun Microsystems, Inc. All rights reserved.</div><br>
    </body></html>"""

# Shows ComboBox with a list of available repositories
# @param repository
# @param bool 1 if we're in management console, 0 if we're not
def goto_rep_box(repository, admin=0):
    if len(SERVER.content) < 2:
        return
    if admin: action = SERVER.admin_console()
    else: action = SERVER.showbundles()
    print """<form class=screen action="%s" method=get>
    <span>Select a repository:</span> <select name=repository>
    """ % (action)
    for rep in SERVER.content:
        if rep==repository:
            print "<option value=%s disabled class=strong>%s</option>" % (escape(rep.name), escape(rep.title))
        else:
            print "<option value=%s>%s</option>" % (escape(rep.name), escape(rep.title))
    print '</select><input type=submit value="Go!"></form>'

# Converts \r\n to \n
def crlf_to_cr(str):
    return str.replace("\r\n", "\n")

# Shows "Manage..." button
def print_manage_btn():
    print """<span class=screen><form action="%(action)s" type=get class=right>
        <input type=submit value="Manage...">
        </form></span><br><br>
        """ % {"action": SERVER.admin_console()}

# Returns MIME type of a file by its name
def get_mime_type(filename):
    _, ext = os.path.splitext(filename)
    mime_type, encoding = mimetypes.guess_type("."+ext)
    if mime_type is None:
        mime_type = "application/octet-stream"
    return mime_type

# Returns content of repconf.xml file
def get_config():
    conf = file(join(SERVER.path, REPOSITORY_CONFIG))
    text = crlf_to_cr(conf.read())
    conf.close()
    return text

# Writes new repconf.xml file
def write_new_conf(text):
    conf = file(join(SERVER.path, REPOSITORY_CONFIG), "w")
    conf.write(crlf_to_cr(text))
    conf.close()
