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

#   Dependences: see server.py header section

import os
from datetime import timedelta
import json
import sqlite3 as lite
import re
import jwt
import md5
import hashlib
import base64
import httplib
import uuid
from shutil import copy
from build_firmware import *
import yaml
import threading
import time
import smtplib
import traceback
import config as server_config

from tornado.httpserver import HTTPServer
from tornado.tcpserver import TCPServer
from tornado import ioloop
from tornado import gen
from tornado import iostream
from tornado import web
from tornado import websocket
from tornado import escape
from tornado.options import define, options
from tornado.log import *
from tornado.concurrent import Future
from tornado_cors import CorsMixin

TOKEN_SECRET = "!@#$%^&*RG)))))))JM<==TTTT==>((((((&^HVFT767JJH"


class BaseHandler(CorsMixin, web.RequestHandler):
    CORS_ORIGIN = '*'
    CORS_HEADERS = 'Content-Type'

    def get_current_user(self):
        user = None
        token = self.get_argument("access_token","")
        if not token:
            try:
                token_str = self.request.headers.get("Authorization")
                token = token_str.replace("token ","")
            except:
                token = None

        if token:
            try:
                cur = self.application.cur
                cur.execute('select * from users where token="%s"'%token)
                rows = cur.fetchall()
                if len(rows) > 0:
                    user = rows[0]
            except:
                user = None
        else:
            user = None

        if not user:
            self.resp(403,"Please login to get the token")
        else:
            gen_log.info("get current user, id: %s, email: %s" % (user['user_id'], user['email']))

        return user



    '''
    200 OK - API call successfully executed.
    400 Bad Request - Wrong Grove/method searching path provided, or wrong parameter for method provided
    403 Forbidden - Your access token is not authorized.
    404 Not Found - The device you requested is not currently online, or the resource/endpoint you requested does not exist.
    408 Timed Out - The server can not communicate with device in a specified time out period.
    500 Server errors - It's usually caused by the unexpected errors of our side.
    '''
    def resp (self, status_code, meta=None):
        if status_code >= 300:
            self.failure_reason = str(meta)
            raise web.HTTPError(status_code)
        else:
            if isinstance(meta, dict):
                self.write(meta)
            elif isinstance(meta, list):
                self.write({"data":meta})
            elif not meta:
                self.write({'result':'ok'})
            else:
                self.write(meta)


    def write_error(self, status_code, **kwargs):
        if self.settings.get("serve_traceback") and "exc_info" in kwargs:
            # in debug mode, try to send a traceback
            lines = []
            for line in traceback.format_exception(*kwargs["exc_info"]):
                lines.append(line)

            self.finish({"error": self.failure_reason, "traceback": lines})
        else:
            try:
              self.finish({"error": self.failure_reason})
            except AttributeError:
              self.finish({"error": "Unknown error occured."})

    def get_uuid (self):
        return str(uuid.uuid4())

    def gen_uuid_without_dash(self):
        return str(uuid.uuid1()).replace('-','')

    def gen_token (self, email):
        return jwt.encode({'email': email,'uuid':self.get_uuid()}, TOKEN_SECRET, algorithm='HS256').split(".")[2]

class IndexHandler(BaseHandler):
    @web.authenticated
    def get(self):
        #DeviceServer.accepted_conns[0].submit_cmd("OTA\r")
        self.resp(400, "Please specify the url as this format: /v1/node/grove_name/property")

class TestHandler(web.RequestHandler):
    def get(self):
        self.render("test.html", ip=self.request.remote_ip)



class UserCreateHandler(BaseHandler):
    def get (self):
        self.resp(404, "Please post to this url")

    def post(self):
        email = self.get_argument("email","")
        passwd = self.get_argument("password","")
        if not email:
            self.resp(400, "Missing email information")
            return
        if not passwd:
            self.resp(400, "Missing password information")
            return
        if not re.match(r'\w[\w\.-]*@\w[\w\.-]+\.\w+', email):
            self.resp(400, "Bad email address")
            return

        cur = self.application.cur
        token = self.gen_token(email)
        try:
            cur.execute('select * from users where email="%s"'%email)
            rows = cur.fetchall()
            if len(rows) > 0:
                self.resp(400, "This email already registered")
                return
            cur.execute("INSERT INTO users(user_id,email,pwd,token,created_at) VALUES(?,?,?,?,datetime('now'))", (self.gen_uuid_without_dash(), email, md5.new(passwd).hexdigest(), token))
        except web.HTTPError:
            raise
        except Exception,e:
            self.resp(500,str(e))
            return
        finally:
            self.application.conn.commit()

        self.resp(200, meta={"token": token})


class ExtUsersHandler(BaseHandler):
    def get (self, uri):
        print uri
        self.resp(404, "Please post to this url")

    def post(self, uri):
        email = self.get_argument("email","")
        bind_id = self.get_argument("bind_id","")
        bind_region = self.get_argument("bind_region","")
        token = self.get_argument("token","")
        secret = self.get_argument("secret","")


        if secret != TOKEN_SECRET:
            self.resp(403, "Wrong secret")
            return
        if not bind_id and not email:
            self.resp(400, "Missing bind_id / email information")
            return
        if not token:
            self.resp(400, "Missing token information")
            return

        cur = self.application.cur

        try:
            create_new = False
            if email:
                cur.execute('SELECT * FROM users WHERE email=?', (email,))
                rows = cur.fetchall()
                if len(rows) > 0:
                    cur.execute('UPDATE users SET token=?,ext_bind_id=?,ext_bind_region=? WHERE email=?', (token, bind_id, bind_region, email))
                else:
                    create_new = True
            else:
                cur.execute('SELECT * FROM users WHERE ext_bind_id=?', (bind_id,))
                rows = cur.fetchall()
                if len(rows) > 0:
                    cur.execute('UPDATE users SET token=? WHERE ext_bind_id=?', (token, bind_id))
                else:
                    create_new = True
            if create_new:
                cur.execute("INSERT INTO users(user_id,email,token,ext_bind_id,ext_bind_region,created_at) VALUES(?,?,?,?,?,datetime('now'))",
                            (self.gen_uuid_without_dash(), email, token, bind_id, bind_region))
        except web.HTTPError:
            raise
        except Exception,e:
            self.resp(500,str(e))
            return
        finally:
            self.application.conn.commit()

        self.resp(200)


