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

#dependence: PyYAML (pip install PyYaml)


import os
import sys
import re
import json

import yaml

import config as server_config



# Make the build process verbose
# It's useful when under dev stage, to display the full printing of make process.
# This setting will be overwritten by handlers.py to False
VERBOSE = True


###############################
GEN_DIR = '.'

TYPE_MAP = {
    'int':'TYPE_INT',
    'float':'TYPE_FLOAT',
    'bool':'TYPE_BOOL',
    'uint8_t':'TYPE_UINT8',
    'int8_t':'TYPE_INT8',
    'uint16_t':'TYPE_UINT16',
    'int16_t':'TYPE_INT16',
    'uint32_t':'TYPE_UINT32',
    'int32_t':'TYPE_INT32',
    'const char *':'TYPE_STRING',
    'char *':'TYPE_STRING'
    }

error_msg = ""

def find_grove_in_database (grove_name, sku, json_obj):
    for grove in json_obj:
        #print grove['GroveName']," -- ", grove_name
        #print grove['SKU']," -- ", sku

        if 'SKU' in grove and sku:
            if grove['SKU'] == str(sku):
                return grove
        elif grove['GroveName'] == grove_name.decode( 'unicode-escape' ):
            return grove
    return {}

def find_grove_in_docs_db (grove_id, json_obj):
    for grove in json_obj:
        #print grove['GroveName']," -- ", grove_name
        if grove['ID'] == grove_id:
            return grove
    return {}

def declare_read_vars (arguments, returns):
    result = ""
    for arg in arguments + returns:
        if not arg: continue;
        line = '    %s %s;\r\n' % (arg[0], arg[1])
        result += line
    return result

def in_list_item(needle, list):
    for item in list:
        if needle == item[1]:
            return True
    return False

def build_read_call_args (raw, arguments, returns):
    result = ""
    for arg in raw:
        if not arg: continue;
        arg = arg.strip().split(' ')[1].replace('*', '')
        if in_list_item(arg, arguments):
            item = '%s, ' % arg
        elif in_list_item(arg, returns):
            item = '&%s, ' % arg
        else:
            raise Exception("No way!!!")
        result += item
    return result.rstrip(', ')

def build_read_print (arg_list):
    global error_msg
    result = '        writer_print(TYPE_STRING, "{");\r\n'
    cnt = len(arg_list)
    for i in xrange(cnt):
        if not arg_list[i]: continue;
        t = arg_list[i][0]
        name = arg_list[i][1]

        if t in TYPE_MAP.keys():
            result += '        writer_print(TYPE_STRING, "\\"%s\\":");\r\n' % name
            if TYPE_MAP[t] == "TYPE_STRING":
                result += '        writer_print(TYPE_STRING, "\\"");\r\n'
            result += "        writer_print(%s, %s%s);\r\n" %(TYPE_MAP[t], '&' if t != 'char *' else '', name)
            if TYPE_MAP[t] == "TYPE_STRING":
                result += '        writer_print(TYPE_STRING, "\\"");\r\n'
            if i < (cnt-1):
                result += '        writer_print(TYPE_STRING, ",");\r\n'
        else:
            error_msg = 'arg type %s not supported' % t
            #sys.exit()
            return ""
    result += '        writer_print(TYPE_STRING, "}");\r\n'
    return result

def build_read_unpack_vars (arg_list):
    global error_msg
    result = ""
    for arg in arg_list:
        if not arg: continue;
        t = arg[0]
        name = arg[1]
        result += '    memcpy(&%s, arg_ptr, sizeof(%s)); arg_ptr += sizeof(%s);\r\n' % (name, t, t)
    return result;

def build_read_with_arg (arg_list):
    global error_msg
    result = ""
    for arg in arg_list:
        if not arg:
            continue
        t = arg[0]
        name = arg[1]
        result += '/{%s %s}' % (t,name)
    return result.replace('char * ', 'char *')

def build_reg_read_arg_type (arg_list):
    global error_msg
    result = ""
    length = min(4, len(arg_list))
    for i in xrange(length):
        if not arg_list[i]:
            continue
        t = arg_list[i][0]
        if t in TYPE_MAP.keys():
            result += "    arg_types[%d] = %s;\r\n" %(i, TYPE_MAP[t])
        else:
            error_msg = 'arg type %s not supported' % t
            #sys.exit()
            return ""
    return result

