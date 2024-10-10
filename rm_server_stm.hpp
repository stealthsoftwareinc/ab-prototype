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
#include <NTL/vec_ZZ_p.h>
#include <vector>
#include <bits/stdc++.h> 

using namespace std;
using namespace NTL;

enum mix_state {
  WAIT_FOR_INPUTS,
  GET_RAND_COIN,
  COMPUTE_WELLFORMEDNESS_PREDICATES,
  BATCHED_OPEN_WF_PREDICATES_1, // send msgs (round 1)
  BATCHED_OPEN_WF_PREDICATES_2, // wait for msgs (round 1)
  BATCHED_OPEN_WF_PREDICATES_3, // send msgs (round 2)
  BATCHED_OPEN_WF_PREDICATES_4, // wait for msgs (round 2)
  OPEN_CHECK_WF_PREDICATES,
  DECOMPRESS_CLIENT_INPUTS,
  COMPUTE_SUM_OF_POWERS,
  BATCHED_OPEN_SUMS_OF_POWERS_5, // send msgs (round 3)
  BATCHED_OPEN_SUMS_OF_POWERS_6, // wait for msgs (round 3)
  BATCHED_OPEN_SUMS_OF_POWERS_7, // send msgs (round 4)
  BATCHED_OPEN_SUMS_OF_POWERS_8, // wait for msgs (round 4)
  COMPUTE_NEWTON_ID_AND_FIND_ROOTS, 
  COMPLETED,
  NUMBER_OF_STATES
};

class rm_mixing_stm{
  public:
    //constructor
    rm_mixing_stm(const rm_info& _info,
                  NTL::ZZ_pX poly_from_xvals);

    // return state
    mix_state get_state()
    {
      return stm_state;
    } 

    // message handler 
    void message_handler(
        rm_net::deserialized_message& dm,
        const rm_info& info); 

    // executes server logics
    void execute_rm_stm(
        rm_client clients[],
        const rm_info& info,
        std::shared_ptr<std::map<uint32_t,bool>> corr_clients,
        std::shared_ptr<std::map<uint32_t,bool>> corr_servers); 

    void set_coin(NTL::ZZ_p seed)
    {
      ver_coin_seed = seed;
    }

    void set_zero_shares(NTL::ZZ_p zero_share)
    {
      deg_2t_zero_shares = zero_share;
    }

    void compute_wellformedness_pred(const rm_info& info);

    // Batch open: expand shares and send i-th expanded share to i-th server
    void batched_open_expand_send(
        rm_client clients[],
        const rm_info& info,
        NTL::vec_ZZ_p& shares,
        size_t num_blocks,
        size_t size_last,
        size_t rd);

    // Batch open: open all received expanded shares to all
    // ret_exp_openings := to store this server's openings
    void open_exp_shares_to_all(
        rm_client clients[],
        const rm_info& info,
        NTL::vec_vec_ZZ_p& ret_exp_openings, 
        size_t num_blocks,
        size_t rd);

    // Batch open: reconstuct all batched shares
    void reconstruct_batched_shares(
        vec_ZZ_p& output_secrets,
        vec_vec_ZZ_p& opened_exp_shares,
        const rm_info& info,
        size_t num_blocks,
        size_t last_size);

    // deompress valid client inputs
    void decompress_input_encodings(
        const rm_info& info,
        std::shared_ptr<std::map<uint32_t,bool>> corr_clients);

    // compute the sum of powers for power p
    void compute_sum_of_powers(
        size_t p, 
        const rm_info& info);

    // compute the sums of powers for all powers
    void compute_sums_of_powers(const rm_info& info);
  
  public:
    uint32_t sid; // session id unique up to each stm
    std::chrono::steady_clock::time_point e2e_start_tick;
    std::chrono::steady_clock::time_point e2e_end_tick;
    std::chrono::steady_clock::time_point wf_start_tick;
    std::chrono::steady_clock::time_point wf_end_tick;

