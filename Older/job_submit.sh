#!/bin/bash
#SBATCH --job-name=MR_BEAN                  # Job name
#SBATCH --mail-type=FAIL,END                # Mail Events (NONE,BEGIN,FAIL,END,ALL)
#SBATCH --mail-user=souravdokania@tamu.edu  # Replace with your email address
#SBATCH --ntasks=1                          # Run on a single CPU
#SBATCH --time=12:00:00                     # Time limit hh:mm:ss
#SBATCH --output=serial_test_%j.log         # Standard output and error log
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

i=$1
predictor=$2
desc=$3
bin_index=$4

# echo "Start of file " > "$output_file"

#for input_file in "${files[@]}"
#for input_file in {0..19}
#do
  input_file=${files[$i]}
  echo "Lets go Mr. Bean, we've got a job on $predictor to run. Our input is ${input_file:0:3}"
  echo "Starting file $input_file, on $(date) with $predictor predictor"
  echo "Index number is $bin_index"
  output_file="out_${predictor}_${i}_$(date '+%y%m%d%H%M%S')_${input_file:0:3}.txt"
  echo "Started running file $input_file at time $(date) with predictor $predictor" > "$output_file"
  echo "Descriptor text: $desc" >> "$output_file"
  bin_file="champsim_$bin_index"
  echo "Index number is: $bin_index, bin file is: $bin_file" >> "$output_file"
  echo "Description of the job: $desc"
  bin/$bin_file -warmup_instructions 200000000 -simulation_instructions 1000000000 -traces ~pgratz/dpc3_traces/$input_file >> $output_file
  echo "***************** Done with $input_file on $predictor at $(date) ******************"
#done

#n_job=$4
#n_job=$((n_job + 1))
#
#if [[ $i -eq 0 ]]
#then
#  echo "Submitting next set of jobs"
#  ./sbatch.sh "$n_job"
#fi


### Add more ChampSim runs below.