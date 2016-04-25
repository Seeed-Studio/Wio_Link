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

#   Dependences: pip install tornado
#                pip install PyJWT
#                pip install pycrypto
#                pip install tornado-cors

from datetime import timedelta
import socket
import json
import sqlite3 as lite
import re
import hashlib
import hmac
import binascii
from Crypto.Cipher import AES
from Crypto import Random

from tornado.httpserver import HTTPServer
from tornado.tcpserver import TCPServer
from tornado import ioloop
from tornado import gen
from tornado import iostream
from tornado import web
from tornado.options import define, options
from tornado.log import *
from tornado.concurrent import Future
from tornado.queues import Queue
from tornado.locks import Semaphore, Condition

import config as server_config
from handlers import *

BS = AES.block_size
pad = lambda s: s if (len(s) % BS == 0) else (s + (BS - len(s) % BS) * chr(0) )
unpad = lambda s : s.rstrip(chr(0))

PENDING_REQ_CNT = 10

class DeviceConnection(object):

    state_waiters = {}
    state_happened = {}

    def __init__ (self, device_server, stream, address):
        self.fw_version = 0.0
        self.recv_msg_cond = Condition()
        self.recv_msg = {}
        self.send_msg_sem = Semaphore(1)
        self.pending_request_cnt = 0
        self.device_server = device_server
        self.stream = stream
        self.address = address
        self.stream.set_nodelay(True)
        self.timeout_handler_onlinecheck = None
        self.timeout_handler_offline = None
        self.killed = False
        self.sn = ""
        self.private_key = ""
        self.node_id = 0
        self.iv = None
        self.cipher_down = None
        self.cipher_up = None

        #self.state_waiters = []
        #self.state_happened = []

        self.event_waiters = []
        self.event_happened = []

        self.ota_ing = False
        self.ota_notify_done_future = None
        self.post_ota = False
        self.online_status = True

    @gen.coroutine
    def secure_write (self, data):
        if self.cipher_down:
            cipher_text = self.cipher_down.encrypt(pad(data))
            yield self.stream.write(cipher_text)

    @gen.coroutine
    def wait_hello (self):
        try:
            self._wait_hello_future = self.stream.read_bytes(64) #read 64bytes: 32bytes SN + 32bytes signature signed with private key
            str1 = yield gen.with_timeout(timedelta(seconds=10), self._wait_hello_future,
                                         io_loop=ioloop.IOLoop.current())
            self.idle_time = 0  #reset the idle time counter

            if len(str1) != 64:
                self.stream.write("sorry\r\n")
                yield gen.sleep(0.1)
                self.kill_myself()
                gen_log.debug("receive length != 64")
                raise gen.Return(100) # length not match 64

            if re.match(r'@\d\.\d', str1[0:4]):
                #new version firmware
                self._wait_hello_future = self.stream.read_bytes(4) #read another 4bytes
                str2 = yield gen.with_timeout(timedelta(seconds=10), self._wait_hello_future, io_loop=ioloop.IOLoop.current())

                self.idle_time = 0  #reset the idle time counter

                if len(str2) != 4:
                    self.stream.write("sorry\r\n")
                    yield gen.sleep(0.1)
                    self.kill_myself()
                    gen_log.debug("receive length != 68")
                    raise gen.Return(100) # length not match 64

                str1 += str2
                self.fw_version = float(str1[1:4])
                sn = str1[4:36]
                sig = str1[36:68]
            else:
                #for version < 1.1
                sn = str1[0:32]
                sig = str1[32:64]

            gen_log.info("accepted sn: %s @fw_version %.1f" % (sn, self.fw_version))

            #query the sn from database
            node = None
            cur = self.device_server.cur
            cur.execute('select * from nodes where node_sn="%s"'%sn)
            rows = cur.fetchall()
            if len(rows) > 0:
                node = rows[0]

            if not node:
                self.stream.write("sorry\r\n")
                yield gen.sleep(0.1)
                self.kill_myself()
                gen_log.info("node sn not found")
                raise gen.Return(101) #node not found

            key = node['private_key']
            key = key.encode("ascii")

            sig0 = hmac.new(key, msg=sn, digestmod=hashlib.sha256).digest()
            gen_log.debug("sig:     "+ binascii.hexlify(sig))
            gen_log.debug("sig calc:"+ binascii.hexlify(sig0))

            if sig0 == sig:
                #send IV + AES Key
                self.sn = sn
                self.private_key = key
                self.node_id = str(node['node_id'])
                gen_log.info("valid hello packet from node %s" % self.node_id)
                #remove the junk connection of the same sn
                ioloop.IOLoop.current().add_callback(self.device_server.remove_junk_connection, self)
                #init aes
                self.iv = Random.new().read(AES.block_size)
                self.cipher_down = AES.new(key, AES.MODE_CFB, self.iv, segment_size=128)

                if self.fw_version > 1.0:
                    self.cipher_up = AES.new(key, AES.MODE_CFB, self.iv, segment_size=128)
                else:
                    #for old version
                    self.cipher_up = self.cipher_down

                cipher_text = self.iv + self.cipher_down.encrypt(pad("hello"))
                gen_log.debug("cipher text: "+ cipher_text.encode('hex'))
                self.stream.write(cipher_text)
                raise gen.Return(0)
            else:
                self.stream.write("sorry\r\n")
                yield gen.sleep(0.1)
                self.kill_myself()
                gen_log.error("signature not match: %s %s" % (sig, sig0))
                raise gen.Return(102) #sig not match
        except gen.TimeoutError:
            self.kill_myself()
            raise gen.Return(1)
        except iostream.StreamClosedError:
            self.kill_myself()
            raise gen.Return(2)

        #ioloop.IOLoop.current().add_future(self._serving_future, lambda future: future.result())

    @gen.coroutine
    def _loop_reading_input (self):
        line = ""
        piece = ""
        while not self.killed:
            msg = ""
            try:
                msg = yield self.stream.read_bytes(16)
                msg = unpad(self.cipher_up.decrypt(msg))
                line += msg

                while line.find('\r\n') > -1:
                    #reset the timeout
                    if self.timeout_handler_onlinecheck:
                        ioloop.IOLoop.current().remove_timeout(self.timeout_handler_onlinecheck)
                    self.timeout_handler_onlinecheck = ioloop.IOLoop.current().call_later(60, self._online_check)

                    if self.timeout_handler_offline:
                        ioloop.IOLoop.current().remove_timeout(self.timeout_handler_offline)
                    self.timeout_handler_offline = ioloop.IOLoop.current().call_later(70, self._callback_when_offline)

                    index = line.find('\r\n')
                    piece = line[:index+2]
                    line = line[index+2:]
                    piece = piece.strip("\r\n")

                    if piece in ['##ALIVE##']:
                        gen_log.info('Node %s alive on %s channel!' % (self.node_id, self.device_server.role))
                        continue

                    json_obj = json.loads(piece)
                    gen_log.info('Node %s recv json on %s channel' % (self.node_id, self.device_server.role))
                    gen_log.debug('%s' % str(json_obj))

                    try:
                        state = None
                        event = None
                        if json_obj['msg_type'] == 'online_status':
                            if json_obj['msg'] in ['1',1,True]:
                                self.online_status = True
                            else:
                                self.online_status = False
                            continue
                        elif json_obj['msg_type'] == 'ota_trig_ack':
                            state = ('going', 'Node has been notified...')
                            self.ota_ing = True
                            if self.ota_notify_done_future:
                                self.ota_notify_done_future.set_result(1)
                        elif json_obj['msg_type'] == 'ota_status':
                            if json_obj['msg'] == 'started':
                                state = ('going', 'Downloading the firmware...')
                            else:
                                state = ('error', 'Failed to start the downloading.')
                                self.post_ota = True
                        elif json_obj['msg_type'] == 'ota_result':
                            if json_obj['msg'] == 'success':
                                state = ('done', 'Firmware updated.')
                            else:
                                state = ('error', 'Update failed. Please reboot the node and retry.')
                            self.post_ota = True
                            self.ota_ing = False
                        elif json_obj['msg_type'] == 'event':
                            event = json_obj
                            event.pop('msg_type')
                        gen_log.debug("state: ")
                        gen_log.debug(state)
                        gen_log.debug("event: ")
                        gen_log.debug(event)
                        if state:
                            #print self.state_waiters
                            #print self.state_happened
                            if self.state_waiters and self.sn in self.state_waiters and len(self.state_waiters[self.sn]) > 0:
                                f = self.state_waiters[self.sn].pop(0)
                                f.set_result(state)
                                if len(self.state_waiters[self.sn]) == 0:
                                    del self.state_waiters[self.sn]
                            elif self.state_happened and self.sn in self.state_happened:
                                self.state_happened[self.sn].append(state)
                            else:
                                self.state_happened[self.sn] = [state]
                        elif event:
                            if len(self.event_waiters) == 0:
                                self.event_happened.append(event)
                            else:
                                for future in self.event_waiters:
                                    future.set_result(event)
                                self.event_waiters = []
                        else:
                            self.recv_msg = json_obj
                            self.recv_msg_cond.notify()
                            yield gen.moment
                    except Exception,e:
                        gen_log.warn("Node %s: %s" % (self.node_id ,str(e)))

            except iostream.StreamClosedError:
                gen_log.error("StreamClosedError when reading from node %s" % self.node_id)
                self.kill_myself()
                return
            except ValueError:
                gen_log.warn("Node %s: %s can not be decoded into json" % (self.node_id, piece))
            except Exception,e:
                gen_log.error("Node %s: %s" % (self.node_id ,str(e)))
                self.kill_myself()
                return

            yield gen.moment

    @gen.coroutine
    def _online_check (self):
        gen_log.info("heartbeat sent to node %s on %s channel" % (self.node_id, self.device_server.role))
        try:
            yield self.secure_write("##PING##")
            if self.timeout_handler_onlinecheck:
                ioloop.IOLoop.current().remove_timeout(self.timeout_handler_onlinecheck)
            self.timeout_handler_onlinecheck = ioloop.IOLoop.current().call_later(3, self._online_check)
        except iostream.StreamClosedError:
            gen_log.error("StreamClosedError when send ping to node %s" % self.node_id)
            if self.timeout_handler_offline:
                ioloop.IOLoop.current().remove_timeout(self.timeout_handler_offline)
            self.kill_myself()

    def _callback_when_offline(self):
        gen_log.error("no answer from node %s, kill" % self.node_id)
        self.kill_myself()


    @gen.coroutine
    def start_serving (self):
        ret = yield self.wait_hello()
        if ret == 0:
            #gen_log.info("waited hello")
            pass
        elif ret == 1:
            gen_log.info("timeout waiting hello, kill this connection")
            return
        elif ret == 2:
            gen_log.info("connection is closed by client")
            return
        elif ret >= 100:
            return

        ## loop reading the stream input
        self._loop_reading_input()

        if self.timeout_handler_onlinecheck:
            ioloop.IOLoop.current().remove_timeout(self.timeout_handler_onlinecheck)
        self.timeout_handler_onlinecheck = ioloop.IOLoop.current().call_later(60, self._online_check)

        if self.timeout_handler_offline:
            ioloop.IOLoop.current().remove_timeout(self.timeout_handler_offline)
        self.timeout_handler_offline = ioloop.IOLoop.current().call_later(70, self._callback_when_offline)


    def kill_myself (self):
        if self.killed:
            return
        self.sn = ""
        ioloop.IOLoop.current().add_callback(self.device_server.remove_connection, self)
        self.stream.close()
        self.killed = True

    def kill_by_server(self):
        self.send_msg_sem.release()
        self.stream.close()

        if self.timeout_handler_onlinecheck:
            ioloop.IOLoop.current().remove_timeout(self.timeout_handler_onlinecheck)

        if self.timeout_handler_offline:
            ioloop.IOLoop.current().remove_timeout(self.timeout_handler_offline)


    @gen.coroutine
    def submit_cmd (self, cmd):
        yield self.send_msg_sem.acquire()
        try:
            yield self.secure_write(cmd)
        finally:
            self.send_msg_sem.release()

    @gen.coroutine
    def submit_and_wait_resp (self, cmd, target_resp, timeout_sec=5):

        self.pending_request_cnt += 1
        if self.pending_request_cnt > PENDING_REQ_CNT:
            self.pending_request_cnt = PENDING_REQ_CNT
            gen_log.warn('Node %s: request too fast' % self.node_id)
            raise gen.Return((False, {"status":400, "msg":"request too fast"}))

        yield self.send_msg_sem.acquire()
        try:
            yield self.secure_write(cmd)
            ok = yield self.recv_msg_cond.wait(timeout=timedelta(seconds=timeout_sec))
            if not ok:
                gen_log.error("timeout when waiting response from Node %s" % self.node_id)
                raise gen.Return((False, {"status":408, "msg":"timeout when waiting response from Node %s" % self.node_id}))

            msg = self.recv_msg
            if msg['msg_type'] == target_resp:
                if msg['msg_type'] == 'resp_post' and 'status' not in msg:
                    msg['msg'] = None
                del msg['msg_type']
                raise gen.Return((True, msg))
            else:
                raise gen.Return((False, {"status":500, "msg":"unexpected error 1"}))
        except gen.Return:
            raise
        except Exception,e:
            gen_log.error(e)
            raise gen.Return((False, {"status":500, "msg":"Node %s: %s" % (self.node_id, str(e))}))
        finally:
            self.send_msg_sem.release()  #inc the semaphore value to 1
            self.pending_request_cnt -= 1


