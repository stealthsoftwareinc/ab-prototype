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
#include <vector>
//#include "secretsharing.h"
//#include "additive2basis.h"
//#include "ntl_comm_sim.hpp"

class rm_server : public rm_net::server_interface
{
  public:
    rm_server(uint16_t Port): rm_net::server_interface(Port)
    {}

  protected:
    virtual bool upon_client_connection(std::shared_ptr<rm_net::connection> client)
    {
      return true;
    }

    virtual void upon_client_disconnection(std::shared_ptr<rm_net::connection> client)
    {
      std::cout << "Client disconnected\n";
    }

    virtual void upon_client_connection(std::shared_ptr<rm_net::connection> client, rm_net::message msg)
    {
      //TODO
    }

    virtual void upon_message(std::shared_ptr<rm_net::connection> client, rm_net::message msg)
    {
      
    }
};

