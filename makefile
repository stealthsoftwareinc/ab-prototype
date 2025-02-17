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

# g++ input_encode.cpp -o test -I ../../sw/include -L ../../sw/lib -lntl -lgmp -pthread
# the compiler: gcc for C program, define as g++ for C++
CC = c++

# compiler flags:
#  -g     - this flag adds debugging information to the executable file
#  -Wall  - this flag is used to turn on most compiler warnings
CFLAGS  = 
NTLFLAGS = -I ../../sw/include -I ../../sw/boost_1_82_0 -L ../../sw/lib -lntl -lgmp -pthread

# The build target
all: rm_client_main rm_server_main

FORCE:

rm_server_main: FORCE
	$(CC) $(CFLAGS) rm_server_main.cpp -o rm_server_main $(NTLFLAGS)

rm_client_main: FORCE
	$(CC) $(CFLAGS) rm_client_main.cpp -o rm_client_main $(NTLFLAGS)
