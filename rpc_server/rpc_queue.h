/*
 * rpc_queue.h
 *
 * Copyright (c) 2012 seeed technology inc.
 * Website    : www.seeed.cc
 * Author     : Jack Shao (jacky.shaoxg@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef __RPC_Q_H__
#define __RPC_Q_H__

template<class Type> class QueueItem;

template<class Type>
class Queue
{
public:
    Queue() : front(0), back(0), _max_size(100) { }
    Queue(int max_size) : front(0), back(0), _max_size(max_size) { }
    ~Queue() { }

    bool push(Type);
    bool pop(Type *);
    bool is_empty() const
    {
        return front == 0;
    }
    int get_size() const
    {
        return _size;
    }

private:
    QueueItem<Type> *front;
    QueueItem<Type> *back;
    
    int _size;
    int _max_size;
};

template<class Type>
class QueueItem
{
public:
    QueueItem(Type val) { item = val; next = 0;}
    friend class Queue<Type>;

private:
    Type item;
    QueueItem *next;
};

template<class Type>
bool Queue<Type>::push(Type val)
{
    if (_size >= _max_size)
    {
        return false;
    }
    QueueItem<Type> *pt = new QueueItem<Type>(val);
    if (is_empty()) front = back = pt;
    else
    {
        back->next = pt;
        back = pt;
    }
    _size++;
    
    return true;
}

template<class Type>
bool Queue<Type>::pop(Type *p_val)
{
    if (is_empty())
    {
        return false;
    }
    QueueItem<Type> *pt = front;
    front = front->next;
    *p_val = pt->item;
    delete pt;
    _size--;
    
    return true;
}

#endif

