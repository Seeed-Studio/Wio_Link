#!/usr/bin/python

#   Copyright (C) 2015 by seeedstudio
#   Author: Jack Shao (jacky.shaoxg@gmail.com)
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import os
import sys
import re
import json
import time as systime
from datetime import *

TYPE_MAP = {
    'SULI_EDT_NONE':'null',
    'SULI_EDT_BOOL':'bool',
    'SULI_EDT_UINT8':'uint8_t',
    'SULI_EDT_INT8':'int8_t',
    'SULI_EDT_UINT16':'uint16_t',
    'SULI_EDT_INT':'int16_t',
    'SULI_EDT_INT16':'int16_t',
    'SULI_EDT_UINT32':'uint32_t',
    'SULI_EDT_INT32':'int32_t',
    'SULI_EDT_FLOAT':'float',
    'SULI_EDT_STRING':'char *'
}


def parse_one_driver_dir (driver_dir):
    files = []
    for f in os.listdir(driver_dir):
        full_path = os.path.join(driver_dir,f)
        if os.path.isfile(full_path) and (full_path.endswith(".h") or full_path.endswith(".cpp")):
            files.append(f)
    return files

def get_class_header_file (files, dir_name):
    for f in files:
        if f.endswith(".h") and f.find("class") > -1:
            return f
    for f in files:
        if f.endswith(".h") and f.find(dir_name) > -1:
            return f
    for f in files:
        if f.endswith(".h"):
            return f

    return ""

def find_comments(comment_type, content):
    comments = re.findall(r'(\/\*\*[\s\S]*?\*\/)[\r\n]+(.*)', content, re.M)
    result = []
    for comment in comments:
        if comment_type == 'read':
            n = re.findall(r'\s+bool\s+(read_[a-zA-z0-9_]+)\((.*)\)', comment[1])
            if n:
                result.append((comment[0], n[0][0]))
        elif comment_type == 'write':
            n = re.findall(r'\s+bool\s+(write_[a-zA-z0-9_]+)\((.*)\)', comment[1])
            if n:
                result.append((comment[0], n[0][0]))
        elif comment_type == 'event':
            n = re.findall(r'\s+DEFINE_EVENT\((.*)\s*,\s*(.*)\)', comment[1])
            if n:
                result.append((comment[0], n[0][0]))
    #print result
    return result

