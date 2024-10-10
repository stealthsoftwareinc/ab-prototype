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
/* MPC Networking Libraries */
#include "network_common.hpp"
#include "network_message.hpp"
#include "network_ts_queue.hpp"
#include "network_connection.hpp"
#include "network_client.hpp"

/* Standard Libraries */
#include <assert.h>
#include <fstream>
#include <ratio>
#include <utility>
#include <functional>

/* NTL Libraries */
#include <NTL/ZZ.h>
#include <NTL/ZZ_p.h>
#include <NTL/vector.h>
#include <NTL/vec_ZZ_p.h>

/* RM Libraries */
#include "secretsharing.h"
#include "rm_additive2basis.h"
#include "root_finding.h"
#include "rm_common.hpp"
#include "rm_client.hpp"

int main(int argc, char *argv[]){
  // if true, the client is running in test mode, submitting messages of different configuration.
  // if false, the client is running in normal mode with configuration determined by config files.
  bool test_mode = true; 
  uint32_t sid = 0;

  if(argc != 4){
    std::cout << "Configuration Files Required as Follows:" << std::endl;
    std::cout << "(1) mpc configuration" << std::endl;
    std::cout << "(2) mix configuration" << std::endl;
    std::cout << "(3) network configuration" << std::endl;
    std::cout << "Usage: ./rm_server configs/mix_config configs/mpc_config configs/net_config" << std::endl;
    return 1;
  }
  std::string filename1(argv[1]);
  std::string filename2(argv[2]);
  std::string filename3(argv[3]);
  std::ifstream fin1;
  std::ifstream fin2;
  std::ifstream fin3;
  fin1.open(filename1); // mpc config
  fin2.open(filename2); // mix config
  fin3.open(filename3); // net config

  if(!fin1.is_open() || !fin2.is_open() || !fin3.is_open()){
    std::cout << "Reading configuration file failed" << std::endl;
    return 1;
  }

  std::string read_param;
  rm_info info;

  /* Prime Modulus Setup */
  fin1 >> read_param; // reading prime length
  if(!fft_prime_from_bit_length(info.fft_prime_info, std::stoi(read_param))){
    std::cerr << "Prime Length is Invalid" << std::endl;
    return 1;
  }
  NTL::ZZ_p::init(info.fft_prime_info.prime);

  /* MPC servers */
  fin1 >> read_param; // reading the number of servers
  info.n = (size_t) std::stoi(read_param); // the number of servers
  info.t; // the corruption threshold (e.g., the max number of corrupted servers)
  if(info.n%4 != 0){
    info.t = info.n / 4;
  }
  else{
    info.t = (info.n - 1) / 4;
  }
  info.server_id = 0;
  info.l = 1; // packing param

  std::cout << "prime: "  << info.fft_prime_info.prime << std::endl;
  std::cout << "Number of Servers: "  << info.n << std::endl;
  std::cout << "Max Number of Corrupted Servers: "  << info.t << std::endl;
  std::cout << "Share Packing Size: "  << info.l << std::endl;

  /* Mix Parameter Configuration */
  fin2 >> read_param;
  info.L = (size_t) std::stoi(read_param); 
  info.N = 14 * pow(info.L, 2)+ 10 * info.L - 1; // the number of messages to be mixed mixes
  std::cout << "The number of messages in an epoch: "  << info.N << std::endl;

  /* Network Parameters */
  std::vector<std::string> IPs;
  std::vector<std::string> ports;
  std::string temp_IP;
  std::string temp_port;
  for(size_t i = 0 ; i != info.n ; i++){
    if (fin3.eof()){
      cerr << "Incorrect Network Configuration" << endl;
      return 1;
    }
    fin3 >> temp_IP;
    if (fin3.eof()){
      cerr << "Incorrect Network Configuration" << endl;
      return 1;
    }
    fin3 >> temp_port;
    IPs.push_back(temp_IP);
    ports.push_back(temp_port);
  }

  for(size_t i = 0 ; i != info.n ; i++){
    std::cout << "Server[" << i+1 << "]'s IP/Port: " << IPs[i] + "/" + ports[i] << std::endl;
  }
  fin1.close();
  fin2.close();
  fin3.close();
  /* End of RM Client Setup */

  /* Begining of RM Client Main */
  // create n clients
  rm_client clients[info.n];

  // each client connected to a server
  for(size_t i = 0 ; i != info.n ;){
    clients[i].local_partyID = 1;
    clients[i].remote_partyID = i+1;
    if (clients[i].connect(IPs[i], ports[i])) {
      std::cout << "Connection to Server[" << IPs[i]
                << " : " << ports[i] << "] established" << std::endl;
      ++i;
    } else {
      std::cout << "Connection to Server[" << IPs[i]
                << " : " << ports[i] << "] failed" << std::endl;
      std::this_thread::sleep_for(std::chrono::seconds(2));
    }
  }

  /***********************************************/
  /********* Test Case Generation/Setup **********/
  /***********************************************/
  size_t prime_idx = 0;
  size_t L_idx = 0;
  size_t prime_max_idx = prime_length.size();
  size_t L_max_idx = L_value.size();
  std::vector<rm_info> test_cases;

  for(L_idx = 0 ; L_idx != L_max_idx ; L_idx++)
  {
    for(prime_idx = 0 ; prime_idx != prime_max_idx ; prime_idx++)
    {
      if(!fft_prime_from_bit_length(info.fft_prime_info, prime_length[prime_idx]))
      {
        std::cerr << "Prime Length is Invalid" << std::endl;
        return 1;
      }
      // Mix Parameter Configuration 
      info.L = L_value[L_idx]; // Mixing parameter L
      info.N = 14 * pow(info.L, 2) + 10 * info.L - 1; // the number of messages to be mixed
      test_cases.emplace_back(info);
      std::cout << "[Test Case]: Prime Info -> " << test_cases.back().fft_prime_info.two_exponent << ", "
                                   << test_cases.back().fft_prime_info.odd_factor << ", "
                                   << test_cases.back().fft_prime_info.zeta << ", "
                << "# of Msgs -> " << test_cases.back().N << std::endl;
    }
  }

  if(!fft_prime_from_bit_length(info.fft_prime_info, test_plen))
  {
    std::cerr << "Prime Length is Invalid" << std::endl;
    return 1;
  }

  for(size_t i = 0 ; i != L_value2.size() ; i++)
  {
    info.L = L_value2[i]; // Mixing parameter L
    info.N = 14 * pow(info.L, 2) + 10 * info.L - 1; // the number of messages to be mixed
    test_cases.emplace_back(info);
    std::cout << "[Test Case]: Prime Info -> " << test_cases.back().fft_prime_info.two_exponent << ", "
                                   << test_cases.back().fft_prime_info.odd_factor << ", "
                                   << test_cases.back().fft_prime_info.zeta << ", "
                << "# of Msgs -> " << test_cases.back().N << std::endl;
  }

  /*******************************/
  /********* Test Start **********/
  /*******************************/
  for(size_t case_idx = 0 ; case_idx != test_cases.size() ; case_idx++)
  {

  sid = case_idx;

  /*******************************/
  /********* Test Setup **********/
  /*******************************/
  info = test_cases[case_idx];
  NTL::ZZ_p::init(info.fft_prime_info.prime);
  std::cout << "[*****]: N = " << info.N << ", Prime = " << NTL::NumBits(info.fft_prime_info.prime) << std::endl;
  
  /*******************************/
  /*******************************/
  /*******************************/
  
  rm_net::message msg_pck;

  NTL::vec_ZZ_p input_msgs;
  NTL::vec_ZZ_p xvals = gen_xvals(info.n); // xvalues 1,2,...,n
  input_msgs.SetLength(info.N);
  NTL::random(input_msgs,info.N); // sample N random messages
  /*
  size_t malicious_client = 2;
  for(size_t i = 0 ; i != info.N ; i++)
  {
    input_msgs[i] = i+1;
  }
  */

  std::chrono::steady_clock::time_point e2e_start_tick;
  std::chrono::steady_clock::time_point start_tick;
  std::chrono::steady_clock::time_point end_tick;
  double encode_lapsed = 0;

  e2e_start_tick = chrono::steady_clock::now();

  // encode, secret-share, and submit messages one by one
  for(size_t i = 0 ; i != info.N ; i++)
  {
    vec_ZZ_p msg_encoding;
    size_t encoding_length = 7*info.L+5;

    // additive 2-bases encoding for msg
    start_tick = chrono::steady_clock::now();
    add2basis_encode(msg_encoding, input_msgs[i], info.L); 

    if(msg_encoding.length() != encoding_length)
    {
      std::cout << "Input Encoding Error Occured at message[" << i << "]\n";
      return 1;
    }

    // vector of ZZ_p vectors
    NTL::vec_vec_ZZ_p shared_encodings; 
    
    // create n vectors, each to be sent to each server
    shared_encodings.SetLength(info.n); 

    for(size_t j = 0 ; j != encoding_length ; j++)
    { 
      vec_ZZ_p temp1; // stores an input to PSS
      vec_ZZ_p temp2; // stores output shares

      // create a vector include a message to be an input to Packed-SS
      temp1.append(msg_encoding[j]);

      // sharing scheme takes as inputs a vector of secrets.
      temp2 = packed_share_secret(xvals, temp1, info.t); 
      for (size_t k = 0 ; k != info.n ; k++)
      {
        shared_encodings[k].append(temp2[k]);
      }
    }
    end_tick = chrono::steady_clock::now();
    encode_lapsed = encode_lapsed + std::chrono::duration<double, std::milli> (end_tick - start_tick).count();

    for(size_t j = 0 ; j != info.n ; j++)
    {
      clients[j].submit_message(shared_encodings[j], info, sid, i);
    }
  }
  
  std::stringstream s;
  s << "[Encode time]: " << encode_lapsed/NTL::conv<int>(info.N) << std::endl;
  std::cout << s.str();

  //std::cout << "Session[" << sid << "]: all messages submitted\n";

  // The following only for testings.
  std::vector<bool> disconnection_status;
  std::vector<bool> completeion_status;
  disconnection_status.resize(info.n, false);
  completeion_status.resize(info.n, false);

  while(!is_all_true(completeion_status))
  {
    for(size_t i = 0 ; i != info.n ; i++)
    {
      if(clients[i].is_connected())
      {
        if(!clients[i].is_incoming_empty())
        {
          rm_net::received_message temp;
          temp = clients[i].access_to_incoming_queue().pop_front();
          if(temp.msg.header.sid != sid)
          {
            continue;
          }
          if(temp.msg.header.mixing_state_id != 15)
          {
            continue;
          }
          if(!completeion_status[i])
          {
            completeion_status[i] = true;
          }
        }
      }
    }
  }
  end_tick = chrono::steady_clock::now();
  auto e2e_lapsed = std::chrono::duration<double, std::milli> (end_tick - e2e_start_tick).count();\
  std::stringstream s1;
  s1.clear();
  s1 << "[e2e time]: " << e2e_lapsed << std::endl;
  std::cout << s1.str();

  } // for-loop for test ends
  
  std::cout << "All Tests Completed.\n";
  //int sec = 10; // wait for 'sec' seconds
  //std::this_thread::sleep_for(std::chrono::milliseconds(1000*sec));
  return 0;
}
