#!/bin/bash

files=(
      "600.perlbench_s-210B.champsimtrace.xz"
      "627.cam4_s-573B.champsimtrace.xz"
      "602.gcc_s-734B.champsimtrace.xz"
      "628.pop2_s-17B.champsimtrace.xz"
      "603.bwaves_s-3699B.champsimtrace.xz"
      "631.deepsjeng_s-928B.champsimtrace.xz"
      "605.mcf_s-665B.champsimtrace.xz"
      "638.imagick_s-10316B.champsimtrace.xz"
      "607.cactuBSSN_s-2421B.champsimtrace.xz"
      "641.leela_s-800B.champsimtrace.xz"
      "619.lbm_s-4268B.champsimtrace.xz"
      "644.nab_s-5853B.champsimtrace.xz"
      "620.omnetpp_s-874B.champsimtrace.xz"
      "648.exchange2_s-1699B.champsimtrace.xz"
      "621.wrf_s-575B.champsimtrace.xz"
      "649.fotonik3d_s-1176B.champsimtrace.xz"
      "623.xalancbmk_s-700B.champsimtrace.xz"
      "654.roms_s-842B.champsimtrace.xz"
      "625.x264_s-18B.champsimtrace.xz"
      "657.xz_s-3167B.champsimtrace.xz"
 )

desc=(
      "Bimodal Table Size 16384"
      "Bimodal Table Size 8192 (prime should be set to 8191)"
      "Bimodal Table Size 16384 and Max Counter Size 1"
      "Perceptron history: 24, Number of perceptrons: 163, Perceptron Bits: 8"
      "Perceptron history: 24, Number of perceptrons: 81, Perceptron Bits: 8"
      "Perceptron history: 24, Number of perceptrons: 163, Perceptron Bits: 4"
      "Global History Length: 14, History Table Size: 16384"
      "Global History Length: 14, History Table Size: 8192"
      "Global History Length: 7, History Table Size: 16384"
)


configs=(
      'bimodal'
      'perceptron'
      'gshare'
)

for j in {0..2} # For the configs
do
  for k in {1..3} # Nth variation of the config
  do
    json_inp=("{ \"branch_predictor\": \"${configs[$j]}$k\" }")
    echo "Configuring predictor ${configs[$j]}$k with champsim simulator"
    echo "${json_inp[0]}" > my_config.json

    # Create the config file for this predictor now
    ./config.py my_config.json
    ind=$((j*3 + (k-1)))
    echo "Starting build of ${configs[$j]}$k config... "

    # Compile source code, make executable
    make
    echo "Starting job submissions for ${configs[$j]}$k ..."
    echo "Description =  ${desc[$ind]}"

    cp bin/champsim "bin/champsim_$ind"
    rm bin/champsim

    echo "Copied and removed"
    echo ""
    ls bin


    # Loop through to send batch jobs
#    for i in {0..19}
#    do
#      echo "Submitting job for the file ${files[$i]} with ${configs[$j]}$k configuration"
#      sbatch job_submit.sh "$i" "${configs[$j]}$k" "${desc[($j+($k-1)*3)]}" "$ind"
#    done
    echo ""
    echo ""
  done
done