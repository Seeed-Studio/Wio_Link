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
#                pip install pycrypto
#                pip install requests





########################################
#Configurations
#
#Set the OTA server which is used by the Wio Links
#This script will connect to OTA server to fetch node informations.
#Options: china, international, custom
OTA_SERVER='china'

#Set the address of the OTA server if OTA_SERVER is set to 'custom'
#Only applies when OTA_SERVER='custom'
CUSTOM_OTA_SERVER_ADDR='https://192.168.31.2'

#Set the accounts which will be used when logging in the OTA server
#Set each account with a key-value pair in which the pattern is email:password
ACCOUNTS={'example@mail.com':'password'}

########################################


from datetime import timedelta
import os
import json
import hashlib
import hmac
import binascii
import requests
import re
from Crypto.Cipher import AES
from Crypto import Random

from tornado.httpserver import HTTPServer
from tornado.tcpserver import TCPServer
from tornado import ioloop
from tornado import gen
from tornado import iostream
from tornado import web
from tornado import websocket
from tornado.options import define, options
from tornado.log import *
from tornado.concurrent import Future
from tornado.queues import Queue
from tornado.locks import Semaphore, Condition


define("mode", default="online", help="Running mode, online: will fetch node info from OTA server at start up.")

PENDING_REQ_CNT = 10
NODES_DATABASE=[]

BS = AES.block_size
pad = lambda s: s if (len(s) % BS == 0) else (s + (BS - len(s) % BS) * chr(0) )
unpad = lambda s : s.rstrip(chr(0))

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
                                         io_loop=self.stream.io_loop)
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
                str2 = yield gen.with_timeout(timedelta(seconds=10), self._wait_hello_future, io_loop=self.stream.io_loop)

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
            for n in NODES_DATABASE:
                if n['node_sn'] == sn:
                    node = n
                    break

            if not node:
                self.stream.write("sorry\r\n")
                yield gen.sleep(0.1)
                self.kill_myself()
                gen_log.info("node sn not found")
                raise gen.Return(101) #node not found

            key = node['node_key']
            key = key.encode("ascii")

            sig0 = hmac.new(key, msg=sn, digestmod=hashlib.sha256).digest()
            gen_log.debug("sig:     "+ binascii.hexlify(sig))
            gen_log.debug("sig calc:"+ binascii.hexlify(sig0))

            if sig0 == sig:
                #send IV + AES Key
                self.sn = sn
                self.private_key = key
                self.node_id = self.sn
                gen_log.info("valid hello packet from node %s" % self.node_id)
                #remove the junk connection of the same sn
                self.stream.io_loop.add_callback(self.device_server.remove_junk_connection, self)
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

        #self.stream.io_loop.add_future(self._serving_future, lambda future: future.result())

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
                    piece = line[:index + 2]
                    line = line[index + 2:]
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
                        gen_log.debug(state)
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
                        gen_log.warn("Node %s: %s" % (self.node_id,str(e)))

            except iostream.StreamClosedError:
                gen_log.error("StreamClosedError when reading from node %s" % self.node_id)
                self.kill_myself()
                return
            except ValueError:
                gen_log.warn("Node %s: %s can not be decoded into json" % (self.node_id, piece))
            except Exception,e:
                gen_log.error("Node %s: %s" % (self.node_id,str(e)))
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

    def __init__ (self):
        self.role = 'xchange'
        TCPServer.__init__(self)


    def handle_stream(self, stream, address):
        conn = DeviceConnection(self, stream,address)

        self.accepted_xchange_conns.append(conn)
        gen_log.info("%s device server accepted conns: %d"% (self.role, len(self.accepted_xchange_conns)))

        conn.start_serving()

    def remove_connection (self, conn):
        gen_log.info("%s device server will remove connection: %s" % (self.role, str(conn)))
        try:
            self.accepted_xchange_conns.remove(conn)
            del conn
        except:
            pass

    def remove_junk_connection (self, conn):
        try:
            connections = self.accepted_xchange_conns
            for c in connections:
                if c.sn == conn.sn and c != conn :
                    c.killed = True
                    c.kill_by_server()
                    #clear waiting futures
                    gen_log.info("%s device server removed one junk connection of same sn: %s"% (self.role, c.sn))
                    connections.remove(c)
                    del c
                    break
        except:
            pass




