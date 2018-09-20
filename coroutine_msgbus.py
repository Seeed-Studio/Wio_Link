
#   Copyright (C) 2018 Seeed Technology Co., Ltd.
#   Author: Jack Shao (jack.shaoxg@gmail.com)
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


from tornado import gen, ioloop
from tornado.concurrent import Future
from tornado.locks import BoundedSemaphore
from tornado.queues import Queue, QueueFull


class BadCommandError(Exception):
    pass


class OfflineError(Exception):
    pass


class EventTopic(object):
    def __init__(self, topic_name, maxsize=10):
        self.topic_name = topic_name
        self.maxsize = maxsize
        self.q_list = []

    def create_queue(self):
        q = Queue(self.maxsize)
        self.q_list.append(q)
        return q

    def delete_queue(self, q):
        if q in self.q_list:
            q.put_nowait(None)
            self.q_list.remove(q)
        else:
            raise Exception('Queue not found')

    def broadcast(self, message):
        for q in self.q_list:
            try:
                q.put_nowait(message)
            except:
                pass

    def queue_count(self):
        return len(self.q_list)


class TopicCollection(object):
    pass


class CoEventBus(object):
    """
    This module creates a pub-sub model between tornado coroutines
    """

    topic_pool = {}

    def __init__(self):
        pass

    def listener(self, topic):
        """
        this method creates a topic if it's not existing, then creates a coroutine Queue, and returns
        the Queue's reference.
        """
        if topic.find('#') >= 0:
            ### this is a wildcard topic
            raise Exception("not implemented")
        else:
            ### this is a single topic
            if topic not in CoEventBus.topic_pool:
                topic_instance = EventTopic(topic)
                CoEventBus.topic_pool[topic] = topic_instance
                return topic_instance
            else:
                return CoEventBus.topic_pool[topic]

    def broadcast(self, topic, message):
        """
        this method returs the reference to the topic if it's existing, otherwise it returns the
        reference of the CoMessageBus instance. With a topic broadcaster, one can broadcast to
        a topic with the broadcast() method.
        - If the topic does not exist, the broadcast() method
        will be the fake one of CoMessageBus, which will do nothing.
        - If the topic exists, the broadcast() method will send the message to each listening Queue
        of that topic. If that topic has no listening Queue, that topic will be destroyed.
        """
        if topic.find('#') >= 0:
            ### this is a wildcard topic
            raise Exception("not implemented")
        else:
            ### this is a single topic
            if topic in CoEventBus.topic_pool:
                topic_instance = CoEventBus.topic_pool[topic]
                if topic_instance.queue_count() == 0:
                    topic_instance = None
                    del CoEventBus.topic_pool[topic]
                else:
                    topic_instance.broadcast(message)


def _MidGenerator():
    counter = 0
    while True:
        yield counter
        counter += 1
        if counter > (1 << 30):
            counter = 0


class BoundedSemaphoreWithValue(BoundedSemaphore):
    def value(self):
        return self._value


class CommandTopic(object):
    def __init__(self, topic_name, maxsize=10):
        self.topic_name = topic_name
        self.q_maxsize = maxsize
        self.q = Queue(maxsize=maxsize)
        self.sem = BoundedSemaphoreWithValue(1)
        self.mid_gen = _MidGenerator()
        self.gc_flag = False
        self.io_loop = ioloop.IOLoop.current()

    def issue(self, ft, cmd, block, timeout):
        if 'mid' not in cmd:
            cmd['mid'] = next(self.mid_gen)
        try:
            h_timeout = self.io_loop.call_later(timeout, self.cmd_timeout, ft, cmd)
            ft.add_done_callback(lambda _: self.io_loop.remove_timeout(h_timeout))

            # [ft, cmd, block]
            self.q.put_nowait([ft, cmd, block])
        except Exception as e:
            ft.set_exception(e)

        return ft

    def cmd_timeout(self, ft, cmd):
        ft.set_exception(gen.TimeoutError('Timeout in waiting response of command: {}'.format(str(cmd))))

    @gen.coroutine
    def start_cmd(self):
        yield self.sem.acquire()
        item = yield self.q.get()
        if not item:
            self.sem.release()
            raise gen.Return(None)
        self.queue_item_under_process = item
        ft, cmd, block = item

        # a special case should be handled:
        # the network is very slow so the future timed out before semaphore acquirement is done
        if ft.done():
            raise gen.Return(None)

        ft.add_done_callback(lambda _: self.sem.release())

        if not block:  # notification type cmd, dont wait response
            ft.set_result(None)

        raise gen.Return(cmd)

    def finish_cmd(self, mid, resp):
        ft, cmd, block = self.queue_item_under_process

        if not ft.done() and mid == cmd['mid']:
            ft.set_result(resp)

    def start_listen(self):
        if self.sem.value() == 0:
            self.q.put(None)
        self.gc_flag = False

    def stop_listen(self):
        if hasattr(self, 'pending_item'):
            ft, cmd, block = self.queue_item_under_process
            if not ft.done():
                ft.set_result(None)
        self.q.put(None)
        self.gc_flag = True

    def is_marked_gc(self):
        return self.gc_flag


class CoCommandBus(object):
    topic_pool = {}

    def __init__(self):
        pass

    def listener(self, topic):
        if topic.find('#') >= 0:
            ### this is a wildcard topic
            raise Exception("not implemented")
        else:
            ### this is a single topic
            if topic not in CoCommandBus.topic_pool:
                topic_instance = CommandTopic(topic)
                CoCommandBus.topic_pool[topic] = topic_instance
                return topic_instance
            else:
                topic_instance = CoCommandBus.topic_pool[topic]
                topic_instance.start_listen()

                return topic_instance

    def issue_command(self, topic, cmd, block=True, timeout=5):
        ft = Future()
        if topic.find('#') >= 0:
            ### this is a wildcard topic
            ft.set_exception(Exception("not implemented"))
        else:
            ### this is a single topic
            if topic in CoCommandBus.topic_pool:
                topic_instance = CoCommandBus.topic_pool[topic]
                if topic_instance.is_marked_gc():
                    topic_instance = None
                    del CoCommandBus.topic_pool[topic]
                    ft.set_exception(OfflineError())
                else:
                    if not isinstance(cmd, dict):
                        ft.set_exception(BadCommandError())
                    else:
                        topic_instance.issue(ft, cmd, block, timeout)
            else:
                ft.set_exception(OfflineError())

        return ft

