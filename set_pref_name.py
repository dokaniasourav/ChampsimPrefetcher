#!/usr/bin/env python3

import json
import sys, os

pref_list = ["ip_stride", "no", "next_line", "next_line_instr", "no_instr", "spp_dev",
             "ppf_ip_stride", "ip_stride_modified", "next_line_degree", "next_line_degree_3",
             "next_line_degree_5", "ppf_next_line_5"]
# "spp_dev",  "va_ampm_lite" -- does not work

if len(sys.argv) < 2:
    print(f'Needs a pre-fetcher type, choose bw: {pref_list}')
    exit()

pref_type = sys.argv[1]

if pref_type not in pref_list:
    print('Error: Prefetcher ', pref_type, ' not found in the list ', pref_list, '. Exiting now..')
    exit()

with open("./my_config.json", "r+") as jsonFile:
    data = json.load(jsonFile)
    data["L2C"]["prefetcher"] = pref_type
    jsonFile.seek(0)  # rewind
    json.dump(data, jsonFile, indent=4)
    jsonFile.truncate()
