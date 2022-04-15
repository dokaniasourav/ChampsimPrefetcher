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

#prefs=(
#      "no"
#      "ip_stride"
#      "next_line"
#      "spp_dev"
#      "next_line_degree_3"
#      "next_line_degree_5"
#      "ppf_next_line_5"
#      )

prefs=(
      "spp_dev"
      )

#Looping through all the pre-fetchers
for pref in "${prefs[@]}"
do

  pref_cpp_file="prefetcher/$pref/$pref.cc"
  if [ -f "$pref_cpp_file" ]; then
    echo "$pref_cpp_file exists, proceed submit new jobs"
  else
    echo
    echo "$pref_cpp_file does not exist, check the pf folder. We will exit now"
    exit
  fi

  python3 set_pref_name.py "$pref"

  check_pf=$(grep "\"prefetcher\": \"$pref\"" my_config.json -wc)
  if [ "$check_pf" == "0" ]; then
    echo
    echo "Prefetcher $pref not set, some error must have occurred. We will exit now"
    exit
  fi

  pref2=$(echo "$pref" | tr _ -)
  echo "Delete $pref2 from bin folder"
  bin_file="bin/champsim_$pref2"
  rm "$bin_file" "bin/champsim"
  echo "Starting build of $pref2 prefetcher"

  ./config.py my_config.json
  make

  cp bin/champsim "$bin_file"
  rm bin/champsim

  if [ -f "$bin_file" ]; then
    echo
    echo "$bin_file exists, proceed submit new jobs"
  else
    echo
    echo "$bin_file does not exist, some error in make command. We will exit now"
    exit
  fi

  echo "Copied and removed"
  echo ""


  for i in {0..19}
  do
    echo ""
    echo "Submitting job for the file ${files[$i]} with $pref2 prefetcher"
    sbatch job_submit2.sh "$i" "$pref2"
  done
  echo "Completed submissions for file index $i, and pre-fetcher $pref2"
  echo ""
done