class UserChangePasswordHandler(BaseHandler):
    def get (self):
        self.resp(403, "Please post to this url")

    @web.authenticated
    def post(self):
        passwd = self.get_argument("password","")
        if not passwd:
            self.resp(400, "Missing new password information")
            return

        email = self.current_user["email"]
        token = self.current_user["token"]
        gen_log.info("%s want to change password with token %s"%(email,token))

        cur = self.application.cur
        try:
            new_token = self.gen_token(email)
            cur.execute('update users set pwd=?,token=? where email=?', (md5.new(passwd).hexdigest(),new_token, email))
            self.resp(200, meta={"token": new_token})
            gen_log.info("%s succeed to change password"%(email))
        except Exception,e:
            self.resp(500,str(e))
            return
        finally:
            self.application.conn.commit()

class UserRetrievePasswordHandler(BaseHandler):
    def get(self):
        self.retrieve()

    def post(self):
        self.retrieve()

    def retrieve(self):
        email = self.get_argument('email','')
        if not email:
            self.resp(400, "You must specify the email of the account which you want to retrieve password.")
            return

        if not re.match(r'^[_.0-9a-z-]+@([0-9a-z][0-9a-z-]+.)+[a-z]{2,4}$', email):
            self.resp(400, "Bad email address")
            return

        gen_log.info("%s want to retrieve password"%(email))

        cur = self.application.cur
        try:
            cur.execute('select * from users where email="%s"' % email)
            row = cur.fetchone()
            if not row:
                self.resp(404, "No account registered with this email")
                return

            new_password = self.gen_token(email)[0:6]
            cur.execute('update users set pwd=? where email=?', (md5.new(new_password).hexdigest(), email))

            #start a thread sending email here
            ioloop.IOLoop.current().add_callback(self.start_thread_send_email, email, new_password)

            self.resp(200)
        except web.HTTPError:
            raise
        except Exception,e:
            self.resp(500, str(e))
            return
        finally:
            self.application.conn.commit()


    def start_thread_send_email (self, email, new_password):
        thread_name = "email_thread-" + str(email)
        li = threading.enumerate()
        for l in li:
            if l.getName() == thread_name:
                gen_log.info('INFO: Skip same email request!')
                return

        threading.Thread(target=self.email_sending_thread, name=thread_name,
            args=(email, new_password)).start()


    def email_sending_thread (self, email, new_password):
        s = smtplib.SMTP_SSL(server_config.smtp_server)
        try:
            s.login(server_config.smtp_user, server_config.smtp_pwd)
        except Exception,e:
            gen_log.error(e)
            return

        sender = 'no_reply@seeed.cc'
        receiver = email

        message = """From: Wio_Link <%s>
To: <%s>
Subject: The password for your account of iot.seeed.cc has been retrieved

Dear User,

Thanks for your interest in iot.seeed.cc, the new password for your account is
%s

Please change it as soon as possible.

Thank you!

IOT Team from Seeed

""" % (sender, receiver, new_password)
        try:
            s.sendmail(sender, receiver, message)
        except Exception,e:
            gen_log.error(e)
            return
        gen_log.info('sent new password %s to %s' % (new_password, email))



class UserLoginHandler(BaseHandler):
    def get (self):
        self.resp(403, "Please post to this url")

    def post(self):
        if self.request.headers.get("content-type") and self.request.headers.get("content-type").find("json") > 0:
            try:
                json_data = json.loads(self.request.body)
                email = json_data['email']
                passwd = json_data['password']
            except ValueError:
                self.resp(400, 'Unable to parse JSON.')
        else:
            email = self.get_argument("email","")
            passwd = self.get_argument("password","")

        if not email:
            self.resp(400, "Missing email information")
            return
        if not passwd:
            self.resp(400, "Missing password information")
            return
        if not re.match(r'^[_.0-9a-z-]+@([0-9a-z][0-9a-z-]+.)+[a-z]{2,4}$', email):
            self.resp(400, "Bad email address")
            return

        cur = self.application.cur
        try:
            cur.execute('select * from users where email=? and pwd=?', (email, md5.new(passwd).hexdigest()))
            row = cur.fetchone()
            if not row:
                self.resp(400, "Login failed - invalid email or password")
                return
            self.resp(200, meta={"token": row["token"], "user_id": row["user_id"]})
        except web.HTTPError:
            raise
        except Exception,e:
            self.resp(500,str(e))
            return
        finally:
            self.application.conn.commit()

class DriversHandler(BaseHandler):
    @web.authenticated
    def get (self):
        cur_dir = os.path.split(os.path.realpath(__file__))[0]
        json_drivers = {}
        with open(os.path.join(cur_dir, "drivers.json")) as f:
            json_drivers = json.load(f)
        self.resp(200, meta={"drivers": json_drivers})

    @web.authenticated
    def post(self):
        self.resp(403, "Please get this url")