  private:
    mix_state stm_state; // current mixing state
    std::vector<std::vector<bool>> msg_reception_status; // boolean vector to indicate the message reception status
    size_t len_input_encoding; // the length of input encoding
    size_t batched_block_size; // block size of batched open
    NTL::vec_ZZ_p xvals; // a vector of 1, 2, ..., n where n is the number of servers
    NTL::ZZ_pX g0;  // polynomial from x values as its roots
    NTL::ZZ_p ver_coin_seed; // a random coin seed for well-formedness verification
    NTL::ZZ_p deg_2t_zero_shares; // degree 2t zero shares used to open 2t shares
    NTL::vec_vec_ZZ_p client_input;
    NTL::vec_ZZ_p preds; // input well-formedness predicates
    NTL::vec_vec_ZZ_p decompressed;
    NTL::vec_ZZ_p shared_sums_of_powers;
    NTL::vec_vec_ZZ_p rec_exp_shares1; // a container to store expanded shares of WF preds
    NTL::vec_vec_ZZ_p ret_open_exp_shares1; // stores opennings of expanded shares returned from other servers
    NTL::vec_vec_ZZ_p rec_exp_shares2; // a container to store expanded shares of sums of powers
    NTL::vec_vec_ZZ_p ret_open_exp_shares2; // store opennings of expanded shares returned from other servers
    size_t client_msg_counter; 
    size_t num_blocks1;
    size_t size_last1;
    NTL::ZZ_p zero;
    std::map<uint32_t,std::shared_ptr<rm_net::connection>> rm_client_connections; // a vector to store all connections to rm clients
};

rm_mixing_stm::rm_mixing_stm(
              const rm_info& info,
              NTL::ZZ_pX poly_from_xvals)
{
  msg_reception_status.resize(4); // Total 4 rounds
  for (size_t i = 0 ; i != 4 ; i++)
  {
    msg_reception_status[i].resize(info.n, false);
  }
  len_input_encoding = 7*info.L+5;
  batched_block_size = info.n - (2*info.t + 1);
  xvals = gen_xvals(info.n);
  g0 = BuildFromRoots(xvals);
  client_msg_counter = 0;
  stm_state = WAIT_FOR_INPUTS;
  num_blocks1 = info.N/info.l; // 23/1 --> 23
  size_last1 = info.N%info.l;  // 0
  if(size_last1 != 0)
  {
    num_blocks1++;
  }
  decompressed.SetLength(info.N);
  // set up spaces for shares for batched opens
  rec_exp_shares1.SetLength(num_blocks1);
  ret_open_exp_shares1.SetLength(num_blocks1); 
  ret_open_exp_shares2.SetLength(num_blocks1); 
  for(size_t i = 0 ; i != num_blocks1 ; i++) 
  { // for each block, we expect n shares
    rec_exp_shares1[i].SetLength(info.n);
    ret_open_exp_shares1[i].SetLength(info.n);
    ret_open_exp_shares2[i].SetLength(info.n);
  }
  zero = 0;
}

void rm_mixing_stm::compute_wellformedness_pred(const rm_info& info){
  size_t encoding_size = 7*info.L+5;
  vec_vec_ZZ_p coins;
  coins.SetLength(info.N);
  for(size_t i = 0 ; i != info.N; i++)
  {
    for(size_t j = 0 ; j != encoding_size-1 ; j++)
    {
      ver_coin_seed = ver_coin_seed + (encoding_size-1 + i) + j;
      NTL::SetSeed(NTL::rep(ver_coin_seed));
      coins[i].append(NTL::to_ZZ_p(NTL::RandomBits_ZZ(NumBits(info.fft_prime_info.prime)*2)));
    }
  }
  for (size_t i = 0 ; i != info.N ; i++){
    preds.append(verify_format(coins[i], client_input[i], info.L));
  }
}

// a helper function to convert a subvector of length n to a matrix of n by 1
mat_ZZ_p get_block_to_open(vec_ZZ_p& input, size_t pos, size_t size_block){
  mat_ZZ_p block;
  block.SetDims(size_block,1);
  for(size_t j = 0 ; j != size_block ; j++){
    block.put(j,0,input[pos+j]);
  }
  return block;
}

// Place element input[i] to vector out[i] when the input has n elements and out has n vectors
void spread_block(vec_vec_ZZ_p& out, vec_ZZ_p& input, size_t n)
{
  if(out.length() != input.length() || input.length() != n)
  {
    std::cout << "STM ERROR: Expanded Mismatching with The number of Servers\n";
    return;
  }
  for(size_t i = 0 ; i != n ; i++)
  {
    out[i].append(input[i]);
  }
}