class DeviceServer(TCPServer):

    accepted_xchange_conns = []
    accepted_ota_conns = []

    def __init__ (self, db_conn, cursor, role):
        self.conn = db_conn
        self.cur = cursor
        self.role = role
        TCPServer.__init__(self)


    def handle_stream(self, stream, address):
        conn = DeviceConnection(self, stream,address)

        if self.role == 'ota':
            self.accepted_ota_conns.append(conn)
            gen_log.info("%s device server accepted conns: %d"% (self.role, len(self.accepted_ota_conns)))
        else:
            self.accepted_xchange_conns.append(conn)
            gen_log.info("%s device server accepted conns: %d"% (self.role, len(self.accepted_xchange_conns)))

        conn.start_serving()

    def remove_connection (self, conn):
        gen_log.info("%s device server will remove connection: %s" % (self.role, str(conn)))
        try:
            if self.role == 'ota':
                self.accepted_ota_conns.remove(conn)
            else:
                self.accepted_xchange_conns.remove(conn)
            del conn
        except:
            pass

    def remove_junk_connection (self, conn):
        try:
            connections = self.accepted_xchange_conns
            if self.role == 'ota':
                connections = self.accepted_ota_conns
            for c in connections:
                if c.sn == conn.sn and c != conn :
                    c.killed = True
                    c.kill_by_server()
                    #clear waiting futures
                    gen_log.info("%s device server removed one junk connection of same sn: %s"% (self.role, c.sn))
                    connections.remove(c)
                    del c
                    break
        except Exception,e:
            gen_log.error(e)



