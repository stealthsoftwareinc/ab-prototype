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
#include <NTL/ZZ.h>
#include <NTL/ZZ_p.h>
#include <NTL/vector.h>
#include <NTL/vec_ZZ_p.h>
#include <NTL/vec_vec_ZZ_p.h>


namespace rm_net
{
  struct message_header
  {
    uint32_t sid; // session sid
    uint32_t sender_id; // sender identifier
    uint16_t mixing_state_id; // specify the correponding state 
    uint16_t block_idx; // the index of the current block (<= total_blocks)
    uint16_t tot_num_blocks; // if not 1, there are multiple blocks of messages for the mixing state id.
    uint16_t dimension; // the body dimenion. 1 = vector, 2 = vector of vector
    uint16_t num_ZZ_p; // the number of ZZ_p elements in each row
    uint32_t size; // message body size in bytes
    std::chrono::system_clock::time_point time; // possibly 
  };

  // struct message and its memeber function for serialization and deserialization
  struct message
  {
    message_header header;
    std::vector<unsigned char> body;

    // a member function to return the size in bytes
    size_t size() const 
    {
      return sizeof(message_header) + body.size();
    }

    // Override operator << to output message's header info
    friend std::ostream& operator << (std::ostream& os, const message& msg)
    {
      os << "Session ID: " << msg.header.sid << std::endl
         << "Sender_id: " << msg.header.sender_id << std::endl
         << "Mix State: " << msg.header.mixing_state_id << std::endl
         << "Block Idx: " << msg.header.block_idx << std::endl
         << "Total # of Blocks: " << msg.header.tot_num_blocks << std::endl
         << "Dimension: " << msg.header.dimension << std::endl
         << "Body Size (ZZ_p): " << msg.header.num_ZZ_p << std::endl
         << "Body Size (Bytes): " << msg.header.size << std::endl;
      return os;
    }

    // serialize ZZ_p into message body
    friend void serialize_from_ZZ_p (message& msg, const NTL::ZZ_p& data, const NTL::ZZ& prime)
    {
      long nbytes = NTL::NumBytes(prime);
      std::string buf(nbytes, 0);

      // current size of vector
      size_t i = msg.body.size();

      // resize the vector by the size of ZZ_p value
      msg.body.resize(msg.body.size() + buf.size());

      // physically copy a ZZ_p into body
      NTL::BytesFromZZ(msg.body.data()+i, 
                       NTL::conv<NTL::ZZ>(data), 
                       buf.size());
      //std::cout << nbytes << " bytes of data added into message body\n";

      // update the body size
      msg.header.size = msg.body.size();
    }

    // serialize vec_ZZ_p into message body in the order from the last to the first elements
    friend void serialize_from_vec_ZZ_p (message& msg, const NTL::vec_ZZ_p& input, const NTL::ZZ& prime)
    {
      size_t len = input.length();
      msg.header.num_ZZ_p = len;
      for(size_t i = len ; i != 0 ; i--)
      {
        serialize_from_ZZ_p(msg, input[i-1], prime);
      }
    }

    // deserialize to a ZZ_p value 
    friend void deserialize_to_ZZ_p(NTL::ZZ_p& data, message& msg, const NTL::ZZ& prime)
    {
      long nbytes = NTL::NumBytes(prime);

      // current size of vector
      size_t i = msg.body.size() - nbytes;

      data = NTL::conv<NTL::ZZ_p>(NTL::ZZFromBytes(msg.body.data()+i, nbytes));
      
      // Resize to remove read bytes
      msg.body.resize(i);

      // update the body size
      msg.header.size = msg.size();
    }

    friend void deserialize_to_vec_ZZ_p(NTL::vec_ZZ_p& output_vec, message& msg, const NTL::ZZ& prime)
    {
      output_vec.SetLength(msg.header.num_ZZ_p);
      for (size_t i = 0 ; i != msg.header.num_ZZ_p ; i++)
      {
        deserialize_to_ZZ_p(output_vec[i], msg, prime);
      }
      //std::cout << "******************************\n";
      //std::cout << "Deserialization of Recieved MSG\n";
      //std::cout << msg;
      //std::cout << output_vec.length() << " elements are deserialized and retrived.\n";
      //std::cout << "******************************\n";
    }
  };

  class connection;// forward declaration

  struct received_message
  {
    std::shared_ptr<connection> conn = nullptr;
    message msg;

    friend std::ostream& operator << (std::ostream& os, const received_message& msg)
    {
      os << msg.msg;
      return os;
    }
  };

  struct deserialized_message 
  {
    uint32_t sid;
    uint16_t mixing_state_id; // specify the correponding state. If 0, message is a client msg submission
    uint32_t sender_id;
    uint16_t block_idx; // the index of the current block (<= total_blocks)
    uint16_t tot_num_blocks; // if not 1, there are multiple blocks of messages for the mixing state id.
    std::shared_ptr<connection> conn = nullptr; // sender's connection (to be used only to send back a message to clients)
    NTL::vec_vec_ZZ_p body;
  };
}
