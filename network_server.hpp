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

namespace rm_net
{
  class server_interface
  {
    public:
      // constructor
      server_interface(uint16_t port): my_acceptor(my_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(),port))
      {

      }

      // destructor
      virtual ~server_interface()
      {
        stop();
      }

      bool start()
      {
        try
        {
          wait_for_connection();
          my_thread = std::thread([this]() { my_context.run(); });
        }
        catch (std::exception& e)
        {
          std::cerr << "Server exception thrown: " << e.what() << std::endl;
          return false;
        }
        std::cout << "Server started ..." << std::endl;
        return true;
      }

      void stop()
      {
        my_context.stop();
        if(my_thread.joinable())
          my_thread.join();

        std::cout << "Server terminated ..." << std::endl;
      }

      // asio - wait for connection
      void wait_for_connection()
      {
        my_acceptor.async_accept(
          [this](std::error_code ec, boost::asio::ip::tcp::socket socket)
          {
            if(!ec)
            {
              /** OPTION For NO_DELAY **/
              boost::asio::ip::tcp::no_delay option(true);
              socket.set_option(option);
              std::cout << "Accepted connection: " << socket.remote_endpoint() << std::endl;
              std::shared_ptr<connection> newconn = 
                    std::make_shared<connection>(connection::owner::server, 
                                                 my_context, 
                                                 std::move(socket), 
                                                 my_received_messages);

              if (upon_connection(newconn))
              {
                my_connections.push_back(std::move(newconn));
                my_connections.back()->connect_to_client(id_counter++);
                std::cout << "[" << my_connections.back()->GetID() << "] connection established\n";
              }
              else
              {
                std::cout << "** Connection denied" << std::endl;
              }
            }
            else
            {
              std::cout << "New connection error: " << ec.message() << std::endl;
            }
            wait_for_connection();
          }
          );
      }

      // send a message to a client - RM does not need for now
      void send_message_to_client(std::shared_ptr<connection> client, const message& msg)
      {
        if (client->is_connected())
        {
          client->send_message(msg);
        }
        else
        {
          upon_disconnection(client);
          client.reset();
          my_connections.erase(
            std::remove(my_connections.begin(), my_connections.end(), client),
                        my_connections.end());
        }
      }

      // send a message to all client - RM does not need for now
      void send_message_to_all_clients(const message& msg, std::shared_ptr<connection> ignored_client = nullptr)
      {
        bool invalidClientExists = false;
        for (auto& client : my_connections)
        {
          if(client->is_connected())
          {
            client->send_message(msg);
          }
          else
          {
            upon_disconnection(client);
            client.reset();
          }
        }
        if (invalidClientExists)
        {
          my_connections.erase(
            std::remove(my_connections.begin(), my_connections.end(), ignored_client),
                        my_connections.end());
        }
      }

      void update(const rm_info& info, 
                  rm_net::async_queue<rm_net::deserialized_message>& deserialized_msgs, 
                  size_t max_messages = -1) // -1 is the max number
      {
        size_t message_count = 0;
        while (message_count < max_messages && !my_received_messages.is_empty())
        {
          auto rec_msg = my_received_messages.pop_front();
          // sanitization: to be removed
          prepare_message(deserialized_msgs, rec_msg, info);
          message_count ++;
        }
      }

    protected:
      // called when a client connects (false = rejection of connection)
      virtual bool upon_connection(std::shared_ptr<connection> client)
      {
        return client->is_connected();
      }

      // called when a client disconnects
      virtual void upon_disconnection(std::shared_ptr<connection> client)
      {
        std::cout << "Client[" << client->GetID() << "] disconnected\n";
      }

      // called when a message is received.
      // msg will be deserialized and stored into deserialized_msgs
      virtual void prepare_message(
        rm_net::async_queue<rm_net::deserialized_message>& deserialized_msgs, 
        rm_net::received_message& rec_msg, 
        const rm_info& info)
      {
        deserialized_message temp;
        temp.sid = rec_msg.msg.header.sid;
        temp.tot_num_blocks = rec_msg.msg.header.tot_num_blocks;
        temp.block_idx = rec_msg.msg.header.block_idx;
        temp.mixing_state_id = rec_msg.msg.header.mixing_state_id;
        temp.sender_id = rec_msg.msg.header.sender_id;
        temp.conn = rec_msg.conn;
        temp.body.SetLength(1);
        deserialize_to_vec_ZZ_p(temp.body[0], rec_msg.msg, info.fft_prime_info.prime);
        assert(temp.body[0].length() == rec_msg.msg.header.num_ZZ_p);
        deserialized_msgs.push_back(std::move(temp));
      }

    protected:

      // asio
      boost::asio::io_context my_context;
      std::thread my_thread;

      // ts queue for received messages
      async_queue<received_message> my_received_messages;
      std::deque<std::shared_ptr<connection>> my_connections;

      // acceptor
      boost::asio::ip::tcp::acceptor my_acceptor;

      // other servers will be identified via an ID
      uint32_t id_counter = 10000;

  };
}
