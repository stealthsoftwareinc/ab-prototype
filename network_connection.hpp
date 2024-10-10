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
#include "network_ts_queue.hpp"
#include "network_message.hpp"

namespace rm_net
{
  class connection : public std::enable_shared_from_this<connection>
  {
    public:
      enum class owner
      {
        server,
        client
      };

      int local_partyID = -1;
      int remote_partyID = -1;

      // constructor
      connection(owner parent, 
                 boost::asio::io_context& asioContext, 
                 boost::asio::ip::tcp::socket socket,
                 async_queue<received_message>& rec_messages)
      : context(asioContext), socket(std::move(socket)), incoming_queue(rec_messages)
      {
        owner_type = parent;
      }

      // destructor
      virtual ~connection() {}

      // return client's id
      uint32_t GetID()
      {
        return id;
      }

    public:
      void connect_to_client(uint32_t uid = 0)
      {
        if(owner_type == owner::server)
        {
          if(socket.is_open())
          {
            id = uid;
            boost::asio::post(context, [this]() { read_header(); });
          }
        }
      }

      // connect to a server with endpoints
      void connect_to_server(const boost::asio::ip::tcp::resolver::results_type& endpoints)
      {
        if (owner_type == owner::client)
        {
          boost::asio::connect(socket, endpoints);
          boost::asio::post(context, [this]() { read_header(); });
        }
      }

      // disconnect the connection
      void disconnect()
      {
        if (is_connected())
        {
          boost::asio::post(context,
          [this]()
          {
            socket.close();
          });
        }
      }

      bool is_connected() const
      {
        return socket.is_open();
      }

    public:
      void send_message(const message& msg)
      {
        outgoing_queue.push_back(msg);
        boost::asio::post(context, [this]() { write_header(); });
      }

    private:

    // asio - read a message header
    void read_header()
    {
      boost::asio::async_read(
            socket, 
            boost::asio::buffer(&temp_msg.header, sizeof(message_header)),
      [this](std::error_code ec, std::size_t lenght)
      {
        if(!ec)
        {
          if (temp_msg.header.size > 0)
          {
            temp_msg.body.resize(temp_msg.header.size);
            read_body();
          }
          else
          {
            Add_to_Incoming_Queue();
          }
        }
        else
        {
          std::cout << "[" << id << "] Reading header failed.\n";
          socket.close();
        }
      });
    }

    // asio - read a message body
    void read_body()
    {
      boost::asio::async_read(
            socket,
            boost::asio::buffer(temp_msg.body.data(),temp_msg.header.size),
      [this](std::error_code ec, std::size_t length)
      {
        if (!ec)
        {
          Add_to_Incoming_Queue();
        }
        else
        {
          std::cout << "[" << id << "] Reading body failed.\n";
          socket.close();
        }
      });
    }

    // asio - write a message header
    void write_header() 
    {
      if (already_writing_ || outgoing_queue.is_empty()) {
        return;
      }
      already_writing_ = true;
      boost::asio::async_write(
        socket, 
        boost::asio::buffer(&outgoing_queue.front().header, 
        sizeof(message_header)),
        [this](std::error_code ec, std::size_t length)
        {
          if(!ec)
          {
            std::stringstream s;
            s << "[TEST HEADER]:" << local_partyID << "->" << remote_partyID << ": " << length << "\n";
            std::cout << s.str();
            if(outgoing_queue.front().header.size > 0)
            {
              write_body();
            }
            else
            {
              outgoing_queue.pop_front();
              already_writing_ = false;
              write_header();
            }
          }
          else
          {
            std::cout << "[" << id << "] writing header failed. \n";
            socket.close();
          }
        }
      );
    }

    // asio - write a message body
    void write_body()
    {
      boost::asio::async_write(
        socket, 
        boost::asio::buffer(outgoing_queue.front().body.data(), 
        outgoing_queue.front().header.size),
        [this](std::error_code ec, std::size_t length)
        {
          if(!ec)
          {
            std::stringstream s;
            s << "[TEST BODY]:" << local_partyID << "->" << remote_partyID << ": " << length << "\n";
            std::cout << s.str();
            outgoing_queue.pop_front();
            already_writing_ = false;
            write_header();
          }
          else
          {
            std::cout << "[" << id << "] writing body failed. \n";
            socket.close();
          }
        }
      );
    }

    void Add_to_Incoming_Queue()
    {
      received_message temp;
      temp.conn = this->shared_from_this();
      temp.msg = temp_msg;
      incoming_queue.push_back(temp);

      // now after adding a msg to queue, read header again
      read_header();
    }

    protected:
      boost::asio::ip::tcp::socket socket;

      // the context is provided by clients/servers and shared with the entire asio instance
      boost::asio::io_context& context; 

      // the queue containing all messages to be sent via this connection
      async_queue<message> outgoing_queue;

      // the queue holding all messages received from this connection.
      // the queue will be provided by its client/server.
      async_queue<received_message>& incoming_queue;

      // temporary message variable
      message temp_msg;

      // two types
      owner owner_type = owner::server;

      // connection id
      uint32_t id = 0;

      bool already_writing_ = false;
  };
}
