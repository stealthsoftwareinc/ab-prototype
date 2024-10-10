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
#include "network_connection.hpp"
#include <NTL/ZZ.h>

namespace rm_net
{
  class client_interface
  {
    public:

      int local_partyID = -1;
      int remote_partyID = -1;

      client_interface()
      {
      }

      virtual ~client_interface()
      {
        // when the client interface is destroyed, then it disconnects from server.
        disconnect();
      }

      // connect to server upon server's listening ip and port
      bool connect(const std::string& host, const std::string& port)
      {
        try{
          // resolve the ip and port to connect
          boost::asio::ip::tcp::resolver resolver(my_context);

          // create endpoints based on resolver on host/port
          auto my_endpoints = resolver.resolve(host, port);

          // create a connection object
          my_connection = std::make_shared<connection>(
            connection::owner::client,
            my_context,
            boost::asio::ip::tcp::socket(my_context),
            incoming_queue
          );

          my_connection->local_partyID = local_partyID;
          my_connection->remote_partyID = remote_partyID;

          // connect to server with the endpoint
          my_connection->connect_to_server(my_endpoints);

          // start context thread
          context_thread = std::thread([this]() {my_context.run();});
        }
        catch (std::exception& e)
        {
          std::cerr << "Client Connection Error: " << e.what() << std::endl;
          return false;
        }
        return true;
      }

      // disconnect server
      void disconnect()
      {
        if (is_connected())
        {
          my_connection->disconnect();
        }
        // stop the io_context
        my_context.stop();
        // stop the thread
        if(context_thread.joinable())
          context_thread.join();
        // release the pointer
        //my_connection.release();
      }

      // return 1 if and only if a connection is valid
      bool is_connected()
      {
        if(my_connection)
          return my_connection->is_connected();
        else
          return false;
      }

      // return 1 if and only if the incoming queue is empty
      bool is_incoming_empty()
      {
        return incoming_queue.is_empty();
      }

      void send_message(const message& msg)
			{
				if (is_connected())
					 my_connection->send_message(msg); 
			}

      async_queue<received_message>& access_to_incoming_queue()
      {
        return incoming_queue;
      }

    protected:
      // asio io context
      boost::asio::io_context my_context;
      
      // thread for its own work in context
      std::thread context_thread;
      
      // connection pointer (as a shared pointer)
      std::shared_ptr<connection> my_connection;

    private: // needs to be private?
      // queue owned by the client 
      async_queue<received_message> incoming_queue;
  };
}