def build_return_values (arg_list):
    result = ""
    for arg in arg_list:
        if not arg:
            continue
        item = '%s %s, ' % (arg[0], arg[1])
        result += item
    return result.rstrip(', ').replace('char * ', 'char *')

def declare_write_vars (arg_list):
    result = ""
    for arg in arg_list:
        if not arg:
            continue
        item = '    %s %s;\r\n' % (arg[0], arg[1]) 
        result += item
    return result

def build_write_unpack_vars (arg_list):
    global error_msg
    result = ""
    for arg in arg_list:
        if not arg:
            continue
        t = arg[0]
        name = arg[1]
        result += '    memcpy(&%s, arg_ptr, sizeof(%s)); arg_ptr += sizeof(%s);\r\n' % (name, t, t)
    return result

def build_write_call_args (arg_list):
    result = ""
    for arg in arg_list:
        if not arg:
            continue
        result += arg[1]
        result += ','
    return result.rstrip(',')

def build_write_args (arg_list):
    result = ""
    #arg_list = [arg for arg in arg_list if arg.find("*")<0]  #find out the ones havent "*"
    for arg in arg_list:
        if not arg:
            continue
        result += '/{%s %s}' % (arg[0], arg[1])

    return result.replace('char * ', 'char *')

def build_reg_write_arg_type (arg_list):
    global error_msg
    result = ""
    length = min(4, len(arg_list))
    for i in xrange(length):
        if not arg_list[i]:
            continue
        t = arg_list[i][0]
        name = arg_list[i][1]
        if t in TYPE_MAP.keys():
            result += "    arg_types[%d] = %s;\r\n" %(i, TYPE_MAP[t])
        else:
            error_msg = 'arg type %s not supported' % t
            #sys.exit()
            return ""
    return result


