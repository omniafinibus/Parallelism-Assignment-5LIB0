#!/usr/bin/env bash 
# reload the FPGA's programmable logic where the real-time RISC-V subsystem resides

if [ ${USER} != "student" ] ; then
  echo 'this script must be run as student (not root)' 1>&2
  exit 1
fi
# kill any ongoing ./readout.sh, otherwise the board will hang
if [ `ps aux | grep channel_readall | grep -v grep | wc -l` != "0" ] ; then
  sudo killall channel_readall
  if [ `ps aux | grep channel_readall | grep -v grep | wc -l` != "0" ] ; then
    echo "couldn't kill readout.sh that is running"
    exit 1
    fi
fi

sudo overlay unload riscv
sudo overlay load riscv
echo "Done"