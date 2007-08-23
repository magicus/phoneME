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

# This file generates HTML and XML formatted server content output.

import os
import zipfile
import cgi
import cgitb
import time
import fnmatch
import xml.sax
import xml.sax.handler
import sys
import provisioning


from urllib import quote, basejoin
from xml.sax.saxutils import escape
from os.path import realpath, join

# ------------ GLOBAL VARIABLES ------------

SERVER_ROOT = "../"

EXT_TO_SHOW = [
    ".jad", ".dd" ]

TO_FETCH = [
    "Created-By",
    "MIDlet-Vendor",
    "MIDlet-Description",
    "MicroEdition-Configuration" ]

TO_FETCH_NAME = [
    "MIDlet-Name" ]

# ------------ FUNCTIONS ------------ 

# Generates XML output
# @param repository     instance of Repository class
# @return void
def process_dir_xml(repository):
    try:
        print provisioning.XML_HEADER
        print '<?xml version="1.0" encoding="utf-8"?>'
        print """<serverContent
            xmlns="http://sun.com/2006/provisioning"
            xmlns:dd="urn:oma:xml:dl:dd:2.0"
            xmlns:xsd="http://www.w3.org/2001/XMLSchema-instance"
            xsd:schemaLocation="http://sun.com/2006/provisioning servercontent.xsd">
            """

        for x in os.listdir(repository.path):
            _, ext = os.path.splitext(x)
            if ext in EXT_TO_SHOW:
                print """<dd:media xmlns="urn:oma:xml:dl:dd:2.0" DDVersion="2.0">
                    <product><mediaObject>"""                
                fullname = join(realpath(repository.path), x)
                fullurl = quote(basejoin(repository.url+"/", x), ":/")
                name, info = get_info(fullname)
                print "<meta><name>%s</name></meta>" % (name)
                print "<size>%d</size>" % (os.path.getsize(fullname))
                print "<type>%s</type>" % (provisioning.get_mime_type(x))
                print "<objectID>%s</objectID>" % (fullurl)
                print "<objectURI><server>%s</server></objectURI>" % fullurl # (cgi.escape(objectURI))
                print "</mediaObject></product></dd:media>"

        print "</serverContent>"
    except Exception, e:
        sys.stderr.write("%s, %s" % (Exception ,e) )

# Parsing MANIFEST file
# @param file       manifest file
# @return string    fields in TO_FETCH
def parse_manifest(manifest):
    info = []
    for line in manifest:
        fields = line.split(":", 1)
        if len(fields) != 2:
            continue
        tag, value = fields
        if tag in TO_FETCH:
            info.append(line)
    return "".join(info)

# Parsing JAR file
# @param string     path to JAR file
# @return couple    (name, result of parse_manifest)
def get_jar_info(filename):
    if not zipfile.is_zipfile(filename):
        return "unknown file type"
    z = zipfile.ZipFile(filename)
    man = z.read("META-INF/MANIFEST.MF")
    lines = []
    if not len(man)==0:
        lines = man.split("\n")
    name, _ = os.path.splitext(filename)
    return name, parse_manifest(lines)

# Parsing JAD file
# @param string     path to JAD file
# @return couple    (name, description)
def get_jad_info(filename):
    name, _ = os.path.splitext(filename)
    _, name = os.path.split(name)
    for line in file(filename):
        fields = line.split(":", 1)
        if len(fields) != 2:
            continue
        tag, value = fields
        if tag in TO_FETCH_NAME:
            name = value.strip()
            break
    return name, parse_manifest(file(filename))

# Parsing DD file for info
# @param string     path to DD file
# @return couple    (name, Vendor + Description)
def get_dd_info(filename):
    mh = provisioning.MyXMLHandler()
    xml.sax.parse(filename, mh)
    _, filename = os.path.split(filename)
    info = filename
    if mh.d.has_key(u'media/description'):
        info = mh.d[u'media/description']
    elif mh.d.has_key(u'media/vendor'):
        info = "Vendor: "+mh.d[u'media/vendor']
    name = filename
    if mh.d.has_key(u'media/name'):
        name = mh.d[u'media/name']
    elif mh.d.has_key(u'media/meta/name'):
        name = mh.d[u'media/meta/name']
    return name, info