def parse_class_header_file (file):
    patterns = {}
    doc = {}
    doc_methods = {}
    doc_events = {}
    print file
    content = open(file, 'r').read()

    json_dump = ""
    try:
        json_dump = json.dumps(content);
    except Exception,e:
        print e

    if not json_dump:
        try:
            content = unicode(content,"ISO-8859-1")
            json_dump = json.dumps(content)
        except Exception,e:
            print e
    if not json_dump:
        try:
            content = unicode(content,"GB2312")
            json_dump = json.dumps(content)
        except Exception,e:
            print e

    if not json_dump:
        return ("Encoding of source file is not one of: utf8,iso-8859-1,gb2312", {}, {})


    ##grove name
    grove_name = re.findall(r'^//GROVE_NAME\s+"(.+)"', content, re.M)
    print grove_name
    if grove_name:
        patterns["GroveName"] = grove_name[0].rstrip('\r')
    else:
        return ("can not find GROVE_NAME in %s"%file, {},{})

    ##SKU
    sku = re.findall(r'^//SKU\s+([a-zA-z0-9\-_]+)', content, re.M)
    print sku
    if sku:
        patterns["SKU"] = sku[0].rstrip('\r')
    else:
        return ("can not find SKU in %s"%file, {},{})


    ##interface type
    if_type = re.findall(r'^//IF_TYPE\s+([a-zA-z0-9]+)', content, re.M)
    print if_type
    if if_type:
        patterns["InterfaceType"] = if_type[0]
    else:
        return ("can not find IF_TYPE in %s"%file,{}, {})
    ##image url
    image_url = re.findall(r'^//IMAGE_URL\s+(.+)', content, re.M)
    print image_url
    if image_url:
        patterns["ImageURL"] = image_url[0].rstrip('\r')
    else:
        return ("can not find IMAGE_URL in %s"%file,{}, {})

    ##DESCRIPTION
    description = re.findall(r'^//DESCRIPTION\s+"(.+)"', content, re.M)
    print description
    if description:
        patterns["Description"] = description[0].rstrip('\r')
    else:
        return ("can not find DESCRIPTION in %s"%file, {},{})

    ##WIKI_URL
    wiki_url = re.findall(r'^//WIKI_URL\s+(.+)', content, re.M)
    print wiki_url
    if wiki_url:
        patterns["WikiURL"] = wiki_url[0].rstrip('\r')
    else:
        return ("can not find WIKI_URL in %s"%file,{}, {})

    ##ADDED_AT
    added_at = re.findall(r'^//ADDED_AT\s+"(.+)"', content, re.M)
    print added_at
    if added_at:
        date_str = added_at[0].rstrip('\r')
        _time = None
        fmt = ['%Y-%m-%d', '%Y-%m-%d %H:%M', '%Y-%m-%d %H:%M:%S']
        for f in fmt:
            try:
                _time = systime.strptime(date_str, f)
                break
            except Exception,e:
                print e
                _time = None
        if not _time:
            return ("%s is not the valid date format, should be YYYY-mm-dd"%date_str, {},{})
        else:
            timestamp = int(systime.mktime(_time))
        patterns["AddedAt"] = timestamp
        print timestamp
    else:
        return ("can not find ADDED_AT in %s"%file, {},{})

    ##AUTHOR
    author = re.findall(r'^//AUTHOR\s+"(.+)"', content, re.M)
    print author
    if author:
        patterns["Author"] = author[0].rstrip('\r')
    else:
        return ("can not find AUTHOR in %s"%file, {},{})

    ##class name
    class_name = re.findall(r'^class\s+([a-zA-z0-9_]+)', content, re.M)
    print class_name
    if class_name:
        patterns["ClassName"] = class_name[0]
    else:
        return ("can not find class name in %s"%file,{}, {})
    ##construct function arg list
    arg_list = re.findall(r'%s\((.*)\);'%class_name[0], content, re.M)
    print arg_list
    if arg_list:
        patterns["ConstructArgList"] = [x.strip(" ") for x in arg_list[0].split(',')]
    else:
        return ("can not find construct arg list in %s"%file,{}, {})

    ## read functions
    read_functions = re.findall(r'^\s+bool\s+(read_[a-zA-z0-9_]+)\((.*)\).*$', content, re.M)
    print read_functions
    reads = {}
    for func in read_functions:
        args = func[1].split(',')
        args = [x.strip() for x in args]
        if 'void' in args:
            args.remove('void')
        #prepare input args
        args_in = []
        args_rest = []
        for a in args:
            if not a: continue;
            if a.find('*') < 0:
                pair = a.split(' ')
                if len(pair) != 2:
                    return ("bad format in argument %s of read function %s" % (a, func[0]),{}, {})
                args_in.append([pair[0], pair[1]])
            elif a.find('char *') >= 0 and a.find('char **') < 0:
                if args.index(a) != len(args)-1:
                    return ("para %s must be the last one, in read function %s" % (a, func[0]),{}, {})
                name = a.replace('char *', '')
                args_in.append(['char *', name])
            else:
                args_rest.append(a)
        #prepare returns
        returns_out = []
        for a in args_rest:
            if not a: continue;
            pair = a.replace('*', '', 1).split(' ')  #reserve * for char **
            if len(pair) != 2:
                return ("bad format in argument %s of read function %s" % (a, func[0]),{}, {})
            if pair[1].find('*') >= 0:
                returns_out.append([pair[0]+' *', pair[1].replace('*','')])
            else:
                returns_out.append([pair[0], pair[1]])

        reads[func[0]] = {'Arguments':args_in, 'Returns':returns_out, 'Raw':args}
    patterns["Reads"] = reads

    read_functions_with_doc = find_comments('read', content)
    print read_functions_with_doc
    for func in read_functions_with_doc:
        paras = re.findall(r'@param (\w*)[ ]?[-:]?[ ]?(.*)$', func[0], re.M)
        dict_paras = {}
        for p in paras:
            dict_paras[p[0]] = p[1]

        briefs = re.findall(r'(?!\* @)\* (.*)', func[0], re.M)
        brief = '\n'.join(briefs)
        dict_paras['@brief@'] = brief if brief.strip().strip('\n') else ""
        doc_methods[func[1]] = dict_paras

    ## write functions
    write_functions = re.findall(r'^\s+bool\s+(write_[a-zA-z0-9_]+)\((.*)\).*$', content, re.M)
    print write_functions
    writes = {}
    for func in write_functions:
        args = func[1].split(',')
        args = [x.strip() for x in args]
        if 'void' in args:
            args.remove('void')
        #prepare args in
        args_in = []
        for a in args:
            if not a: continue;
            if a.find('char *') >= 0:
                if a.find('**') >= 0:
                    return ("%s not accepted in function %s" % (a, func[0]),{}, {})
                pair = a.replace('*', '').split(' ')
                if len(pair) != 2:
                    return ("bad format in argument %s of function %s" % (a, func[0]),{}, {})
                if args.index(a) != len(args)-1:
                    return ("para %s must be the last one, in function %s" % (a, func[0]),{}, {})
                args_in.append(['char *', pair[1]])
            elif a.find('*') >= 0:
                return ("%s not accepted in function write_%s" % (a, func[0]),{}, {})
            else:
                pair = a.split(' ')
                if len(pair) != 2:
                    return ("bad format in argument %s of function %s." % (a, func[0]),{}, {})
                args_in.append([pair[0], pair[1]])

        writes[func[0]] = {'Arguments': args_in}
    patterns["Writes"] = writes

    write_functions_with_doc = find_comments('write', content)
    print write_functions_with_doc
    for func in write_functions_with_doc:
        paras = re.findall(r'@param (\w*)[ ]?[-:]?[ ]?(.*)$', func[0], re.M)
        dict_paras = {}
        for p in paras:
            dict_paras[p[0]] = p[1]

        briefs = re.findall(r'(?!\* @)\* (.*)', func[0], re.M)
        brief = '\n'.join(briefs)
        dict_paras['@brief@'] = brief if brief.strip().strip('\n') else ""
        doc_methods[func[1]] = dict_paras

    ## event
    #    DEFINE_EVENT(fire, SULI_EDT_INT);
    event_attachments = re.findall(r'^\s+DEFINE_EVENT\((.*)\s*,\s*(.*)\).*$', content, re.M)
    print event_attachments
    events = {}
    for ev in event_attachments:
        if ev[1] in TYPE_MAP:
            events[ev[0]] = TYPE_MAP[ev[1]]
        else:
            print 'event data type %s not supported' % ev[1]
            sys.exit(1)

    patterns["Events"] = events

    event_attachments_with_doc = find_comments('event', content)
    print event_attachments_with_doc
    for event in event_attachments_with_doc:
        briefs = re.findall(r'(?!\* @)\* (.*)', event[0], re.M)
        brief = '\n'.join(briefs)
        doc_events[event[1]] = brief if brief.strip().strip('\n') else ""

    if len(event_attachments) > 0:
        patterns["HasEvent"] = True
    else:
        patterns["HasEvent"] = False

    ## get last error
    get_last_error_func = re.findall(r'^\s+char\s*\*\s*get_last_error\(\s*\).*$', content, re.M)
    if len(get_last_error_func) > 0:
        patterns["CanGetLastError"] = True
    else:
        patterns["CanGetLastError"] = False

    doc['Methods'] = doc_methods
    doc['Events'] = doc_events

    ## on power on
    on_power_on_func = re.findall(r'^\s+bool\s+on_power_on\(\s*\).*$', content, re.M)
    print on_power_on_func
    if len(on_power_on_func) > 0:
        patterns["HasPowerOnFunc"] = True
    else:
        patterns["HasPowerOnFunc"] = False

    ## on power off
    on_power_off_func = re.findall(r'^\s+bool\s+on_power_off\(\s*\).*$', content, re.M)
    print on_power_off_func
    if len(on_power_off_func) > 0:
        patterns["HasPowerOffFunc"] = True
    else:
        patterns["HasPowerOffFunc"] = False

    return ("OK",patterns, doc)



