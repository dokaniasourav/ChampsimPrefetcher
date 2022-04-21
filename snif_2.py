#!/usr/bin/env python3

import os
import csv
from pprint import pprint

dir_names = ['data/ppf_spp_dev_3']
filenames = []
dir_names_2 = []

dir_name = dir_names[0]

for dir_n in os.listdir(dir_name):
    dir_comb = os.path.join(dir_name, dir_n)
    for filename in os.listdir(dir_comb):
        if filename.endswith('_stat_cpu0_L2C.csv'):
            filenames.append(os.path.join(dir_comb, filename))
            dir_names_2.append(dir_n)

print(dir_names_2)
newfile = open('new_file_2.csv', 'w+')

first = True
for f_ind, filename in enumerate(filenames):
    file_cont = open('./' + filename, 'r').read()
    lines = file_cont.split('\n')
    if first:
        newfile.write("Dir Name, File Name, " + lines[0] + "\n")
        first = False
    for l_ind, line in enumerate(lines):
        if l_ind == 0:
            continue
        if len(line) < 10:
            continue
        newfile.write(dir_names_2[f_ind] + ", " + filename + ", " + line + "\n")
newfile.close()
print(filenames)