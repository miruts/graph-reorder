#!/bin/sh

# $1 vertexline_size
# $2 exe_path
# $3 input_args

if [ "$#" -ne 3 ]; then
  echo "Usage: $0 vertexline_size exe_path input_args"
  exit 1
fi

../gem5/build/X86/gem5.opt ../gem5/configs/example/se.py --cpu-type=detailed --num_cpus=16 --sys-clock=2GHz --caches --cacheline_size=64 --num-dirs=4 --mem-size=4GB --l1i_size=16kB --l1i_assoc=4 --l1d_size=16kB --l1d_assoc=8 --l2cache --num-l2caches=16 --vertexline_size=$1 --vertex_assoc=1 --vertex_size=1MB --l2_size=1MB --l2_assoc=8 --l2_lat=3 --vertex_lat=3 --ruby --topology=Crossbar -c $2 -o "$3"
