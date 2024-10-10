#!/bin/bash

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

set -E -e -u -o pipefail || exit $?
trap exit ERR

mkdir -p logs

server_pids=()
num_servers=5

for ((i = 1; i <= num_servers; ++i)); do
  ./rm_server_main configs/mpc_config$i configs/mix_config configs/net_config &>logs/log$i.txt &
  server_pids+=($!)
done

./rm_client_main configs/mpc_config1 configs/mix_config configs/net_config &>logs/logclient.txt &
client_pid=$!

status=0

for ((i = 1; i <= num_servers; ++i)); do
  wait ${server_pids[i - 1]} && :
  s=$?
  echo exit status $s >>logs/log$i.txt
  if ((!status && s)); then status=$s; fi
done

wait $client_pid && :
s=$?
echo exit status $s >>logs/logclient.txt
if ((!status && s)); then status=$s; fi

exit $status
