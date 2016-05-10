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

import config as server_config
from build_firmware import *

if __name__ == '__main__':

    build_phase = [1]
    app_num = 'ALL' if len(sys.argv) < 2 else sys.argv[1]
    user_id = "local_user" if len(sys.argv) < 3 else sys.argv[2]
    node_sn = "00000000000000000000" if len(sys.argv) < 4 else sys.argv[3]
    node_name = "esp8266_node" if len(sys.argv) < 5 else sys.argv[4]
    server_ip = "" if len(sys.argv) < 6 else sys.argv[5]

    server_config.ALWAYS_BUILD_FROM_SRC = True

    if not gen_and_build(build_phase, app_num, user_id, node_sn, node_name, server_ip, None, None):
        print get_error_msg()