class NodeBaseHandler(web.RequestHandler):

    def initialize (self, conns, state_waiters, state_happened):
        self.conns = conns
        self.state_waiters = state_waiters
        self.state_happened = state_happened


    def get_current_user(self):
        return None

    def get_node (self):
        node = None
        token = self.get_argument("access_token","")
        if not token:
            try:
                token_str = self.request.headers.get("Authorization")
                token = token_str.replace("token ","")
            except:
                token = None
        gen_log.debug("node token:"+ str(token))
        if token and len(token) == 32:
            try:
                for n in NODES_DATABASE:
                    if n['node_key'] == token:
                        node = n
                        node['private_key'] = token
                        node['node_id'] = node['node_sn']
                        break
            except:
                node = None
        else:
            node = None

        if not node:
            self.resp(403,"Node is offline or invalid node token (not the user token).")
        else:
            gen_log.info("get current node, sn: %s, name: %s" % (node['node_sn'],node["name"]))

        return node

    '''
    200 OK - API call successfully executed.
    400 Bad Request - Wrong Grove/method searching path provided, or wrong parameter for method provided
    403 Forbidden - Your access token is not authorized.
    404 Not Found - The device you requested is not currently online, or the resource/endpoint you requested does not exist.
    408 Timed Out - The server can not communicate with device in a specified time out period.
    500 Server errors - It's usually caused by the unexpected errors of our side.
    '''
    def resp (self, status_code, msg="",meta=None):
        if status_code >= 300:
            self.failure_reason = msg
            raise web.HTTPError(status_code)
        else:
            #response = {"status":status,"msg":msg}
            #response = dict(response, **meta)
            #self.write(response)
            if isinstance(meta, dict):
                self.write(meta)
            elif isinstance(meta, list):
                self.write({"data":meta})
            elif isinstance(meta, str):
                self.write(meta)
            else:
                self.write({'result':'ok'})

    def write_error(self, status_code, **kwargs):
        if self.settings.get("serve_traceback") and "exc_info" in kwargs:
            # in debug mode, try to send a traceback
            lines = []
            for line in traceback.format_exception(*kwargs["exc_info"]):
                lines.append(line)

            self.finish({"error": self.failure_reason, "traceback": lines})
        else:
            self.finish({"error": self.failure_reason})



class IndexHandler(NodeBaseHandler):
    def get(self):
        self.resp(400,msg = "Please specify the url as this format: /v1/node/grove_name/property")


class NodeReadWriteHandler(NodeBaseHandler):

    @gen.coroutine
    def pre_request(self, req_type, uri):
        return True

    @gen.coroutine
    def post_request(self, req_type, uri, resp):
        #append node name to the response of .well-known
        if req_type == 'get' and uri.find('.well-known') >= 0 and 'msg' in resp and type(resp['msg']) == dict:
            resp['msg']['name'] = self.node['name']


    @gen.coroutine
    def get(self, uri):

        uri = uri.split("?")[0]
        gen_log.debug("get: "+ str(uri))


        node = self.get_node()
        if not node:
            return
        self.node = node

        if not self.pre_request('get', uri):
            return

        for conn in self.conns:
            if conn.sn == node['node_sn'] and not conn.killed:
                try:
                    cmd = "GET /%s\r\n"%(uri)
                    cmd = cmd.encode("ascii")
                    ok, resp = yield conn.submit_and_wait_resp (cmd, "resp_get")
                    if ok:
                        self.post_request('get', uri, resp)
                    if 'status' in resp and resp['status'] != 200:
                        msg = resp['msg'] or 'Unknown reason'
                        self.resp(resp['status'], msg)
                    else:
                        self.resp(200,meta=resp['msg'])
                except web.HTTPError:
                    raise
                except Exception,e:
                    gen_log.error(e)
                return
        self.resp(404, "Node is offline")

    @gen.coroutine
    def post (self, uri):

        uri = uri.split("?")[0].rstrip("/")
        gen_log.info("post to: "+ str(uri))

        node = self.get_node()
        if not node:
            return
        self.node = node

        if self.request.headers.get("content-type") and self.request.headers.get("content-type").find("json") > 0:
            self.resp(400, "Can not accept application/json post request.")
            return

        if not self.pre_request('post', uri):
            return

        for conn in self.conns:
            if conn.sn == node['node_sn'] and not conn.killed:
                try:
                    cmd = "POST /%s\r\n"%(uri)
                    cmd = cmd.encode("ascii")
                    ok, resp = yield conn.submit_and_wait_resp (cmd, "resp_post")
                    if ok:
                        self.post_request('post', uri, resp)
                    if 'status' in resp and resp['status'] != 200:
                        msg = resp['msg'] or 'Unknown reason'
                        self.resp(resp['status'], msg)
                    else:
                        self.resp(200,meta=resp['msg'])
                except web.HTTPError:
                    raise
                except Exception,e:
                    gen_log.error(e)
                return

        self.resp(404, "Node is offline")