void rm_mixing_stm::batched_open_expand_send(
  rm_client clients[], // connections to other servers as a client
  const rm_info& info, // rm info
  NTL::vec_ZZ_p& shares, // shares to open
  size_t num_blocks, // expected number of blocks
  size_t size_last,  // expected size of the last block
  size_t rd)         // round > 0
{
  mat_ZZ_p vdm, temp, block_to_open;
  vec_vec_ZZ_p expanded_shares;
  /* sanity check for shares to be opened, against the designated memory size */
  size_t size_shares = shares.length();
  size_t temp_num_blocks = size_shares/info.l;
  size_t temp_size_last = size_shares%info.l;

  if(temp_size_last != 0)
  {
    temp_num_blocks++;
  }
  assert(temp_num_blocks == num_blocks && temp_size_last == size_last);
  /* Upon the sanity check passing, proceed to expand shares and send them out */
  expanded_shares.SetLength(info.n); // container for expanded shares per server
  vdm = vandermonde_gen(info.n, info.l-1);
  for (size_t i = 0 ; i != num_blocks ; i++)
  {
    block_to_open = get_block_to_open(shares, i*info.l, info.l);
    temp = NTL::transpose(vdm * block_to_open); // multiply (expand) and convert the output into a vector 
    spread_block(expanded_shares, temp[0], info.n);
  }
  if (size_last != 0)
  {
    vdm = vandermonde_gen(info.n, size_last-1);
    block_to_open = get_block_to_open(shares, num_blocks*info.l, size_last);
    temp = NTL::transpose(vdm * block_to_open);
    spread_block(expanded_shares, temp[0], info.n);
  }
  for(size_t i = 0 ; i != info.n ; i++)
  {
    if(info.server_id-1 == i) // if the shares are mine, I locally store.
    {
      for (size_t j = 0 ; j!= num_blocks ; j++)
      {
        rec_exp_shares1[j][i] = expanded_shares[i][j];
      }
      msg_reception_status[rd-1][info.server_id-1] = true;
      continue;
    }
    clients[i].send_vector(expanded_shares[i], info, sid, stm_state+1, 1, 1, 1);
  }
}

void rm_mixing_stm::open_exp_shares_to_all(
  rm_client clients[],    // connections to send data to other servers as clients
  const rm_info& info,    // MPC/RM info
  NTL::vec_vec_ZZ_p& ret_exp_openings, // to store this server's opening
  size_t num_blocks,      // number of blocks
  size_t rd)              // round > 0
{
  size_t cnt = 0;
  NTL::vec_ZZ_p temp, errors, opened_shares;
  bool test;

  for(size_t i = 0 ; i != num_blocks ; i++)
  {
    assert(rec_exp_shares1[i].length() == info.n);
    test = rs_decode(temp, errors, xvals, rec_exp_shares1[i], g0, 2*info.t, 1);
    if(test) // if opening succeeded, then add the shares to opened_shares
    {
      opened_shares.append(temp[0]);
    }
    else
    {
      std::cout << "[Open Expended Shares To All]: RS DECODE FAILED" << "\n";
      opened_shares.append(zero); // otherwise, set it to 0
    }
    temp.kill();
    errors.kill();
  }

  for (size_t i = 0 ; i != info.n ; i++)
  {
    if(info.server_id-1 == i) // if the shares are mine, I locally store.
    {
      for (size_t j = 0 ; j!= num_blocks ; j++)
      {
        ret_exp_openings[j][i] = opened_shares[j];
      }
      msg_reception_status[rd-1][info.server_id-1] = true;
      continue;
    }
    if(clients[i].is_connected()){
      clients[i].send_vector(opened_shares, info, sid, stm_state+1, 1, 1, 1);
    }
  }
}

/*
bool is_all_true(std::vector<bool> rec_status_vec)
{
  size_t len = rec_status_vec.size();
  bool res = true;
  for(size_t i = 0 ; i != len ; i++)
  {
    res = res & rec_status_vec[i];
  }
  return res;
}
*/