class DriversStatusHandler(BaseHandler):
    @web.authenticated
    def get (self):
        cur_dir = os.path.split(os.path.realpath(__file__))[0]
        scan_status = {}
        with open(os.path.join(cur_dir, "scan_status.json")) as f:
            scan_status = json.load(f)
        self.resp(200, meta=scan_status)


    @web.authenticated
    def post(self):
        self.resp(403, "Please get this url")

class BoardsListHandler(BaseHandler):
    @web.authenticated
    def get (self):
        cur_dir = os.path.split(os.path.realpath(__file__))[0]
        boards = []
        with open(os.path.join(cur_dir, "boards.json")) as f:
            boards = json.load(f)
        self.resp(200, meta={"boards": boards})


    @web.authenticated
    def post(self):
        self.resp(403, "Please get this url")



class NodeCreateHandler(BaseHandler):
    def get (self):
        self.resp(404, "Please post to this url")

    @web.authenticated
    def post(self):
        node_name = self.get_argument("name","").strip()
        if not node_name:
            self.resp(400, "Missing node name information")
            return

        board = self.get_argument("board","").strip()
        if not board:
            board = "Wio Link v1.0"

        user = self.current_user
        email = user["email"]
        user_id = user["user_id"]
        node_id = self.gen_uuid_without_dash()
        node_sn = md5.new(email+self.get_uuid()).hexdigest()
        node_key = md5.new(self.gen_token(email)).hexdigest()  #we need the key to be 32bytes long too

        cur = self.application.cur
        try:
            cur.execute("INSERT INTO nodes(node_id,user_id,node_sn,name,private_key,board) VALUES(?,?,?,?,?,?)", (node_id, user_id, node_sn,node_name, node_key, board))
            self.resp(200, meta={"node_sn":node_sn,"node_key": node_key})
        except Exception,e:
            self.resp(500,str(e))
            return
        finally:
            self.application.conn.commit()


class NodeListHandler(BaseHandler):
    def initialize (self, conns):
        self.conns = conns

    @web.authenticated
    def get (self):
        user = self.current_user
        email = user["email"]
        user_id = user["user_id"]

        cur = self.application.cur
        try:
            cur.execute("select * from nodes where user_id='%s'" % (user_id))
            rows = cur.fetchall()
            nodes = []
            for r in rows:
                online = False
                for conn in self.conns:
                    if r['node_sn'] == conn.sn and conn.online_status == True:
                        online = True
                        break
                board = r["board"] if r["board"] else "Wio Link v1.0"
                nodes.append({"name":r["name"],"node_sn":r["node_sn"],"node_key":r['private_key'], "online":online, "dataxserver":r["dataxserver"], "board":board})
            self.resp(200, meta={"nodes":nodes})
        except Exception,e:
            self.resp(500,str(e))
            return

    def post(self):
        self.resp(404, "Please get this url")

class NodeRenameHandler(BaseHandler):
    def get (self):
        self.resp(404, "Please post to this url")

    @web.authenticated
    def post(self):
        node_sn = self.get_argument("node_sn","").strip()
        if not node_sn:
            self.resp(400, "Missing node sn information")
            return

        new_node_name = self.get_argument("name","").strip()
        gen_log.debug('node %s wants to change its name to %s' % (node_sn, new_node_name))
        if not new_node_name:
            self.resp(400, "Missing node name information")
            return

        cur = self.application.cur
        try:
            cur.execute("UPDATE nodes set name=? WHERE node_sn=?" , (new_node_name, node_sn))
            if cur.rowcount > 0:
                self.resp(200)
            else:
                self.resp(400, "Node not exist")
        except web.HTTPError:
            raise
        except Exception,e:
            self.resp(500,str(e))
            return
        finally:
            self.application.conn.commit()

class NodeDeleteHandler(BaseHandler):
    @web.authenticated
    def get (self):
        self.resp(404, "Please post to this url")

    @web.authenticated
    def post(self):
        node_sn = self.get_argument("node_sn","").strip()
        if not node_sn:
            self.resp(400, "Missing node sn information")
            return

        user = self.current_user
        user_id = user["user_id"]

        cur = self.application.cur
        try:
            cur.execute("select * from nodes where user_id=? and node_sn=?", (user_id, node_sn))
            rows = cur.fetchall()
            if len(rows) > 0:
                node_id = rows[0]['node_id']
                cur.execute("delete from resources where node_id=?", (node_id, ))
                cur.execute("delete from nodes where node_id=?", (node_id, ))
                if cur.rowcount > 0:
                    self.resp(200)
                else:
                    self.resp(400, "node not exist")
            else:
                self.resp(400, "node not exist")

        except web.HTTPError:
            raise
        except Exception,e:
            self.resp(500,str(e))
            return
        finally:
            self.application.conn.commit()

class NodeBaseHandler(BaseHandler):

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
        if token:
            try:
                cur = self.application.cur
                cur.execute('select * from nodes where private_key="%s"'%token)
                rows = cur.fetchall()
                if len(rows) > 0:
                    node = rows[0]
            except:
                node = None
        else:
            node = None

        if not node:
            self.resp(403,"Please attach the valid node token (not the user token)")
        else:
            gen_log.debug("get current node, id: %s, name: %s" % (node['node_id'],node["name"]))

        return node



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