def gen_wrapper_registration (instance_name, info, arg_list):
    global error_msg

    instance_name = instance_name.replace(" ","_");
    grove_name = info["IncludePath"].replace("./grove_drivers/", "").lower()
    gen_header_file_name = grove_name+"_gen.h"
    gen_cpp_file_name = grove_name+"_gen.cpp"
    fp_wrapper_h = open(os.path.join(GEN_DIR, gen_header_file_name),'w')
    fp_wrapper_cpp = open(os.path.join(GEN_DIR, gen_cpp_file_name), 'w')
    str_main_include = ""
    str_main_declare = ""
    str_reg_method = ""
    str_wellknown = ""
    error_msg = ""

    #leading part
    fp_wrapper_h.write('#include "%s"\r\n\r\n' % info['ClassFile'])
    fp_wrapper_cpp.write('#include "%s"\r\n#include "%s"\r\n#include "%s"\r\n\r\n' % (gen_header_file_name, "rpc_server.h", "rpc_stream.h"))
    str_main_include += '#include "%s"\r\n' % gen_header_file_name
    args_in_string = ""
    for arg in info['ConstructArgList']:
        arg_name = arg.strip().split(' ')[1]
        if arg_name in arg_list.keys():
            args_in_string += ","
            args_in_string += str(arg_list[arg_name])
        else:
            error_msg = "ERR: no construct arg name in config file matchs %s" % arg_name
            #sys.exit()
            return (False, "", "", "", "")

    str_main_declare += 'extern %s *%s;\r\n' % (info['ClassName'], instance_name+"_ins")

    str_reg_method += '\r\n'
    str_reg_method += '    //%s\r\n'%instance_name
    str_reg_method += '    %s = new %s(%s);\r\n' % (instance_name+"_ins", info['ClassName'], args_in_string.lstrip(","))


    #loop part
    #read functions

    for fun in info['Reads'].items():
        fp_wrapper_h.write('bool __%s_%s(void *class_ptr, char *method_name, void *input_pack);\r\n' % (grove_name, fun[0]))

        fp_wrapper_cpp.write('bool __%s_%s(void *class_ptr, char *method_name, void *input_pack)\r\n' % (grove_name, fun[0]))
        fp_wrapper_cpp.write('{\r\n')
        fp_wrapper_cpp.write('    %s *grove = (%s *)class_ptr;\r\n' % (info['ClassName'], info['ClassName']))
        fp_wrapper_cpp.write('    uint8_t *arg_ptr = (uint8_t *)input_pack;\r\n')
        fp_wrapper_cpp.write('%s\r\n'%declare_read_vars(fun[1]['Arguments'], fun[1]['Returns']))
        fp_wrapper_cpp.write(build_read_unpack_vars(fun[1]['Arguments']))
        fp_wrapper_cpp.write('\r\n')
        fp_wrapper_cpp.write('    if(grove->%s(%s))\r\n'%(fun[0],build_read_call_args(fun[1]['Raw'], fun[1]['Arguments'], fun[1]['Returns'])))
        fp_wrapper_cpp.write('    {\r\n')
        fp_wrapper_cpp.write(build_read_print(fun[1]['Returns']))
        fp_wrapper_cpp.write('        return true;\r\n')
        fp_wrapper_cpp.write('    }else\r\n')
        fp_wrapper_cpp.write('    {\r\n')
        if info['CanGetLastError']:
            fp_wrapper_cpp.write('        writer_print(TYPE_STRING, "\\"");\r\n')
            fp_wrapper_cpp.write('        writer_print(TYPE_STRING, grove->get_last_error());\r\n')
            fp_wrapper_cpp.write('        writer_print(TYPE_STRING, "\\"");\r\n')
        else:
            fp_wrapper_cpp.write('        writer_print(TYPE_STRING, "null");\r\n')
        fp_wrapper_cpp.write('        return false;\r\n')
        fp_wrapper_cpp.write('    }\r\n')
        fp_wrapper_cpp.write('}\r\n\r\n')

        str_reg_method += '    memset(arg_types, TYPE_NONE, MAX_INPUT_ARG_LEN);\r\n'
        str_reg_method += build_reg_read_arg_type(fun[1]['Arguments'])
        str_reg_method += '    rpc_server_register_method("%s", "%s", METHOD_READ, %s, %s, arg_types);\r\n' % \
                          (instance_name, fun[0].replace('read_',''), '__'+grove_name+'_'+fun[0], instance_name+"_ins")

        str_wellknown += '    writer_block_print(TYPE_STRING, "\\"GET " OTA_SERVER_URL_PREFIX "/node/%s/%s%s -> %s\\",");\r\n' % \
                            (instance_name, fun[0].replace('read_',''), build_read_with_arg(fun[1]['Arguments']), \
                             build_return_values(fun[1]['Returns']))

    str_reg_method += '\r\n'

    #write functions
    for fun in info['Writes'].items():
        fp_wrapper_h.write('bool __%s_%s(void *class_ptr, char *method_name, void *input_pack);\r\n' % (grove_name, fun[0]))

        fp_wrapper_cpp.write('bool __%s_%s(void *class_ptr, char *method_name, void *input_pack)\r\n' % (grove_name, fun[0]))
        fp_wrapper_cpp.write('{\r\n')
        fp_wrapper_cpp.write('    %s *grove = (%s *)class_ptr;\r\n' % (info['ClassName'], info['ClassName']))
        fp_wrapper_cpp.write('    uint8_t *arg_ptr = (uint8_t *)input_pack;\r\n')
        fp_wrapper_cpp.write('%s\r\n' % declare_write_vars(fun[1]['Arguments']))
        fp_wrapper_cpp.write(build_write_unpack_vars(fun[1]['Arguments']))
        fp_wrapper_cpp.write('\r\n')
        fp_wrapper_cpp.write('    if(grove->%s(%s))\r\n'%(fun[0],build_write_call_args(fun[1]['Arguments'])))
        fp_wrapper_cpp.write('    {\r\n')
        fp_wrapper_cpp.write('        writer_print(TYPE_STRING, "\\"OK\\"");\r\n')
        fp_wrapper_cpp.write('        return true;\r\n')
        fp_wrapper_cpp.write('    }\r\n')
        fp_wrapper_cpp.write('    else\r\n')
        fp_wrapper_cpp.write('    {\r\n')
        if info['CanGetLastError']:
            fp_wrapper_cpp.write('        writer_print(TYPE_STRING, "\\"");\r\n')
            fp_wrapper_cpp.write('        writer_print(TYPE_STRING, grove->get_last_error());\r\n')
            fp_wrapper_cpp.write('        writer_print(TYPE_STRING, "\\"");\r\n')
        else:
            fp_wrapper_cpp.write('        writer_print(TYPE_STRING, "null");\r\n')
        fp_wrapper_cpp.write('        return false;\r\n')
        fp_wrapper_cpp.write('    }\r\n')
        fp_wrapper_cpp.write('}\r\n\r\n')

        str_reg_method += '    memset(arg_types, TYPE_NONE, MAX_INPUT_ARG_LEN);\r\n'
        str_reg_method += build_reg_write_arg_type(fun[1]['Arguments'])
        str_reg_method += '    rpc_server_register_method("%s", "%s", METHOD_WRITE, %s, %s, arg_types);\r\n' % \
                          (instance_name, fun[0].replace('write_',''), '__'+grove_name+'_'+fun[0], instance_name+"_ins")

        str_wellknown += '    writer_block_print(TYPE_STRING, "\\"POST " OTA_SERVER_URL_PREFIX "/node/%s/%s%s\\",");\r\n' % \
                            (instance_name, fun[0].replace('write_',''), build_write_args(fun[1]['Arguments']))


    # event attachment
    if info['HasEvent']:
        for ev in info['Events']:
            str_reg_method += '\r\n'
            str_reg_method += '    event = %s->attach_event_reporter_for_%s(rpc_server_event_report);\r\n' % (instance_name+"_ins", ev)
            #str_reg_method += '    event->event_name="%s";\r\n' % (ev)
            str_wellknown += '    writer_block_print(TYPE_STRING, "\\"Event %s %s\\",");\r\n' % (instance_name, ev)

    # on power on function
    if info['HasPowerOnFunc']:
        fp_wrapper_h.write('bool __%s_on_power_on(void *class_ptr, char *method_name, void *input_pack);\r\n' % grove_name)

        fp_wrapper_cpp.write('bool __%s_on_power_on(void *class_ptr, char *method_name, void *input_pack)\r\n' % grove_name)
        fp_wrapper_cpp.write('{\r\n')
        fp_wrapper_cpp.write('    %s *grove = (%s *)class_ptr;\r\n' % (info['ClassName'], info['ClassName']))
        fp_wrapper_cpp.write('    return grove->on_power_on();\r\n')
        fp_wrapper_cpp.write('}\r\n\r\n')

        str_reg_method += '    memset(arg_types, TYPE_NONE, MAX_INPUT_ARG_LEN);\r\n'
        str_reg_method += '    rpc_server_register_method("%s", "?poweron", METHOD_INTERNAL, %s, %s, arg_types);\r\n' % \
                          (instance_name, '__' + grove_name + '_on_power_on', instance_name + "_ins")

    # on power on function
    if info['HasPowerOffFunc']:
        fp_wrapper_h.write('bool __%s_on_power_off(void *class_ptr, char *method_name, void *input_pack);\r\n' % grove_name)

        fp_wrapper_cpp.write('bool __%s_on_power_off(void *class_ptr, char *method_name, void *input_pack)\r\n' % grove_name)
        fp_wrapper_cpp.write('{\r\n')
        fp_wrapper_cpp.write('    %s *grove = (%s *)class_ptr;\r\n' % (info['ClassName'], info['ClassName']))
        fp_wrapper_cpp.write('    return grove->on_power_off();\r\n')
        fp_wrapper_cpp.write('}\r\n\r\n')

        str_reg_method += '    memset(arg_types, TYPE_NONE, MAX_INPUT_ARG_LEN);\r\n'
        str_reg_method += '    rpc_server_register_method("%s", "?poweroff", METHOD_INTERNAL, %s, %s, arg_types);\r\n' % \
                          (instance_name, '__' + grove_name + '_on_power_off', instance_name + "_ins")


    fp_wrapper_h.close()
    fp_wrapper_cpp.close()
    if error_msg:
        return (False, "","","","")

    return (True, str_main_include, str_main_declare, str_reg_method, str_wellknown)