void rm_mixing_stm::reconstruct_batched_shares(
  vec_ZZ_p& output_secrets,
  vec_vec_ZZ_p& opened_exp_shares,
  const rm_info& info,
  size_t num_blocks,
  size_t last_size)
{ 
  vec_ZZ_p secrets, errors;
  bool test;
  for(size_t i = 0 ; i != num_blocks ; i++)
  {
    if (i == num_blocks-1 && last_size != 0)
    {
      test = rs_decode(secrets, errors, xvals, opened_exp_shares[num_blocks-1], g0, last_size-1, last_size); 
    }
    else
    {
      test = rs_decode(secrets, errors, xvals, opened_exp_shares[i], g0, info.l-1, info.l);
    }
    for(size_t j = 0 ; j != secrets.length() ; j++)
    {
      output_secrets.append(secrets[j]);
    }
    opened_exp_shares[i].kill();
    secrets.kill();
  }
  opened_exp_shares.kill();
}

void rm_mixing_stm::decompress_input_encodings(
  const rm_info& info,
  std::shared_ptr<std::map<uint32_t,bool>> corr_clients)
{
  for(size_t i = 0 ; i != info.N ; i++){
    assert(decompressed[i].length()==0);
    assert(client_input[i].length()==len_input_encoding);
    if (corr_clients->at(static_cast<uint32_t>(i))){
      decompressed[i].SetLength(info.N); // set to be all zero's
    }
    else{
      decompressed[i] = opt_decompress_encoding(client_input[i],info.L);
    }
    assert(decompressed[i].length()==info.N);
    client_input[i].kill();
  }
  client_input.kill();
}

void rm_mixing_stm::compute_sum_of_powers(
  size_t p, 
  const rm_info& info)
{
  for(size_t i = 0 ; i != info.N ; i++){
    shared_sums_of_powers[p] += decompressed[i][p];
  }
  //shared_sums_of_powers[p] += zero_share_2d;
}

void rm_mixing_stm::compute_sums_of_powers(const rm_info& info){
  shared_sums_of_powers.SetLength(info.N);
  for(size_t i = 0 ; i != info.N ; i++){
    compute_sum_of_powers(i, info);
  }
}

void rm_mixing_stm::message_handler(
  rm_net::deserialized_message& dm,
  const rm_info& info
){
  switch (dm.mixing_state_id)
  {
    case WAIT_FOR_INPUTS: // [rount 0] wait for client msgs
    {
      if(client_msg_counter == 0)
      {
        //std::cout << "MSG HANDLER: The first client message is received. Memroy allocated.\n";
        client_input.SetLength(info.N);
        rm_client_connections.insert({dm.sender_id, dm.conn});
        rm_client_connections.find(dm.sender_id)->second->local_partyID = info.server_id;
        rm_client_connections.find(dm.sender_id)->second->remote_partyID = dm.sender_id;
      }
      if (dm.body[0].length() != 7*info.L+5)
      {
        std::cout << "MSG HANDLER: Deserialized Input Is Incorrectly Received\n";
        // TODO: add this client to the corrupted client list
      }
      assert(client_input.length() == info.N);
      client_input[dm.sender_id].SetLength(7*info.L+5);
      client_input[dm.sender_id] = dm.body[0];
      client_msg_counter++;

      /*
      if(rm_client_connections.find(dm.sender_id) == rm_client_connections.end())
      {
        rm_client_connections.insert({dm.sender_id, dm.conn});
      }
      */
      break;
    }
    case BATCHED_OPEN_WF_PREDICATES_2: // [rount 1] wait
    {
      if (msg_reception_status[0][dm.sender_id-1]) // TODO: map dm.sender_id to a x-value
      {
        break; // we will ingonre the current dm message
      }
      for(size_t i = 0 ; i != num_blocks1 ; i++)
      {
        rec_exp_shares1[i][dm.sender_id-1] = dm.body[0][i];
      }
      msg_reception_status[0][dm.sender_id-1] = true;
      break;
    }
    case BATCHED_OPEN_WF_PREDICATES_4: // [rount 2] wait
    {
      if (msg_reception_status[1][dm.sender_id-1]) // TODO: map dm.sender_id to a x-value
      {
        break; // we will ingonre the current dm message
      }
      for(size_t i = 0 ; i != num_blocks1 ; i++)
      {
        ret_open_exp_shares1[i][dm.sender_id-1] = dm.body[0][i];
      }
      msg_reception_status[1][dm.sender_id-1] = true;
      break;
    }
    case BATCHED_OPEN_SUMS_OF_POWERS_6: // [rount 3] wait
    {
      if (msg_reception_status[2][dm.sender_id-1]) // TODO: map dm.sender_id to a x-value
      {
        break; // we will ingonre the current dm message
      }
      for(size_t i = 0 ; i != num_blocks1 ; i++)
      {
        rec_exp_shares1[i][dm.sender_id-1] = dm.body[0][i];
      }
      msg_reception_status[2][dm.sender_id-1] = true;
      break;
    }
    case BATCHED_OPEN_SUMS_OF_POWERS_8: // [round 4] wait
    {
      if (msg_reception_status[3][dm.sender_id-1]) // TODO: map dm.sender_id to a x-value
      {
        break; // we will ingonre the current dm message
      }
      for(size_t i = 0 ; i != num_blocks1 ; i++)
      {
        ret_open_exp_shares2[i][dm.sender_id-1] = dm.body[0][i];
      }
      msg_reception_status[3][dm.sender_id-1] = true;
      break;
    }
    default:
    {
      cout << "MSG HANDLER ERROR: Message for Undefined State Received: " << dm.mixing_state_id << endl;
      break;
    }
  }
}