class NodeSettingHandler(NodeReadWriteHandler):

    def pre_request(self, req_type, uri):
        if req_type == 'post' and uri.find('setting/dataxserver') >= 0:
            ips = re.findall(r'.*/(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})', uri)
            if not ips:
                self.resp(400, "please specify the correct ip address for data exchange server")
                return False
            url = self.get_argument('dataxurl', '').rstrip('/')
            patt = r'^(?=^.{3,255}$)(http(s)?:\/\/)?(www\.)?[a-zA-Z0-9][-a-zA-Z0-9]{0,62}(\.[a-zA-Z0-9][-a-zA-Z0-9]{0,62})+(:\d+)*(\/\w+\.\w+)*$'
            if not url or not re.match(patt, url):
                self.resp(400, "please specify the correct url for data exchange server")
                return False
            else:
                return True

        if req_type == 'post' and uri.find('setting/drop') >= 0:
            for conn in self.conns:
                if conn.sn == self.node['node_sn'] and not conn.killed:
                    conn.kill_myself()
                    self.resp(200)
                    return False
            self.resp(404, "Node is offline")
            return False

    def post_request(self, req_type, uri, resp):
        if req_type == 'post' and uri.find('setting/dataxserver') >= 0:
            #ips = re.findall(r'.*/(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})', uri)
            url = self.get_argument('dataxurl', '')
            if url:
                #print url
                gen_log.debug('node %s want to change data x server to %s' % (self.node['node_id'], url))
                try:
                    cur = self.application.cur
                    cur.execute('update nodes set dataxserver=? where node_id=?', (url, self.node['node_id']))
                except Exception,e:
                    gen_log.error(e)
                finally:
                    self.application.conn.commit()


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


class NodeGetConfigHandler(NodeBaseHandler):

    def get(self):
        node = self.get_node()
        if not node:
            return

        user_id = node["user_id"]
        node_name = node["name"]
        node_sn = node["node_sn"]

        cur_dir = os.path.split(os.path.realpath(__file__))[0]
        user_build_dir = cur_dir + '/users_build/' + str(user_id)  + '_' + node_sn
        if not os.path.exists(user_build_dir):
            self.resp(404, "Config not found")
            return

        try:
            # try to fine connection_config.json first
            json_file = open("%s/connection_config.json" % user_build_dir, 'r')
            self.resp(200,  meta={"config":json.load(json_file), "type":"json"})
            json_file.close()
            return
        except Exception,e:
            gen_log.warn("Exception when reading json file:"+ str(e))

        try:
            # fall back to old version connection_config.yaml
            yaml_file = open("%s/connection_config.yaml" % user_build_dir, 'r')
            self.resp(200, meta={"config": yaml_file.read(), "type":"yaml"})
            yaml_file.close()
            return
        except Exception,e:
            gen_log.error("Exception when reading yaml file:" + str(e))
            self.resp(404, "Config not found")


    def post(self):
        self.resp(404, "Please get this url")


