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

# Management functions

import os
import zipfile
import cgi
import cgitb
import time
import fnmatch
import xml.sax
import xml.sax.handler
import shutil
import sys

# provisioning module is a bit higher
sys.path.append("..")
import provisioning

from os.path import realpath, join

SERVER_ROOT = "../../"

# --------------- FUNCTIONS ----------------
def cgi_main():
    form = cgi.FieldStorage()
    if form.has_key("repository"):
        repository = provisioning.get_rep_by_name(form["repository"].value)
        message, status = "", True
        if form.has_key("add_file") and form["add_file"].file:
            filename = form["add_file"].filename
            fileobj = form["add_file"].file
            message, status = add_file(repository, fileobj, filename)
            
        if form.has_key("delete_file"):
            message, status = delete_file(repository, form["delete_file"].value)
            
        manage_repository_page(repository, message, status)
    elif form.has_key("new_conf"):
        message, status = edit_config(form["new_conf"].value)
        provisioning.init(SERVER_ROOT)
        choose_repository_page(message, status)
        
    else: choose_repository_page()

# Updating configuration
# @param string new configuration
# @return couple (message, status)
def edit_config(text):
    try:
        provisioning.write_new_conf(text)
        return "Server configuration was updated successfully.", True
    except Exception, e:
        return "%s" % (e), false

# Adding a new file to repository
# @param repository to add a file
# @param file object of file to be added
# @param string file name
# @return couple (message, status)
def add_file(repository, fileobj, filename):
    try:
        filename = filename.replace("\\", "/")
        filename = os.path.normpath(filename)
        filename = os.path.basename(filename)
        targetfile = file(join(repository.path, filename), "w")
        shutil.copyfileobj(fileobj, targetfile)
        return "File "+ join(repository.path, filename)+" added.", True
    except Exception, e:
        return "%s" % (e), false

# Deleting a file
# @param repository with the file
# @param string file name
# @return couple (message, status)
def delete_file(repository, filename):
    try:
        os.remove(join(repository.path, filename))
        return "File "+join(repository.path, filename)+" deleted.", True
    except Exception, e:
        return "%s [Error %d]: %s" % (e.strerror, e.errno, e.filename), false

# Shows repository management page
# @param repository
# @param string message
# @param bool status True=OK, else Error
def manage_repository_page(repository, message="", status=True):
    provisioning.print_html_header("Repository Management")
    try:
        provisioning.print_manage_btn()
        provisioning.goto_rep_box(repository, 1)

        # Shows heading with repository title and description
        print "<h3>%s</h3><p>%s</p>" % (repository.title, repository.description)
    
        # Shows brief repository info
        files_count = len(os.listdir(repository.path))
        print """
            <strong>Name:</strong> <em>%s</em><br>
            <strong>URI:</strong> <em>%s</em><br>
            <strong>Web home:</strong> <em><a href="%s">%s</a></em><br>
            <strong>Files count:</strong> <em>%d</em><br>
            """ % (repository.name, repository.url, repository.web, repository.web,
            files_count)
        
        # Shows status of previous operation
        if not message == "":
            if status == True: print "<div class=message>%s</div>" % (message)
            else: print "<div class=error_message>%s</div>" % (message)
        
        # Add a new file form
        print "<center>"
        print """
            <form action=%s enctype=multipart/form-data method=post class=screen>
            <span>Add a new file:</span>
            <input type=hidden value=%s name=repository>
            <input type=file size=40 value=%s accept="*.*" name=add_file>
            <input type=Submit value="Add"></input>
            </form>
            """ % (provisioning.SERVER.admin_console(), repository.name, repository.path)
        
        # Shows repository content
        if files_count > 0: print_rep_table(repository)
        else: print "<p>The repository is empty.</p></center>"
    except Exception, e:
        print "<div class=error_message>Sorry, an error occured.</div>"
        sys.stderr.write("Cannot show 'manage repository page': %s, %s" %(Exception, e))
    provisioning.print_html_footer()

# Draws repository content table with 'delete' option
# @param repository
def print_rep_table(repository):    
    print "<table class=table cellpadding=3 cellspacing=0><tr class=table_header>"
    for title in ["File Name", "Size", "Time"]:
        print "<td>%s</td>" % (title)
    print "<td class=screen>&nbsp;</td>"
    print "</tr>"

    odd = True
    for x in os.listdir(repository.path):
        fullname = join(repository.path, x)
        if odd: print "<tr class=odd_row>"
        else:   print "<tr class=even_row>"
        odd = not odd
        
        print '<td><a href="%s">%s</a></td>' % (join(repository.url, x), x)
        print "<td class=size_column>%s</td>" % (str(os.path.getsize(fullname)))
        print "<td class=time_column>%s</td>" % (time.ctime(os.path.getctime(fullname)))
        print """<td class=screen><a href="%s?repository=%s&delete_file=%s">delete</a></td>
            """  % (provisioning.SERVER.admin_console(), repository.name, x)
        print "</tr>"
    print "</table></center>"

# Generates main Management Console page with configuration editing text area
def choose_repository_page(message="", status=True):
    provisioning.print_html_header("Management Console")
    try:
        print "<p>%s</p>" % (provisioning.SERVER.description)
    
        odd = 1
        for rep in provisioning.SERVER.content:
            if odd: print "<div class=odd_row_list>"
            else:   print "<div class=even_row_list>"
            odd = not odd
            
            print '<a class=item_name href="%s">%s</a><br>' % (rep.web, rep.title)
            print "<div class=list_text>%s" % (rep.description)
            manage_url = provisioning.SERVER.admin_console() + "?repository=" + rep.name
            print '<a class=screen href="%s">...Edit</a>' % (manage_url)
            print "</div></div>"
    
        if not message == "":
            if status == True: print "<div class=message>%s</div>" % (message)
            else: print "<div class=error_message>%s</div>" % (message)

        print """<p>Edit repository configuration directly:</p>
        <form action="%s" method=post>
        """ % (provisioning.SERVER.admin_console())
        conf_xml = provisioning.get_config()
        print """
        <textarea name="new_conf" rows=20 cols=80>%s</textarea><br> 
        <input type=submit value=Submit> <input type=reset>
        </form>
        """ % (conf_xml)

    except Exception, e:
        print "<div class=error_message>Sorry, an error occured.</div>"
        sys.stderr.write("Cannot show 'choose repository page': %s, %s" %(Exception, e))
    provisioning.print_html_footer()

try:
    cgitb.enable()
    provisioning.init(SERVER_ROOT)
    cgi_main()
except Exception, e:
    sys.stderr.write("Provisioning server error: %s, %s" %(Exception, e))
