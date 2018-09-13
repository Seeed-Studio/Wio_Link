#   Copyright (C) 2018 Seeed
#   Author: Jack Shao (jack.shaoxg@gmail.com)
#
#   Expiring in-memory cache module
#

from collections import OrderedDict
from time import time

from tornado import ioloop
from tornado.log import *


class CacheException(Exception):
    """
    Generic cache exception
    """
    pass


class CachedObject(object):
    def __init__(self, name, obj, ttl):
        """
        Initializes a new cached object

        Args:
            name               (str): Human readable name for the cached entry
            obj               (type): Object to be cached
            ttl                (int): The TTL in seconds for the cached object

        """
        self.hits = 0
        self.name = name
        self.obj = obj
        self.ttl = ttl
        self.timestamp = time()


class CacheInventory(object):
    """
    Inventory for cached objects

    """

    def __init__(self, maxsize=0, housekeeping=0):
        """
        Initializes a new cache inventory

        Args:
            maxsize      (int): Upperbound limit on the number of items
                                that will be stored in the cache inventory
            housekeeping (int): Time in seconds to perform periodic cache housekeeping

        """
        if maxsize < 0:
            raise CacheException('Cache inventory size cannot be negative')

        if housekeeping < 0:
            raise CacheException('Cache housekeeping period cannot be negative')

        self._cache = OrderedDict()
        self.maxsize = maxsize
        self.housekeeping = housekeeping

        if self.housekeeping > 0:
            self._timer = ioloop.PeriodicCallback(self.housekeeper, self.housekeeping * 1000)
            self._timer.start()

    def __len__(self):
        return len(self._cache)

    def __contains__(self, key):
        if key not in self._cache:
            return False

        item = self._cache[key]
        if self._has_expired(item):
            return False
        return True

    def _has_expired(self, item):
        """
        Checks if a cached item has expired and removes it if needed

        If the upperbound limit has been reached then the last item
        is being removed from the inventory.

        Args:
            item (CachedObject): A cached object to lookup

        """
        if item.ttl == 0:
            return False
        if time() > item.timestamp + item.ttl:
            gen_log.debug(
                    'Object %s has expired and will be removed from cache [hits %d]',
                    item.name,
                    item.hits
            )
            self._cache.pop(item.name)
            return True
        return False

    def add(self, key, obj, ttl=0):
        """
        Add an item to the cache inventory

        Args:
            obj (CachedObject): A CachedObject instance to be added

        Raises:
            CacheException

        """
        item = CachedObject(key, obj, ttl)

        if self.maxsize > 0 and len(self._cache) == self.maxsize:
            popped = self._cache.popitem(last=False)
            gen_log.debug('Cache maxsize reached, removing %s [hits %d]', popped.name, popped.hits)

        gen_log.debug('Caching object %s [ttl: %d seconds]', item.name, item.ttl)
        self._cache[item.name] = item

    def get(self, key):
        """
        Retrieve an object from the cache inventory

        Args:
            key (str): Name of the cache item to retrieve

        Returns:
            The cached object if found, None otherwise

        """
        if key not in self._cache:
            return None

        item = self._cache[key]
        if self._has_expired(item):
            return None

        item.hits += 1
        gen_log.debug(
                'Returning object %s from cache [hits %d]',
                item.name,
                item.hits
        )

        return item.obj

    def housekeeper(self):
        """
        Remove expired entries from the cache on regular basis

        """
        total = len(self._cache)
        expired = 0
        for name, item in self._cache.items():
            if self._has_expired(item):
                expired += 1
        gen_log.debug('Cache housekeeper completed [%d total] [%d removed]', total, expired)

