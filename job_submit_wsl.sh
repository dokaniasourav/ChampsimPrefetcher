#!/bin/bash
#SBATCH --job-name=MR_BEAN                  # Job name
#SBATCH --mail-type=FAIL                    # Mail Events (NONE,BEGIN,FAIL,END,ALL)
#SBATCH --mail-user=souravdokania@tamu.edu  # Replace with your email address
#SBATCH --ntasks=1                          # Run on a single CPU
#SBATCH --time=12:00:00                     # Time limit hh:mm:ss
#SBATCH --output=logDump_%j.log             # Standard output and error log
#SBATCH --partition=non-gpu                 # This job does not use a GPU

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

file_index=$1
pref_name=$2
file_name=${files[$file_index]}
output_dir="output/prefetcher_$(date '+%y%m%d')"
output_file="${output_dir}/out_${pref_name}_${file_index}_${file_name:0:3}_$(date '+%H%M%S').txt"

mkdir -p "$output_dir"

echo "Started running file $file_name at time $(date) with prefetcher $pref_name" > "$output_file"

echo "Lets go Mr. Bean, we've got a job on Prefetcher no $pref_name to run. Our input is ${file_name:0:3}"
echo "Starting file $file_name, on $(date) with Prefetcher $pref_name predictor, Output = $output_file"

bin_file="champsim_$pref_name"
bin/"$bin_file" -warmup_instructions 200000000 -simulation_instructions 1000000000 -traces ./trace_files/"$file_name" >> "$output_file"

echo "***************** Done with $file_name using $pref_name, on $(date) ******************"
echo "***************** Done with $file_name using $pref_name, on $(date) ******************" >> "$output_file"

logger_dir="logs/d_$(date '+%y%m%d')"
mkdir -p "$logger_dir"
mv logDump_* "$logger_dir"
echo "Log files moved to $logger_dir"

### Add more ChampSim runs below.