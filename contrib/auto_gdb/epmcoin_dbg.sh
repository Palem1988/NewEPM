#!/bin/bash
# use testnet settings,  if you need mainnet,  use ~/.epmcoincore/epmcoind.pid file instead
epmcoin_pid=$(<~/.epmcoincore/testnet3/epmcoind.pid)
sudo gdb -batch -ex "source debug.gdb" epmcoind ${epmcoin_pid}