class NodeFunctionHandler(NodeReadWriteHandler):

    @gen.coroutine
    def post (self, uri):

        uri = uri.split("?")[0].rstrip("/")
        gen_log.info("post to: "+ str(uri))

        node = self.get_node()
        if not node:
            return
        self.node = node

        if self.request.headers.get("content-type") and self.request.headers.get("content-type").find("json") > 0:
            self.resp(400, "Can not accept application/json post request.")
            return

        arg = None
        try:
            arg = self.get_body_argument('arg')
            print arg
            arg = base64.b64encode(arg)
            print arg
        except web.MissingArgumentError:
            self.resp(400, "Missing function's argument - arg")
            return
        except UnicodeEncodeError:
            self.resp(400, "Unicode is not supported")
            return

        uri = uri.strip('/')
        if uri.count('/') > 1:
            self.resp(400, "Bad URL - function's argument should in post body")
            return

        for conn in self.conns:
            if conn.sn == self.node['node_sn'] and not conn.killed:
                try:
                    cmd = "POST /%s/%s\r\n"%(uri.strip('/'), arg)
                    cmd = cmd.encode("ascii")
                    ok, resp = yield conn.submit_and_wait_resp (cmd, "resp_post")
                    if 'status' in resp and resp['status'] != 200:
                        msg = resp['msg'] or 'Unknown reason'
                        self.resp(resp['status'], msg)
                    else:
                        self.resp(200,meta=resp['msg'])
                except web.HTTPError:
                    raise
                except Exception,e:
                    gen_log.error(e)
                return
        self.resp(404, "Node is offline")


class NodeEventHandler(websocket.WebSocketHandler):
    def initialize (self, conns):
        self.conns = conns
        self.cur_conn = None
        self.node_key = None
        self.connected = False
        self.future = None

    def check_origin(self, origin):
        return True

    def open(self):
        gen_log.info("websocket open")
        self.connected = True

    def on_close(self):
        gen_log.info("websocket close")
        if self.connected and self.cur_conn:
            cur_waiters = self.cur_conn.event_waiters
            if self.future in cur_waiters:
                self.future.set_result(None)
                cur_waiters.remove(self.future)
                # cancel yield
                pass

        self.connected = False

    def find_node(self, key):
        for c in self.conns:
            if c.private_key == key and not c.killed:
                return c
        return None

    @gen.coroutine
    def on_message(self, message):
        self.node_key = message
        if len(message) != 32:
            self.write_message({"error":"invalid node sn "})
            self.connected = False
            self.close()

        self.cur_conn = self.find_node(message)

        if not self.cur_conn:
            self.node_offline()
            return

        #clear the events buffered before any websocket client connected
        self.cur_conn.event_happened = []

        while self.connected:
            self.future = self.wait_event_post()
            event = None
            try:
                event = yield gen.with_timeout(timedelta(seconds=5), self.future, io_loop=ioloop.IOLoop.current())
            except gen.TimeoutError:
                if not self.cur_conn or self.cur_conn.killed:
                    gen_log.debug("node %s is offline" % message)
                    self.cur_conn = self.find_node(message)
                    if not self.cur_conn:
                        self.node_offline()
            if event:
                self.write_message(event)
            yield gen.moment

    def wait_event_post(self):
        result_future = Future()

        if len(self.cur_conn.event_happened) > 0:
            result_future.set_result(self.cur_conn.event_happened.pop(0))
        else:
            self.cur_conn.event_waiters.append(result_future)

        return result_future

    def node_offline(self):
        try:
            self.write_message({"error":"node is offline"})
        except:
            pass
        self.connected = False
        self.close()



