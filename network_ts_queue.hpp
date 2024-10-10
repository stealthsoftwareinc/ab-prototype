/*
#
# Copyright (C) 2024 Stealth Software Technologies, Inc.
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation
# files (the "Software"), to deal in the Software without
# restriction, including without limitation the rights to use,
# copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following
# conditions:
#
# The above copyright notice and this permission notice (including
# the next paragraph) shall be included in all copies or
# substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
# OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
# HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
# WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
# OTHER DEALINGS IN THE SOFTWARE.
#
# SPDX-License-Identifier: MIT
#
*/
#pragma once
#include "network_common.hpp"
#include "network_message.hpp"

namespace rm_net
{
  template<typename T> class async_queue
  {
    public:
      async_queue() = default;
      async_queue(const std::deque<T>&) = delete;
      virtual ~async_queue() { clear(); }

    public:

      // returns and holds a message at the front of the queue
      const T& front()
      {
        std::scoped_lock lock(mtxQueue);
        return deqQueue.front();
      }

      // add a message to the back of the queue
      void push_back(const T& item)
      {
        std::scoped_lock lock(mtxQueue);
        deqQueue.emplace_back(std::move(item));
      }

      // returns 1 if and only if the queue is empty
      bool is_empty()
      {
        std::scoped_lock lock(mtxQueue);
        return deqQueue.empty();
      }

      // returns the number of messages currently in the queue
      size_t count()
      {
        std::scoped_lock lock(mtxQueue);
        return deqQueue.size();
      }

      // clears out the queue
      void clear()
      {
        std::scoped_lock lock(mtxQueue);
        deqQueue.clear();
      }

      // returns and removes the first message from the queue
      T pop_front()
      {
        std::scoped_lock lock(mtxQueue);
        auto temp = std::move(deqQueue.front());
        deqQueue.pop_front();
        return temp;
      }

    protected:
      std::mutex mtxQueue;
      std::deque<T> deqQueue;
  };
}