class myApplication(web.Application):

    def __init__(self,db_conn,cursor):
        handlers = [
        (r"/v1[/]?", IndexHandler),
        (r"/v1/test[/]?", TestHandler),
        (r"/v1/user/create[/]?", UserCreateHandler),
        (r"/v1/user/changepassword[/]?", UserChangePasswordHandler),
        (r"/v1/user/retrievepassword[/]?", UserRetrievePasswordHandler),
        (r"/v1/user/login[/]?", UserLoginHandler),
        (r"/v1/scan/drivers[/]?", DriversHandler),
        (r"/v1/scan/status[/]?", DriversStatusHandler),
        (r"/v1/boards/list[/]?", BoardsListHandler),
        (r"/v1/nodes/create[/]?", NodeCreateHandler),
        (r"/v1/nodes/list[/]?", NodeListHandler, dict(conns=DeviceServer.accepted_ota_conns)),
        (r"/v1/nodes/rename[/]?", NodeRenameHandler),
        (r"/v1/nodes/delete[/]?", NodeDeleteHandler),
        (r"/v1/node/(?!event|config|resources|setting|function)(.+)", NodeReadWriteHandler, dict(conns=DeviceServer.accepted_xchange_conns, state_waiters=DeviceConnection.state_waiters, state_happened=DeviceConnection.state_happened)),
        (r"/v1/node/(?=function)(.+)", NodeFunctionHandler, dict(conns=DeviceServer.accepted_xchange_conns, state_waiters=DeviceConnection.state_waiters, state_happened=DeviceConnection.state_happened)),
        (r"/v1/node/(?=setting)(.+)", NodeSettingHandler, dict(conns=DeviceServer.accepted_ota_conns, state_waiters=DeviceConnection.state_waiters, state_happened=DeviceConnection.state_happened)),
        (r"/v1/node/event[/]?", NodeEventHandler,dict(conns=DeviceServer.accepted_xchange_conns)),
        (r"/v1/node/config[/]?", NodeGetConfigHandler,dict(conns=DeviceServer.accepted_ota_conns, state_waiters=DeviceConnection.state_waiters, state_happened=DeviceConnection.state_happened)),
        (r"/v1/node/resources[/]?", NodeGetResourcesHandler,dict(conns=DeviceServer.accepted_ota_conns, state_waiters=DeviceConnection.state_waiters, state_happened=DeviceConnection.state_happened)),
        (r"/v1/ota/trigger[/]?", FirmwareBuildingHandler, dict(conns=DeviceServer.accepted_ota_conns, state_waiters=DeviceConnection.state_waiters, state_happened=DeviceConnection.state_happened)),
        (r"/v1/ota/status[/]?", OTAStatusReportingHandler, dict(conns=DeviceServer.accepted_ota_conns, state_waiters=DeviceConnection.state_waiters, state_happened=DeviceConnection.state_happened)),
        (r"/v1/cotf/(project)[/]?", COTFHandler, dict(conns=DeviceServer.accepted_ota_conns, state_waiters=DeviceConnection.state_waiters, state_happened=DeviceConnection.state_happened)),
        ]

        self.conn = db_conn
        self.cur = cursor

        auto_reload_for_debug = False
        try:
            auto_reload_for_debug = server_config.auto_reload_for_debug
        except:
            pass

        web.Application.__init__(self, handlers, debug=auto_reload_for_debug, login_url="/v1/user/login",
            template_path = 'templates')