class myApplication(web.Application):

    def __init__(self):
        handlers = [
        (r"/v1[/]?", IndexHandler, dict(conns=None)),
        (r"/v1/node/(?!event|config|resources|setting|function)(.+)", NodeReadWriteHandler, dict(conns=DeviceServer.accepted_xchange_conns, state_waiters=DeviceConnection.state_waiters, state_happened=DeviceConnection.state_happened)),
        (r"/v1/node/(?=function)(.+)", NodeFunctionHandler, dict(conns=DeviceServer.accepted_xchange_conns, state_waiters=DeviceConnection.state_waiters, state_happened=DeviceConnection.state_happened)),
        (r"/v1/node/event[/]?", NodeEventHandler,dict(conns=DeviceServer.accepted_xchange_conns)),
        ]

        web.Application.__init__(self, handlers, debug=False)


def fetch_node_info():

    global NODES_DATABASE

    ota_server_addr = CUSTOM_OTA_SERVER_ADDR
    if OTA_SERVER == 'china':
        ota_server_addr = 'https://cn.iot.seeed.cc'
    elif OTA_SERVER == 'international':
        ota_server_addr = 'https://iot.seeed.cc'

    if len(ACCOUNTS) == 0:
        gen_log.error("Please configure the user account!!!")
        sys.exit()

    gen_log.info("Fetching the list of nodes...")

    for a in ACCOUNTS:
        acc = {'email':a, 'password': ACCOUNTS[a]}
        login_result_obj = {'token':'', }
        try:
            r = requests.post(ota_server_addr.rstrip('/')+'/v1/user/login', data=acc, verify=False)
            if r.status_code != requests.codes.ok:
                r.raise_for_status()
            else:
                login_result_obj = r.json()

        except requests.exceptions.HTTPError,e:
            gen_log.error(e)
            sys.exit()

        #print login_result_obj
        if 'status' in login_result_obj and login_result_obj['status'] != 200:
            gen_log.error('error happened when logging in %s' % a)
            gen_log.info(login_result_obj['msg'])
            sys.exit()


        try:
            r = requests.get(ota_server_addr.rstrip('/')+'/v1/nodes/list?access_token='+login_result_obj['token'], verify=False)
            if r.status_code != requests.codes.ok:
                r.raise_for_status()
            else:
                list_result_obj = r.json()

        except requests.exceptions.HTTPError,e:
            gen_log.error(e)
            sys.exit()

        if 'status' in list_result_obj and list_result_obj['status'] != 200:
            gen_log.error('error happened when listing nodes for %s' % a)
            gen_log.info(list_result_obj['msg'])
            sys.exit()

        NODES_DATABASE += list_result_obj['nodes']

    if len(NODES_DATABASE) == 0:
        gen_log.error("No node!!!")
        sys.exit()

    gen_log.info("fetch nodes done!")
    gen_log.debug(NODES_DATABASE)
    cur_dir = os.path.split(os.path.realpath(__file__))[0]
    open("%s/nodes_db_lean.json"%cur_dir,"w").write(json.dumps(NODES_DATABASE))

def main():

    global NODES_DATABASE

    ###--log_file_prefix=./server.log
    ###--logging=debug
    enable_pretty_logging()
    options.parse_command_line()

    cur_dir = os.path.split(os.path.realpath(__file__))[0]

    if options.mode == 'online':
        fetch_node_info()
    else:
        db_file_path = '%s/nodes_db_lean.json' % cur_dir

        if not os.path.exists(db_file_path):
            fetch_node_info()
        else:
            NODES_DATABASE = json.load(open(db_file_path))
            gen_log.info('load nodes database from json done!')
            gen_log.debug(NODES_DATABASE)


    app = myApplication()
    http_server = HTTPServer(app)
    http_server.listen(8080)


    tcp_server = DeviceServer()
    tcp_server.listen(8000)

    gen_log.info("server's running in lean mode ...")
    gen_log.info("please request the APIs through http://your-ip-address:8080/v1/node/...")

    ioloop.IOLoop.current().start()


if __name__ == '__main__':
    main()