class NodeGetResourcesHandler(NodeBaseHandler):

    vhost_url_base = 'https://iot.seeed.cc'

    def prepare_data_for_template(self, node, grove_instance_name, grove, grove_doc):

        data = []
        events = []

        methods_doc = grove_doc['Methods']
        #read functions
        for fun in grove['Reads'].items():
            item = {}
            item['type'] = 'GET'
            #build read arg
            arguments = []
            arguments_name = []
            method_name = fun[0].replace('read_','')
            url = self.vhost_url_base + '/v1/node/' + grove_instance_name + '/' + method_name
            arg_list = fun[1]['Arguments']
            for arg in arg_list:
                if not arg:
                    continue
                t = arg[0]
                name = arg[1]

                if fun[0] in methods_doc and name in methods_doc[fun[0]]:
                    comment = ", " + methods_doc[fun[0]][name]
                else:
                    comment = ""

                arguments.append('[%s]: %s value%s' % (name, t, comment))
                arguments_name.append('[%s]' % name)
                url += ('/[%s]' % name)

            item['url'] = url + '?access_token=' + node['private_key']
            item['arguments'] = arguments
            item['arguments_name'] = arguments_name

            item['brief'] = ""
            if fun[0] in methods_doc and '@brief@' in methods_doc[fun[0]]:
                item['brief'] = methods_doc[fun[0]]['@brief@']


            #build returns
            returns = ""
            return_docs = []
            arg_list = fun[1]['Returns']
            for arg in arg_list:
                if not arg:
                    continue
                t = arg[0]
                name = arg[1]
                returns += ('"%s": [%s value],' % (name, t))

                if fun[0] in methods_doc and name in methods_doc[fun[0]]:
                    comment = ", " + methods_doc[fun[0]][name]
                    return_docs.append('%s: %s value%s' % (name, t, comment))


            returns = returns.rstrip(',')
            item['returns'] = returns
            item['return_docs'] = return_docs
            item['uuid'] = self.get_uuid();

            data.append(item)

        for fun in grove['Writes'].items():
            item = {}
            item['type'] = 'POST'
            #build write arg
            arguments = []
            arguments_name = []
            method_name = fun[0].replace('write_','')
            url = self.vhost_url_base + '/v1/node/' + grove_instance_name + '/' + method_name
            #arg_list = [arg for arg in fun[1] if arg.find("*")<0]  #find out the ones havent "*"
            arg_list = fun[1]['Arguments']
            for arg in arg_list:
                if not arg:
                    continue
                t = arg[0]
                name = arg[1]

                if fun[0] in methods_doc and name in methods_doc[fun[0]]:
                    comment = ", " + methods_doc[fun[0]][name]
                else:
                    comment = ""

                arguments.append('[%s]: %s value%s' % (name, t, comment))
                arguments_name.append('[%s]' % name)
                url += ('/[%s]' % name)
            item['url'] = url + '?access_token=' + node['private_key']
            item['arguments'] = arguments
            item['arguments_name'] = arguments_name

            item['brief'] = ""
            if fun[0] in methods_doc and '@brief@' in methods_doc[fun[0]]:
                item['brief'] = methods_doc[fun[0]]['@brief@']
            item['uuid'] = self.get_uuid();

            data.append(item)

        if grove['HasEvent']:
            for ev in grove['Events']:
                if ev in grove_doc['Events']:
                    events.append({'event_name':ev, 'event_data_type':grove['Events'][ev], 'event_desc':grove_doc['Events'][ev]})
                else:
                    events.append({'event_name':ev, 'event_data_type':grove['Events'][ev], 'event_desc': ''})

        return (data, events)


    def get(self):
        node = self.get_node()
        if not node:
            return

        user_id = node["user_id"]
        node_name = node["name"]
        node_id = node['node_id']
        node_sn = node['node_sn']
        node_key = node['private_key']
        dataxserver = node['dataxserver']

        cur_dir = os.path.split(os.path.realpath(__file__))[0]
        user_build_dir = cur_dir + '/users_build/' + str(user_id) + '_' + node_sn
        if not os.path.exists(user_build_dir):
            self.resp(404, "Configuration file not found")
            return

        #adjust the vhost
        self.vhost_url_base = server_config.vhost_url_base.rstrip('/')

        if not self.vhost_url_base:
            if self.request.host.find(":8080") >= 0:
                protocol = 'http'
            else:
                protocol = 'https'
            self.vhost_url_base = '%s://%s' % (protocol, self.request.host)

        self.vhost_url_base = self.get_argument("data_server", self.vhost_url_base)
        self.vhost_url_base = self.vhost_url_base.rstrip('/')
        #print self.vhost_url_base

        patt = r'^(?=^.{3,255}$)(http(s)?:\/\/)?(www\.)?[a-zA-Z0-9][-a-zA-Z0-9]{0,62}(\.[a-zA-Z0-9][-a-zA-Z0-9]{0,62})+(:\d+)*(\/\w+\.\w+)*$'
        if not re.match(patt, self.vhost_url_base):
            self.write("Bad format of parameter data_server. Should be: http[s]://domain-or-ip-address[:port]")
            return

        #if self.vhost_url_base.find('http') < 0:
        #    self.vhost_url_base = 'https://'+self.vhost_url_base
        #print self.vhost_url_base


        #open the yaml file for reading
        try:
            config_file = open('%s/connection_config.yaml'%user_build_dir,'r')
        except Exception,e:
            gen_log.error("Exception when reading yaml file:"+ str(e))
            self.resp(404, "No resources, the node has not been configured jet.")
            return

        #open the json file for reading
        try:
            drv_db_file = open('%s/drivers.json' % cur_dir,'r')
            drv_doc_file= open('%s/driver_docs.json' % cur_dir,'r')
        except Exception,e:
            gen_log.error("Exception when reading grove drivers database file:"+ str(e))
            self.resp(404, "Internal error, the grove drivers database file is corrupted.")
            return

        #calculate the checksum of 2 file
        sha1 = hashlib.sha1()
        sha1.update(config_file.read() + self.vhost_url_base)
        chksum_config = sha1.hexdigest()
        sha1 = hashlib.sha1()
        sha1.update(drv_db_file.read() + drv_doc_file.read())
        chksum_drv_db = sha1.hexdigest()

        #query the database, if 2 file not changed, echo cached html
        resource = None
        try:
            cur = self.application.cur
            cur.execute('select * from resources where node_id="%s"'%node_id)
            rows = cur.fetchall()
            if len(rows) > 0:
                resource = rows[0]
        except:
            resource = None

        if resource:
            if chksum_config == resource['chksum_config'] and chksum_drv_db == resource['chksum_dbjson']:
                gen_log.info("echo the cached page for node_id: %s" % node_id)
                self.write(resource['render_content'])
                return

        gen_log.info("re-render the resource page for node_id: %s" % node_id)

        #else render new resource page
        #load the yaml file into object
        config_file.seek(0)
        drv_db_file.seek(0)
        drv_doc_file.seek(0)
        try:
            config = yaml.load(config_file)
        except yaml.YAMLError, err:
            gen_log.error("Error in parsing yaml file:"+ str(err))
            self.resp(404, "No resources, the configuration file is corrupted.")
            return
        except web.HTTPError:
            raise
        except Exception,e:
            gen_log.error("Error in loading yaml file:"+ str(e))
            self.resp(404, "No resources, the configuration file is corrupted.")
            return
        finally:
            config_file.close()

        #load the json file into object
        try:
            drv_db = json.load(drv_db_file)
            drv_docs = json.load(drv_doc_file)
        except Exception,e:
            gen_log.error("Error in parsing grove drivers database file:"+ str(e))
            self.resp(404, "Internal error, the grove drivers database file is corrupted.")
            return
        finally:
            drv_db_file.close()
            drv_doc_file.close()

        #prepare resource data structs
        data = []
        events = []

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
                grove = find_grove_in_database(_name, _sku, drv_db)
                if grove:
                    grove_doc = find_grove_in_docs_db(grove['ID'], drv_docs)
                    if grove_doc:
                        d, e = self.prepare_data_for_template(node,grove_instance_name,grove,grove_doc)
                        data += d
                        events += e

                    else:  #if grove_doc
                        error_msg = "Error, cannot find %s in grove driver documentation database file."%grove_instance_name
                        gen_log.error(error_msg)
                        self.resp(404, error_msg)
                        return

                else:  #if grove
                    error_msg = "Error, cannot find %s in grove drivers database file."%grove_instance_name
                    gen_log.error(error_msg)
                    self.resp(404, error_msg)
                    return

        #render the template
        ws_domain = self.vhost_url_base.replace('https:', 'wss:')
        ws_domain = ws_domain.replace('http:', 'ws:')
        #print data
        #print events
        #print node_key
        page = self.render_string('resources.html', node_name = node_name, events = events, data = data,
                                  node_key = node_key , url_base = self.vhost_url_base, ws_domain=ws_domain)

        #store the page html into database
        try:
            cur = self.application.cur
            if resource:
                cur.execute('update resources set chksum_config=?,chksum_dbjson=?,render_content=? where node_id=?',
                            (chksum_config, chksum_drv_db, page, node_id))
            else:
                cur.execute('insert into resources (node_id, chksum_config, chksum_dbjson, render_content) values(?,?,?,?)',
                            (node_id, chksum_config, chksum_drv_db, page))
        except:
            resource = None
        finally:
            self.application.conn.commit()

        #echo out
        self.write(page)



    def post(self):
        self.resp(404, "Please get this url")