def gen_and_build (build_phase, app_num, user_id, node_sn, node_name, server_ip, json_drivers, json_connections):
    global error_msg
    global GEN_DIR
    global VERBOSE

    ###the default: wio link
    func_key = 0
    status_led = 2
    grove_power_switch = 15
    flash_map = '6'
    flash_spi_speed = '40'
    flash_spi_mode = 'QIO'
    
    ###generate rpc wrapper and registration files

    cur_dir = os.path.split(os.path.realpath(__file__))[0]
    user_build_dir = cur_dir + '/users_build/' + user_id + '_' + node_sn

    if not os.path.exists(user_build_dir):
        os.mkdir(user_build_dir)


    if not json_drivers:
        try:
            f = open('%s/drivers.json' % cur_dir, 'r')
            json_drivers = json.load(f)
            f.close()
        except Exception,e:
            error_msg = str(e)
            #return False


    config = {}
    #try to find connection_config.json
    if not json_connections and os.path.exists('%s/connection_config.json' % user_build_dir):
        try:
            f = open('%s/connection_config.json' % user_build_dir, 'r')
            json_connections = json.load(f)
            f.close()
        except Exception,e:
            error_msg = str(e)
            #return False

    if json_connections:
        try:
            f = open('%s/boards.json' % cur_dir, 'r')
            json_boards = json.load(f)
            f.close()

            board_name = json_connections['board_name']
            board = None
            for b in json_boards:
                if b['board_name'] == board_name:
                    board = b
                    break
            if not board:
                error_msg = "Can not find board %s" % board_name
                return False

            interfaces = board['interfaces']
            board_builtin = board['board_builtin']

            func_key = board_builtin['FUNCTION_KEY']
            status_led = board_builtin['STATUS_LED']
            grove_power_switch = board_builtin['GROVE_POWER_SWITCH']
            flash_map = board['board_flash_map']
            flash_spi_speed = board['board_flash_spi_speed']
            flash_spi_mode = board['board_flash_spi_mode']

            my_connections = json_connections['connections']
            for c in my_connections:
                item = {}
                item['sku'] = c['sku']
                item['construct_arg_list'] = interfaces[c['port']]
                grove = find_grove_in_database("", c['sku'], json_drivers)
                if grove:
                    grove_ins = grove['ClassName'] + c['port']
                    config[grove_ins] = item
                else:
                    error_msg = 'Can not find grove sku %s in drivers database.' % c['sku']
                    return False

            with open('%s/connection_config.yaml' % user_build_dir, 'w') as f:
                yaml.safe_dump(config, f)


        except Exception,e:
            error_msg = str(e)
            #return False


    #or fall back to the the old version
    #try to find the old connection_config.yaml
    if not config:
        try:
            f = open('%s/connection_config.yaml' % user_build_dir, 'r')
            config = yaml.load(f)
            f.close()
        except Exception,e:
            error_msg = str(e)
            return False

    print config

    GEN_DIR = user_build_dir


    fp_reg_cpp = open(os.path.join(user_build_dir, "rpc_server_registration.cpp"),'w')
    fp_main_h = open(os.path.join(user_build_dir, "Main.h"),'w')

    str_main_include = ""
    str_main_declare = ""
    str_reg_method = ""
    str_reg_event = ""
    str_well_known = ""
    grove_list = ""

    if config:
        for grove_instance_name in config.keys():
            if 'sku' in config[grove_instance_name]:
                _sku = config[grove_instance_name]['sku']
            else:
                _sku = None

            if 'name' in config[grove_instance_name]:
                _name = config[grove_instance_name]['name']
            else:
                _name = None

            grove = find_grove_in_database(_name, _sku, json_drivers)
            if grove:
                ret, inc, dec, method, wellknown = gen_wrapper_registration(grove_instance_name, grove, config[grove_instance_name]['construct_arg_list'])
                if(ret == False):
                    return False
                str_main_include += inc
                str_main_declare += dec
                str_reg_method  += method
                str_well_known  += wellknown
                grove_list += (grove["IncludePath"].replace("./grove_drivers/", " "))
            else:
                error_msg = "can not find %s in database"%grove_instance_name
                return False

        str_well_known = str_well_known[:-6]+'");\r\n'

    fp_reg_cpp.write('#include "suli2.h"\r\n')
    fp_reg_cpp.write('#include "rpc_server.h"\r\n')
    fp_reg_cpp.write('#include "rpc_stream.h"\r\n')
    fp_reg_cpp.write('#include "Main.h"\r\n\r\n')
    fp_reg_cpp.write(str_main_declare.replace('extern ',''))
    fp_reg_cpp.write('\r\n')
    fp_reg_cpp.write('void rpc_server_register_resources()\r\n')
    fp_reg_cpp.write('{\r\n')
    fp_reg_cpp.write('    uint8_t arg_types[MAX_INPUT_ARG_LEN];\r\n')
    fp_reg_cpp.write('    EVENT_T *event;\r\n')
    fp_reg_cpp.write('    \r\n')
    fp_reg_cpp.write(str_reg_method)
    fp_reg_cpp.write('}\r\n')
    fp_reg_cpp.write('\r\n')
    fp_reg_cpp.write('void print_well_known()\r\n')
    fp_reg_cpp.write('{\r\n')
    fp_reg_cpp.write('    writer_print(TYPE_STRING, "{\\"well_known\\":");\r\n')
    fp_reg_cpp.write('    writer_print(TYPE_STRING, "[");\r\n')
    fp_reg_cpp.write(str_well_known)
    fp_reg_cpp.write('    writer_print(TYPE_STRING, "]}");\r\n')
    fp_reg_cpp.write('}\r\n')

    fp_reg_cpp.close()

    fp_main_h.write('#ifndef __MAIN_H__\r\n')
    fp_main_h.write('#define __MAIN_H__\r\n')
    fp_main_h.write('#include "suli2.h"\r\n')
    fp_main_h.write(str_main_include)
    fp_main_h.write('\r\n')
    fp_main_h.write(str_main_declare)
    fp_main_h.write('#endif\r\n')
    fp_main_h.close()


    ### only generate the source code files
    if 2 not in build_phase:
        return True


    ### make
    grove_list = '%s' % grove_list.lstrip(" ")



    if not os.path.exists('%s/Main.cpp' % user_build_dir):
        os.system('cd %s;cp -f ../../Main.cpp.template ./Main.cpp ' % user_build_dir)
    if not os.path.exists('%s/Makefile' % user_build_dir):
        os.system('cd %s;cp -f ../../Makefile.template ./Makefile ' % user_build_dir)

    node_name = node_name.replace('"', "'").replace('\\', '\\\\').replace('$', '$$').replace('&', '\\&').replace('`', '\\`')
    os.putenv("SPI_SPEED",str(flash_spi_speed))
    os.putenv("SPI_MODE",str(flash_spi_mode))
    os.putenv("SPI_SIZE_MAP",str(flash_map))
    os.putenv("GROVES",grove_list)
    os.putenv("NODE_NAME",node_name)

    if server_ip and re.match(r'\d+,\d+,\d+,\d+', server_ip):
        os.putenv("SERVER_IP", server_ip)

    if server_config.ALWAYS_BUILD_FROM_SRC:
        os.putenv("BUILD_FROM_SRC", "1")
    else:
        os.putenv("BUILD_FROM_SRC", "")

    os.putenv("FUNCTION_KEY", str(func_key))
    os.putenv("STATUS_LED", str(status_led))
    os.putenv("GROVE_POWER_SWITCH", str(grove_power_switch))



    if app_num in [1,'1','ALL']:
        os.putenv("APP","1")

        if VERBOSE:
            cmd = 'cd %s;make clean;make 2>&1|tee build.log' % (user_build_dir)
        else:
            cmd = 'cd %s;make clean;make > build.log 2>&1' % (user_build_dir)
        print '---- start to build app 1 ---'
        print cmd
        os.system(cmd)

        with open(user_build_dir + "/build.log", 'r') as f_build_log:
            content = f_build_log.readlines()
            for line in content:
                if line.find("error:") > -1 or line.find("make:") > -1 or line.find("undefined reference to") > -1:
                    if VERBOSE:
                        print "\r\n".join(content)
                    error_msg = line
                    return False

    if app_num in [2, '2', 'ALL']:
        os.putenv("APP","2")

        if VERBOSE:
            cmd = 'cd %s;make clean;make 2>&1|tee build.log' % (user_build_dir)
        else:
            cmd = 'cd %s;make clean;make > build.log 2>&1' % (user_build_dir)
        print '---- start to build app 2 ---'
        print cmd
        os.system(cmd)

        with open(user_build_dir + "/build.log", 'r') as f_build_log:
            content = f_build_log.readlines()
            for line in content:
                if line.find("error:") > -1 or line.find("make:") > -1 or line.find("undefined reference to") > -1:
                    if VERBOSE:
                        print "\r\n".join(content)
                    error_msg = line
                    return False

    os.system('cd %s;rm -rf *.S;rm -rf *.dump;rm -rf *.d;rm -rf *.o'%user_build_dir)

    return True

def set_build_verbose(vb):
    global VERBOSE
    VERBOSE = vb

def get_error_msg ():
    global error_msg
    return error_msg

if __name__ == '__main__':

    build_phase = [1,2]
    app_num = 'ALL' if len(sys.argv) < 2 else sys.argv[1]
    user_id = "local_user" if len(sys.argv) < 3 else sys.argv[2]
    node_sn = "00000000000000000000" if len(sys.argv) < 4 else sys.argv[3]
    node_name = "esp8266_node" if len(sys.argv) < 5 else sys.argv[4]
    server_ip = "" if len(sys.argv) < 6 else sys.argv[5]

    if not gen_and_build(build_phase, app_num, user_id, node_sn, node_name, server_ip, None, None):
        print get_error_msg()