# Returns name and description of target file
# @param string     path to file
# @return couple    (name, description)
def get_info(filename):
    try:
        info = "-"
        name, ext = os.path.splitext(filename)
        _, name = os.path.split(name)
        if ext == ".jad":
            name, info = get_jad_info(filename)
        elif ext == ".dd":
            name, info = get_dd_info(filename)
        elif ext in [".jar", ".war", ".ear"]: #".par", 
            name, info = get_jar_info(filename)
    except:
        info = "cannot read meta information"
    return name, info

# Generates HTML output
# @param repository     an instance of Repository class
def process_dir_html(repository):
    try:
        provisioning.print_html_header("Repository Content")
        provisioning.print_manage_btn()
        provisioning.goto_rep_box(repository)
        
        print "<h3>%s</h3><p>%s</p>" % (repository.title, repository.description)
        files_count = len(os.listdir(repository.path))
        print "<center>"
        if files_count>0: print_rep_table(repository)
        else: print "<p>The repository is empty.</p>"
        print "</center>"
        provisioning.print_html_footer()
    except Exception, e:
        print "<div class=error_message>An error occured.</div>"
        sys.stderr.write("%s, %s" % (Exception ,e) )

# Draws repository content table
# @param repository
def print_rep_table(repository):
    print "<table class=table cellpadding=3 cellspacing=0><tr class=table_header>"
    for title in ["Name", "Size", "Type", "Time", "Info"]:
        print "<td>%s</td>" % (title)
    print "</tr>"

    odd = True
    for x in os.listdir(repository.path):
       _, ext = os.path.splitext(x)
       if ext in EXT_TO_SHOW:
            fullname = join(realpath(repository.path), x)
            fullurl = quote(basejoin(repository.url+"/", x), ":/")

            if odd: print "<tr class=odd_row>"
            else:   print "<tr class=even_row>"
            odd = not odd
            name, info = get_info(fullname)
            info = provisioning.crlf_to_cr(info).replace("\n", "<br>")
            print '<td><a href="%s">%s</a></td>' % (fullurl, name)
            print "<td class=size_column>%s</td>" % (str(os.path.getsize(fullname)))
            print "<td class=type_column><em>%s</em></td>" % (ext) # (provisioning.get_mime_type(x))
            print "<td class=time_column>%s</td>" % (time.ctime(os.path.getctime(fullname)))
            print "<td class=info_column>%s</td></tr>" % ( info )
    print "</table>"

# Shows "The server doesnt't have a repository" message
def server_is_empty():
    print provisioning.HTML_HEADER
    print """
    <html><head><title>%s</title></head>
    <body>
    <h1>%s</h1>
    <p>The server doesn't have a repository.</p>
    </body></html>
    """ % (provisioning.SERVER.name, provisioning.SERVER.name)

# infoURI response
# @param repository
# @param string         URL to file
def return_fileinfo(repository, fileurl):
    filename = fileurl.replace(repository.path, repository.url)
    _, info = get_info(filename)
    print info

# ------------ MAIN

def cgi_main():
    # getting a Form object from CGI input
    form = cgi.FieldStorage()
    
    # "NULL object"
    repository = provisioning.Repository()

    if form.has_key("repository"):
        repository = provisioning.get_rep_by_name(form["repository"].value)
    elif len(provisioning.SERVER.content)>0:
        repository = provisioning.SERVER.content[0]
    else: server_is_empty()
    
    if form.has_key("id"):
        if form.has_key("installed"):
            print
            return
        print provisioning.TEXT_HEADER
        fileurl = form["id"].value
        return_fileinfo(repository, fileurl)
        return
    
    if form.has_key("type"):
        if form["type"].value == "xml":
            process_dir_xml(repository)
        else:
            process_dir_html(repository)
    else:
        process_dir_html(repository)

# ------------ START

try:
    cgitb.enable()
    provisioning.init(SERVER_ROOT)
    cgi_main()
except Exception, e:
    sys.stderr.write("Provisioning server error: %s, %s" %(Exception, e))