class FirmwareBuildingHandler(NodeBaseHandler):
    """
    post two para, node_token and yaml

    """

    def get (self):
        self.resp(404, "Please post to this url")

    @gen.coroutine
    def post(self):
        gen_log.info("FirmwareBuildingHandler")
        node = self.get_node()
        if not node:
            return

        cur_conn = None

        for conn in self.conns:
            if conn.private_key == node['private_key'] and not conn.killed:
                cur_conn = conn
                break

        if not cur_conn:
            self.resp(404, "Node is offline")
            return

        self.cur_conn = cur_conn

        self.user_id = node["user_id"]
        self.node_name = node["name"]
        self.node_id = node["node_id"]
        self.node_sn = node["node_sn"]

        cur_dir = os.path.split(os.path.realpath(__file__))[0]
        user_build_dir = cur_dir + '/users_build/' + str(self.user_id) + '_' + self.node_sn
        if not os.path.exists(user_build_dir):
            os.makedirs(user_build_dir)

        phase = self.get_argument("build_phase","")
        build_phase = [1,2]
        if phase == '1':
            build_phase = [1]
        elif phase == '2':
            build_phase = [2]
        elif phase == '3':
            build_phase = [1,2]
        self.build_phase = build_phase


        ### prepare the config file or config json object
        json_connections = None
        json_drivers = None

        if 1 in build_phase:
            if self.request.headers.get("content-type") and self.request.headers.get("content-type").find("json") > 0:
                #try json first
                gen_log.debug("json request")
                gen_log.debug(type(self.request.body))
                gen_log.debug(self.request.body)

                try:
                    json_connections = json.loads(self.request.body)
                except Exception,e:
                    self.resp(400, 'invalid json: ' + str(e))
                    return

                if 'board_name' in json_connections and 'connections' in json_connections:
                    pass
                else:
                    self.resp(400, "config json invalid!")
                    return

                with open("%s/connection_config.json" % user_build_dir, 'wr') as f:
                    f.write(json.dumps(json_connections))
            else:
                #fall back to old version yaml config file
                yaml = self.get_argument("yaml","")
                if not yaml:
                    self.resp(400, "Missing yaml information")
                    return

                try:
                    yaml = base64.b64decode(yaml)
                except:
                    yaml = ""

                gen_log.debug(yaml)
                if not yaml:
                    gen_log.error("no valid yaml provided")
                    self.resp(400, "no valid yaml provided")
                    return


                with open("%s/connection_config.yaml" % user_build_dir, 'wr') as f:
                    f.write(yaml)

        ### start to compile
        # always overwrite the Makefile
        copy('%s/Makefile.template'%cur_dir, '%s/Makefile'%user_build_dir)

        #try to get the current running app number
        app_num = 'ALL'
        if 2 in build_phase:
            try:
                cmd = "APP\r\n"
                ok, resp = yield conn.submit_and_wait_resp (cmd, "resp_app")
                if ok and resp['msg'] in [1,2,'1','2']:
                    app_num = '1' if resp['msg'] in [2,'2'] else '2'
                    gen_log.info('Get to know node %s is running app %s' % (self.node_id, resp['msg']))
                else:
                    gen_log.warn('Failed while getting app number for node %s: %s' % (self.node_id, str(resp)))
            except Exception,e:
                gen_log.error(e)

        #try to create a thread
        thread_name = "build_thread_" + self.node_sn
        li = threading.enumerate()
        for l in li:
            if l.getName() == thread_name:
                msg = 'A building thread is still running, node id %d' % self.node_id
                gen_log.info(msg)
                state = ("error", msg)
                self.send_notification(state)
                return

        threading.Thread(target=self.build_thread, name=thread_name,
                         args=(build_phase, app_num, self.user_id, self.node_name, self.node_sn, json_drivers, json_connections)).start()

        #clear the possible old state recv during last ota process
        try:
            self.state_happened[self.node_sn] = []
            self.state_waiters[self.node_sn] = []
        except Exception,e:
            pass

        #log the building
        if 2 in build_phase:
            try:
                cur = self.application.cur
                cur.execute("insert into builds (node_id, build_date, build_starttime, build_endtime) \
                            values(?,date('now'),datetime('now'),datetime('now'))", (self.node_id, ))
                self.cur_build_id = cur.lastrowid
            except Exception,e:
                gen_log.error("Failed to log the building record: %s" % str(e))
            finally:
                self.application.conn.commit()

        #echo the response first
        if 2 in build_phase:
            self.resp(200,meta={'ota_status': "going", "ota_msg": "Building the firmware..."})
        elif 1 in build_phase:
            self.resp(200,meta={'ota_status': "going", "ota_msg": "Generating the files..."})


    @gen.coroutine
    def build_thread (self, build_phase, app_num, user_id, node_name, node_sn, json_drivers, json_connections):

        node_name = node_name.encode('unicode_escape').replace('\\u', 'x')

        gen_log.debug('build_thread for node %s app %s' % (node_name, app_num))

        set_build_verbose(False)

        if not gen_and_build(build_phase, app_num, str(user_id), node_sn, node_name, '', json_drivers, json_connections):
            error_msg = get_error_msg()
            result = ("error", error_msg)
        else:
            result = ("ok", "")

        ioloop.IOLoop.current().add_callback(self.post_build, result)

    @gen.coroutine
    def post_build(self, result):

        if result[0] != "ok":
            gen_log.error(result[1])
            self.send_notification(result)
            #delete the failed record
            try:
                cur = self.application.cur
                cur.execute("delete from builds where bld_id=?", (self.cur_build_id, ))
            except Exception,e:
                gen_log.error("Failed to delete the building record: %s" % str(e))
            finally:
                self.application.conn.commit()

            return

        if 2 not in self.build_phase:
            self.send_notification(('done', 'Source file generated.'))
            return

        #update the build end time
        try:
            cur = self.application.cur
            cur.execute("update builds set build_endtime=datetime('now') where bld_id=?", (self.cur_build_id, ))
        except Exception,e:
            gen_log.error("Failed to log the building record: %s" % str(e))
        finally:
            self.application.conn.commit()

        #query the connection status again
        if self.cur_conn not in self.conns:
            cur_conn = None

            for conn in self.conns:
                if conn.sn == self.node_sn and not conn.killed:
                    cur_conn = conn
                    break
            self.cur_conn = cur_conn

        if not self.cur_conn or self.cur_conn not in self.conns:
            gen_log.info('Node is offline, sn: %s, name: %s' % (self.node_sn, self.node_name))
            state = ("error", "Node is offline")
            self.send_notification(state)
            return

        # go OTA
        retries = 0
        while(retries < 3 and not self.cur_conn.killed):
            try:
                state = ("going", "Notifying the node...[%d]" % retries)
                self.send_notification(state)

                self.cur_conn.ota_notify_done_future = Future()

                cmd = "OTA\r\n"
                cmd = cmd.encode("ascii")
                self.cur_conn.submit_cmd (cmd)

                yield gen.with_timeout(timedelta(seconds=10), self.cur_conn.ota_notify_done_future, io_loop=ioloop.IOLoop.current())
                break
            except gen.TimeoutError:
                pass
            except Exception,e:
                gen_log.error(e)
                #save state
                state = ("error", "notify error: "+str(e))
                self.send_notification(state)
                return
            retries = retries + 1

        if retries >= 3:
            state = ("error", "Time out in notifying the node.")
            gen_log.info(state[1])
            self.send_notification(state)
        else:
            gen_log.info("Succeed in notifying node %s." % self.cur_conn.node_id)

    def send_notification(self, state):
        if self.state_waiters and self.node_sn in self.state_waiters and len(self.state_waiters[self.node_sn]) > 0:
            f = self.state_waiters[self.node_sn].pop(0)
            f.set_result(state)
            if len(self.state_waiters[self.node_sn]) == 0:
                del self.state_waiters[self.node_sn]
        elif self.state_happened and self.node_sn in self.state_happened:
            self.state_happened[self.node_sn].append(state)
        else:
            self.state_happened[self.node_sn] = [state]