void rm_mixing_stm::execute_rm_stm(
      rm_client clients[],
      const rm_info& info,
      std::shared_ptr<std::map<uint32_t,bool>> corr_clients,
      std::shared_ptr<std::map<uint32_t,bool>> corr_servers) 
      // TODO: create a separate class for corr clients/servers
{
  bool flag = true;
  while(flag)
  {
    switch (stm_state)
    {
      case WAIT_FOR_INPUTS:
      {
        if(client_msg_counter == info.N)
        {
          //std::cout << "STM State: All " << info.N << " client messages are recieved\n";
          std::cout << "[*****]: N = " << info.N << ", Prime = " << NTL::NumBits(info.fft_prime_info.prime) << std::endl;
          client_msg_counter = 0; // initialize the counter
          stm_state = COMPUTE_WELLFORMEDNESS_PREDICATES;
          //std::cout << "MOVING TO THE WF VERIFICATION\n";
        }
        else
        {
          flag = false; // stop the stm and wait for more client msgs
        }
        break;
      }
      case COMPUTE_WELLFORMEDNESS_PREDICATES:
      {
        e2e_start_tick = chrono::steady_clock::now();
        wf_start_tick = chrono::steady_clock::now();

        //std::cout << "STM State: Compute Predicates\n";

        std::chrono::steady_clock::time_point start_tick;
        std::chrono::steady_clock::time_point end_tick;
        start_tick = chrono::steady_clock::now();

        compute_wellformedness_pred(info);

        end_tick = chrono::steady_clock::now();
        auto lapsed = std::chrono::duration<double, std::milli> (end_tick - start_tick).count();
        std::cout <<  "[COMWF time]: " << lapsed << "\n";

        stm_state = BATCHED_OPEN_WF_PREDICATES_1;
        break;
      }
      case BATCHED_OPEN_WF_PREDICATES_1: // [Round 1] expand/send preds
      {
        //std::cout << "STM State: [Round 1] Batch Open Predicates\n";
        batched_open_expand_send(clients, info, preds, num_blocks1, size_last1,1);
        stm_state = BATCHED_OPEN_WF_PREDICATES_2;
        preds.kill(); // release memory
        flag = false; // stop the stm and wait for shares from other servers
        break;
      }
      case BATCHED_OPEN_WF_PREDICATES_2: // [Round 1] wait for data
      {
        if(is_all_true(msg_reception_status[0]))
        {
          //std::cout << "STM State: [Round 1] All messages are recieved\n";
          assert(rec_exp_shares1.length() == num_blocks1);
          for(size_t i = 0 ; i != num_blocks1 ; i++){
            assert(rec_exp_shares1.at(i).length() == info.n);
          }
          msg_reception_status[0].clear();
          stm_state = BATCHED_OPEN_WF_PREDICATES_3; // proceed to the next state
          break;
        }
        else 
        {
          flag = false; // stop the stm and wait for shares from other servers
        }
        break;
      }
      case BATCHED_OPEN_WF_PREDICATES_3: // [Round 2] open all shares to all servers
      {
        //std::cout << "STM State: [Round 2] Reconstruct/Send Expanded Predicate Openings to all\n";
        
        open_exp_shares_to_all(clients, info, ret_open_exp_shares1, num_blocks1, 2);
        
        stm_state = BATCHED_OPEN_WF_PREDICATES_4; // proceed to the next state
        flag = false; // stop the stm and wait for shares from other servers
        break;
      }
      case BATCHED_OPEN_WF_PREDICATES_4: // [Round 2] wait for data
      {
        if(is_all_true(msg_reception_status[1]))
        {
          //std::cout << "STM State: [Round 2] All Openned Expanded Predicate Shares Returned\n";
          stm_state = OPEN_CHECK_WF_PREDICATES; // proceed to the next state
          msg_reception_status[1].clear();
          break;
        }
        else 
        {
          flag = false; // stop the stm and wait for shares from other servers
        }
        break;
      }
      case OPEN_CHECK_WF_PREDICATES: // open all shares to all servers
      {
        //std::cout << "STM State: Reconstruct and Verify WF Predicates\n";
        vec_ZZ_p output_preds;

        reconstruct_batched_shares(
            output_preds, 
            ret_open_exp_shares1, 
            info, 
            num_blocks1, 
            size_last1);

        for(size_t i = 0 ; i != info.N ; i++)
        {
          if(output_preds[i] != 0)
          {
            corr_clients->at(i) = true;
          }
        }
        assert(output_preds.length() == info.N);

        wf_end_tick = chrono::steady_clock::now();
        auto wf_lapsed = std::chrono::duration<double, std::milli> (wf_end_tick - wf_start_tick).count();
        std::cout << "[E2EWF time]: " << wf_lapsed << "\n";

        stm_state = DECOMPRESS_CLIENT_INPUTS; // proceed to the next state 
        break;
      }
      case DECOMPRESS_CLIENT_INPUTS:
      {
        //std::cout << "STM State: Decompress Client Inputs\n";

        std::chrono::steady_clock::time_point start_tick;
        std::chrono::steady_clock::time_point end_tick;
        start_tick = chrono::steady_clock::now();

        decompress_input_encodings(info, corr_clients);

        end_tick = chrono::steady_clock::now();
        auto lapsed = std::chrono::duration<double, std::milli> (end_tick - start_tick).count();
        std::cout << "[DECOM time]: " << lapsed << "\n";

        stm_state = COMPUTE_SUM_OF_POWERS; 
        break;
      }
      case COMPUTE_SUM_OF_POWERS: 
      {
        //std::cout << "STM State: Compute Sums of Powers\n";

        std::chrono::steady_clock::time_point start_tick;
        std::chrono::steady_clock::time_point end_tick;
        start_tick = chrono::steady_clock::now();

        compute_sums_of_powers(info);

        end_tick = chrono::steady_clock::now();
        auto lapsed = std::chrono::duration<double, std::milli> (end_tick - start_tick).count();
        std::cout << "[SOPOW time]: " << lapsed << "\n";

        stm_state = BATCHED_OPEN_SUMS_OF_POWERS_5; 
        break;
      }
      case BATCHED_OPEN_SUMS_OF_POWERS_5:  // [round 3] expand/send shares
      {
        //std::cout << "STM State: [Round 3] Batch Open Shared Sums of Powers\n";
        batched_open_expand_send(clients, info, shared_sums_of_powers, num_blocks1, size_last1,3);
        shared_sums_of_powers.kill(); // release memeory after sending all
        stm_state = BATCHED_OPEN_SUMS_OF_POWERS_6; 
        flag = false; 
        break;
      }
      case BATCHED_OPEN_SUMS_OF_POWERS_6: // [rount 3] wait
      {
        if(is_all_true(msg_reception_status[2]))
        {
          //std::cout << "STM State: [Round 3] All Expanded SOPs Shares Received\n";
          stm_state = BATCHED_OPEN_SUMS_OF_POWERS_7; // proceed to the next state
          break;
        }
        else 
        {
          flag = false; // stop the stm and wait for shares from other servers
        }
        break;
      }
      case BATCHED_OPEN_SUMS_OF_POWERS_7: // round 4: open to all
      {
        //std::cout << "STM State: [Round 4] Reconstruct/Send Expanded Sums of Power Openings to all\n";
        open_exp_shares_to_all(clients, info, ret_open_exp_shares2, num_blocks1, 4);
        stm_state = BATCHED_OPEN_SUMS_OF_POWERS_8; 
        flag = false; 
        break;
      }
      case BATCHED_OPEN_SUMS_OF_POWERS_8: // [round 4] wait
      {
        if(is_all_true(msg_reception_status[3]))
        {
          //std::cout << "STM State: [Round 4] All Expanded SOP Openings Returned\n";
          stm_state = COMPUTE_NEWTON_ID_AND_FIND_ROOTS; // proceed to the next state
          break;
        }
        else 
        {
          flag = false; // stop the stm and wait for shares from other servers
        }
        break;
      }
      case COMPUTE_NEWTON_ID_AND_FIND_ROOTS: // round 4: Reconctruct Sums of Powers
      {
        //std::cout << "STM State: Reconstruct Sums of Powers anc Complete Mixing\n";
        NTL::ZZ_pX sym_poly;
        NTL::vec_ZZ_p sums_of_powers;
        NTL::vec_ZZ_p rm_output;

        // reconstruct all sums of powers
        reconstruct_batched_shares(
          sums_of_powers,
          ret_open_exp_shares2, 
          info, 
          num_blocks1, 
          size_last1);
        
        sym_poly.SetLength(info.N+1);

        // compute a symmetric polynomial via Newton's Identities
        
        std::chrono::steady_clock::time_point start_tick;
        std::chrono::steady_clock::time_point end_tick;
        start_tick = chrono::steady_clock::now();

        newton_to_polynomial(sym_poly, sums_of_powers, info.N);

        end_tick = chrono::steady_clock::now();
        auto lapsed = std::chrono::duration<double, std::milli> (end_tick - start_tick).count();
        std::cout << "[NEWID Time]: " << lapsed << "\n";

        // find the roots of the symmetric polynomial and complete the mixing stm.
        start_tick = chrono::steady_clock::now();
        
        rm_output = find_roots(
                        sym_poly, 
                        info.fft_prime_info.zeta, 
                        info.fft_prime_info.two_exponent, 
                        info.fft_prime_info.odd_factor);

        end_tick = chrono::steady_clock::now();
        lapsed = std::chrono::duration<double, std::milli> (end_tick - start_tick).count();
        std::cout << "[ROOTF Time]: " << lapsed << "\n";
        
        e2e_end_tick = chrono::steady_clock::now();
        auto e2e_lapsed = std::chrono::duration<double, std::milli> (e2e_end_tick - e2e_start_tick).count();
        std::cout <<  "[RME2E time]: " << e2e_lapsed << "\n";
        
        size_t num_output = rm_output.length();

        /*
        std::cout << "*** Output Messages ***\n";
        for(size_t i = 0 ; i != num_output ; i++)
        {
          std::cout << rm_output.at(i) << ",";
        }
        std::cout << std::endl;
        */

        //std::cout << "*** Mixing Completed ***\n";
        stm_state = COMPLETED; 
        flag = false; 
        // notify all clients the completion of mixing for session
        rm_net::message response;
        response.header.sid = sid;
        response.header.mixing_state_id = COMPLETED;
        response.header.sender_id = info.server_id;
        std::map<uint32_t,std::shared_ptr<rm_net::connection>>::iterator itr;
        for(itr = rm_client_connections.begin() ; itr != rm_client_connections.end() ; itr++)
        {
          if(itr->second->is_connected())
          {
            itr->second->send_message(response);
          }
        }
        break;
      }
      default:
      {
        std::cout << "Undefined State Reached" << endl;
        break;
      }
    }
  }
}