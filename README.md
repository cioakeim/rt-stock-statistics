# Real-time Stock Statistics

This project was built for the Real-time embedded systems class in Aristotle University's ECE Department.
The system connects to Finhhub's API and processes transactions of various chosen stocks. 
The purpose is to log the transactions and calculate per-minute candlesticks/moving averages in real time,
and with minimal resource demand.

## Build
The program is built using CMake. To build the program for your native device, while inside the repository:
```
mkdir build 
cd build
cmake ..
make
```
The above assumes that libwebsockets and OpenSSL are installed on your device.  
To cross-build a binary for your Raspberry Pi:
```
cd cross-build 
mkdir build 
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain.cmake ..
make
```
For convenience, the static libraries of libwebsockets and OpenSSL are inside the 
repository, so compilation of those won't be necassary.

## Usage
To use either: `./main {api_key}` to use the API key of a specific user,
or use: `./main` to use the hardcoded `API_KEY` parameter thats defined in `main.c`, pre-compilation. 

To use on your Raspberry Pi, you have to transfer both the `main` executable as well as 
`ca-certificates.crt` to ensure that OpenSSL can function correctly.