class OTAStatusReportingHandler(NodeBaseHandler):

    @gen.coroutine
    def post (self):
        self.resp(404, "Please get this url")

    @gen.coroutine
    def get(self):
        gen_log.info("request ota status")

        node = self.get_node()
        if not node:
            return

        self.node_sn = node['node_sn']

        state = None
        state_future = None
        if self.state_happened and self.node_sn in self.state_happened and len(self.state_happened[self.node_sn]) > 0:
            state = self.state_happened[self.node_sn].pop(0)
            if len(self.state_happened[self.node_sn]) == 0:
                del self.state_happened[self.node_sn]
        else:
            state_future = Future()
            if self.state_waiters and self.node_sn in self.state_waiters:
                self.state_waiters[self.node_sn].append(state_future)
            else:
                self.state_waiters[self.node_sn] = [state_future]


        if not state and state_future:
            #print state_future
            try:
                state = yield gen.with_timeout(timedelta(seconds=180), state_future, io_loop=ioloop.IOLoop.current())
            except gen.TimeoutError:
                state = ("error", "Time out when waiting new status.")
            except:
                pass

        gen_log.info("+++send ota state to app:"+ str(state))

        if self.request.connection.stream.closed():
            return

        self.resp(200, meta={'ota_status': state[0], 'ota_msg': state[1]})

    def on_connection_close(self):
        # global_message_buffer.cancel_wait(self.future)
        gen_log.info("on_connection_close")


