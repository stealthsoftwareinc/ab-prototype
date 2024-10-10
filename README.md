# Robust Anonymous Broadcast (Rabbit-Mix) Research Prototype
This is a research prototype that implements the online protocols of Rabbit-Mix. 

# How to build and run
We tested our prototype on Ubuntu 22.04. Once you download the prototype, go to the directory and run the followings.
## Required Package Installation
To install required packages, run the following bash script. 
```
bash testsetup.bash
```
## Building Rabbit-Mix Prototype 
Simply run the following.
```
make
```
## Running a local test 
For testing, the client and server mains upon their instantiations will build test cases based on parameters in `rm_common.hpp`. In `rm_common.hpp`, you have four variables for two phases of tests: `prime_length` and `L_value` for the first phase, and `test_plen` and `L_value2` for the second phase. For the first phase, the client and servers run rabbit-mix protocols for all possible combinations of primes (defined in `prime_length`) and the number of messages to be mixed (defined by `L_value`). Then, they run rabbit-mix for the second phase defined by a single prime value (`test_plen`) and the number of messages to be mixed (`L_values2`). The purpose of the first phase is to observe the rabbit-mix with different primes and the number of messages, while the second phase is to test rabbit-mix with as many client messages as possible with a single relatively small prime.

Before running the tests, you should edit `prime_length`, `L_value`,
`test_plen`, and `L_value2` in `rm_common.hpp` and rebuild by running
`make`.
For example, you might make the following edit:
 
```c++ 
std::vector<int> prime_length({256});
std::vector<int> L_value({5});

int test_plen = 256; // 32 bytes

std::vector<int> L_value2({6,10,13,16,18,20,22,23,25,26,27});
```

- **Note**: The above example will take about 6 minutes to run all 12 test cases.
- **Warning**: Each of the servers may use up to about 8GB memory (totaling about 40GB for 5 servers).

After editing `rm_common.hpp` and rebuilding by running `make`, run the following command to run the test cases with 1 client and 5 servers:

```
bash localtest.bash
```

# License
MIT License. For the further notes, refer to the [LICENSE](LICENSE) file.