## main ##

if __name__ == '__main__':

    print __file__
    cur_dir = os.path.split(os.path.realpath(__file__))[0]
    grove_drivers_abs_dir = os.path.abspath(cur_dir + "/grove_drivers")
    grove_database = []
    grove_docs = []
    failed = False
    failed_msg = ""
    grove_id = 0
    for f in os.listdir(grove_drivers_abs_dir):
        full_dir = os.path.join(grove_drivers_abs_dir, f)
        grove_info = {}
        grove_doc = {}
        if os.path.isdir(full_dir):
            print full_dir
            files = parse_one_driver_dir(full_dir)
            class_file = get_class_header_file(files,f)
            if class_file:
                result, patterns, doc = parse_class_header_file(os.path.join(full_dir,class_file))
                if patterns:
                    grove_info['ID'] = grove_id
                    grove_info['IncludePath'] = full_dir.replace(cur_dir, ".")
                    grove_info['Files'] = files
                    grove_info['ClassFile'] = class_file
                    grove_info = dict(grove_info, **patterns)
                    print grove_info
                    grove_database.append(grove_info)
                    grove_doc['ID'] = grove_id
                    grove_doc['Methods'] = doc['Methods']
                    grove_doc['Events'] = doc['Events']
                    grove_doc['GroveName'] = grove_info['GroveName']
                    print grove_doc
                    grove_docs.append(grove_doc)
                    grove_id = grove_id + 1
                else:
                    failed_msg = "ERR: parse class file: %s"%result
                    failed = True
                    break
            else:
                failed_msg = "ERR: can not find class file of %s" % full_dir
                failed = True
                break

    #print grove_database

    print "========="
    print grove_docs
    print ""

    if not failed:
        open("%s/drivers.json"%cur_dir,"w").write(json.dumps(grove_database))
        open("%s/driver_docs.json"%cur_dir,"w").write(json.dumps(grove_docs))
        open("%s/scan_status.json"%cur_dir,"w").write('{"result":"OK", "msg":"scanned %d grove drivers at %s"}' % (len(grove_database), str(datetime.now())))
    else:
        print failed_msg
        open("%s/scan_status.json" % cur_dir,
             "w").write('{"result":"Failed", "msg":"%s"}' % (failed_msg))
        sys.exit(1)

    skip_build_libs = '' if len(sys.argv) < 2 else sys.argv[1]

    if skip_build_libs in ['-k', 'skip']:
        sys.exit(1)

    user_build_dir = cur_dir + '/users_build/local_user_00000000000000000000'

    #os.putenv("SPI_SPEED", "40")
    #os.putenv("SPI_MODE", "QIO")
    #os.putenv("SPI_SIZE_MAP", "6")

    os.system('cd %s;cp -f ../../Makefile.template ./Makefile ' % user_build_dir)


    cmd = 'cd %s;make clean_libs;make libs|tee build.log 2>&1' % (user_build_dir)
    print '---- start to build the prebuilt libs ---'
    print cmd
    os.system(cmd)

    content = open(user_build_dir+"/build.log", 'r').readlines()
    for line in content:
        if line.find("error:") > -1 or line.find("make:") > -1 or line.find("undefined reference to") > -1:
            print line
            sys.exit(1)