class myApplication_OTA(web.Application):

    def __init__(self,db_conn,cursor):
        handlers = [
        (r"/v1[/]?", IndexHandler),
        (r"/v1/test[/]?", TestHandler),
        (r"/v1/ota/bin", OTAFirmwareSendingHandler, dict(conns=DeviceServer.accepted_ota_conns, state_waiters=DeviceConnection.state_waiters, state_happened=DeviceConnection.state_happened))
        ]
        self.conn = db_conn
        self.cur = cursor

        web.Application.__init__(self, handlers)


def main():

    ###--log_file_prefix=./server.log
    ###--logging=debug
    enable_pretty_logging()
    options.parse_command_line()

    conn = None
    cur = None
    try:
        conn = lite.connect('database.db')
        conn.row_factory = lite.Row
        cur = conn.cursor()
        cur.execute('SELECT SQLITE_VERSION()')
        data = cur.fetchone()
        gen_log.info("SQLite version: %s" % data[0])
    except lite.Error, e:
        gen_log.error(e)
        sys.exit(1)

    app = myApplication(conn, cur)
    http_server = HTTPServer(app)
    http_server.listen(8080)

    app2 = myApplication_OTA(conn, cur)
    http_server2 = HTTPServer(app2)
    http_server2.listen(8081)


    tcp_server = DeviceServer(conn, cur, 'xchange')
    tcp_server.listen(8000)

    tcp_server2 = DeviceServer(conn, cur, 'ota')
    tcp_server2.listen(8001)

    ioloop.IOLoop.current().start()

if __name__ == '__main__':
    main()

