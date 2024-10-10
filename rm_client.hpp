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
#include <NTL/ZZ.h>
#include <NTL/ZZ_p.h>
#include <NTL/vector.h>

/* MPC Networking Libraries */
#include "network_common.hpp"
#include "network_message.hpp"
#include "network_ts_queue.hpp"
#include "network_connection.hpp"
#include "network_client.hpp"

class rm_client: public rm_net::client_interface
{
  public:
    //constructor
    rm_client(){};

    //
    ~rm_client(){};

  public:

    std::chrono::system_clock::time_point time1;

    void submit_message(
      const NTL::vec_ZZ_p& vec, 
      const rm_info& info, 
      const uint32_t& sid,
      const size_t& my_id)
    {
      rm_net::message msg;
      msg.header.sid = sid;
      msg.header.sender_id = my_id;
      msg.header.mixing_state_id = 0;
      msg.header.block_idx = 1;
      msg.header.tot_num_blocks = 1;
      msg.header.dimension = 1;
      msg.header.num_ZZ_p = vec.length();
      serialize_from_vec_ZZ_p(msg, vec, info.fft_prime_info.prime);
      msg.header.time = std::chrono::system_clock::now();
      time1 = msg.header.time;
      //std::cout << "**** Sending Message ****\n"; // Print out info
      //std::cout << msg; // Print out info
      send_message(msg);
      //std::cout << "*** Sending Completed ***\n"; // Print out info
    }

    void send_vector(
      const NTL::vec_ZZ_p& vec,  // a vector of ZZ_p to be transmitted
      const rm_info& info,
      uint32_t in_sid,
      uint16_t in_state,
      uint16_t in_block_idx,
      uint16_t in_tot_num_blocks,
      uint16_t in_dimension
      )
    {
      rm_net::message msg;
      msg.header.sid = in_sid;
      msg.header.sender_id = info.server_id;
      msg.header.mixing_state_id = in_state;
      msg.header.block_idx = in_block_idx;
      msg.header.tot_num_blocks = in_tot_num_blocks; 
      msg.header.dimension = in_dimension; 
      msg.header.num_ZZ_p = vec.length();
      serialize_from_vec_ZZ_p(msg, vec, info.fft_prime_info.prime);
      //std::cout << "**** Sending Message ****\n"; // Print out info
      //std::cout << msg; // Print out info
      send_message(msg);
      //std::cout << "*** Sending Completed ***\n"; // Print out info
    }

  private:
    
};