class OTAFirmwareSendingHandler(BaseHandler):

    def initialize (self, conns, state_waiters, state_happened):
        self.conns = conns
        self.state_waiters = state_waiters
        self.state_happened = state_happened

    def get_node (self):
        node = None
        sn = self.get_argument("sn","")
        if not sn:
            gen_log.error("ota bin request has no sn provided")
            return


        sn = sn[:-4]
        try:
            sn = base64.b64decode(sn)
        except:
            sn = ""

        if len(sn) != 32:
            gen_log.error("ota bin request has no valid sn provided")
            return

        if sn:
            try:
                cur = self.application.cur
                cur.execute('select * from nodes where node_sn="%s"'%sn)
                rows = cur.fetchall()
                if len(rows) > 0:
                    node = rows[0]
            except:
                node = None
        else:
            node = None

        if not node:
            self.resp(403,"Please attach the valid node sn")
        else:
            gen_log.info("get current node, id: %s, name: %s" % (node['node_id'],node["name"]))

        return node


    @gen.coroutine
    def head(self):
        app = self.get_argument("app","")
        if not app or app not in [1,2,"1","2"]:
            gen_log.error("ota bin request has no app number provided")
            return

        node = self.get_node()
        if not node:
            return

        node_sn = node['node_sn']
        user_id = node['user_id']
        node_id = node['node_id']
        self.node_sn = node_sn

        #get the user dir and path of bin
        bin_path = os.path.join("users_build/",str(user_id) + '_' + str(node_sn), "user%s.bin"%str(app))

        #put user*.bin out
        self.set_header("Content-Type","application/octet-stream")
        self.set_header("Content-Transfer-Encoding", "binary")
        self.set_header("Content-Length", os.path.getsize(bin_path))

        self.flush()

    @gen.coroutine
    def get(self):
        app = self.get_argument("app","")
        if not app or app not in [1,2,"1","2"]:
            gen_log.error("ota bin request has no app number provided")
            return

        node = self.get_node()
        if not node:
            return

        node_sn = node['node_sn']
        user_id = node['user_id']
        node_id = node['node_id']
        self.node_sn = node_sn

        #get the user dir and path of bin
        bin_path = os.path.join("users_build/",str(user_id) + '_' + str(node_sn), "user%s.bin"%str(app))

        #put user*.bin out
        self.set_header("Content-Type","application/octet-stream")
        self.set_header("Content-Transfer-Encoding", "binary")
        self.set_header("Content-Length", os.path.getsize(bin_path))

        with open(bin_path,"rb") as file:
            while True:
                try:
                    chunk_size = 64 * 1024
                    chunk = file.read(chunk_size)
                    if chunk:
                        self.write(chunk)
                        yield self.flush()
                    else:
                        gen_log.info("Node %s: firmware bin sent done." % node_id)
                        state = ('going', 'Verifying the firmware...')
                        self.send_notification(state)
                        break
                except Exception,e:
                    gen_log.error('node %s error when sending binary file: %s' % (node_id, str(e)))
                    state = ('error', 'Error when sending binary file. Please retry.')
                    self.send_notification(state)
                    self.clear()
                    break


    def post (self, uri):
        self.resp(404, "Please get this url.")

    def send_notification(self, state):
        if self.state_waiters and self.node_sn in self.state_waiters and len(self.state_waiters[self.node_sn]) > 0:
            f = self.state_waiters[self.node_sn].pop(0)
            f.set_result(state)
            if len(self.state_waiters[self.node_sn]) == 0:
                del self.state_waiters[self.node_sn]
        elif self.state_happened and self.node_sn in self.state_happened:
            self.state_happened[self.node_sn].append(state)
        else:
            self.state_happened[self.node_sn] = [state]

class COTFHandler(NodeBaseHandler):

    @gen.coroutine
    def get (self, uri):
        node = self.get_node()
        if not node:
            return

        user_id = node["user_id"]
        node_name = node["name"]
        node_sn = node["node_sn"]

        cur_dir = os.path.split(os.path.realpath(__file__))[0]
        user_build_dir = cur_dir + '/users_build/' + str(user_id)  + '_' + node_sn
        if not os.path.exists(user_build_dir):
            self.resp(404, "User's working dir not found")
            return

        if uri == 'project':
            # walk through the dir
            file_list = []
            for root, dirs, files in os.walk(user_build_dir):
                for name in files:
                    if re.match(r'.*(?<!_gen)\.(cpp|c|h)', name) and name.find('rpc_server_registration.cpp') < 0:
                        file_list.append(os.path.join(root, name))

            output_json = {}
            for file in file_list:
                with open(file) as f:
                    f_name = file.replace(user_build_dir, '.')
                    output_json[f_name] = f.read()

            self.resp(200, meta=output_json)
            return


        self.resp(404, "API endpoint not found")

    @gen.coroutine
    def post (self, uri):
        node = self.get_node()
        if not node:
            return

        user_id = node["user_id"]
        node_name = node["name"]
        node_sn = node["node_sn"]

        cur_dir = os.path.split(os.path.realpath(__file__))[0]
        user_build_dir = cur_dir + '/users_build/' + str(user_id)  + '_' + node_sn
        if not os.path.exists(user_build_dir):
            self.resp(404, "User's working dir not found")
            return

        if uri == 'project':
            if self.request.headers.get("content-type") and self.request.headers.get("content-type").find("json") > 0:
                gen_log.debug(self.request.body)

                project_src = None
                try:
                    project_src = json.loads(self.request.body)
                except Exception,e:
                    self.resp(400, 'invalid json: ' + str(e))
                    return

                if type(project_src) != dict:
                    self.resp(400, 'json is not dict')
                    return

                for k,v in project_src.items():
                    if k.find('..') > 0:
                        self.resp(400, 'please dont!!!')
                        return
                    if k.startswith('./'):
                        pass
                    elif k.startswith('/'):
                        k = '.' + k
                    else:
                        k = './' + k

                    k = os.path.join(user_build_dir, k.lstrip('./'))

                    d = os.path.dirname(k)
                    if not os.path.exists(d): os.makedirs(d)

                    with open(k, 'w') as f:
                        f.write(v)

                self.resp(200)

            else:
                self.resp(400, "Please post the request in json")

            return


        self.resp(404, "API endpoint not found")
