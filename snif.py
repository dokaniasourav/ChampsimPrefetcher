#!/usr/bin/env python3

import os
import csv
from pprint import pprint


# Directory Names to use
# dir_names = ['output/prefetcher_220321/', 'output/prefetcher_220322/', 'output/prefetcher_220323/',
#              'output/prefetcher_220330', 'output/prefetcher_220331/']

dir_names = ['output/prefetcher_220414']
filenames = []
for dir_name in dir_names:
    for filename in os.listdir(dir_name):
        if filename.endswith('.txt'):
            filenames.append(os.path.join(dir_name, filename))

temp = {}
pref_name = 'spp_dev'

for filename in filenames:
    if filename.endswith('.txt') and 'out_spp-dev' in filename:

        file_cont = open(filename, 'r').read()
        filename = filename.split('/')[-1]
        lines = file_cont.split('\n')
        line_index = 0
        for line_num in range(line_index, 30):
            if 'CPU 0 runs' in lines[line_num]:
                words = lines[line_num].split()
                benchmark_name = words[-1].split('/')[-1]
                temp[filename] = {
                }
                temp[filename]['Benchmark'] = benchmark_name
                line_index = line_num
                break

        last_line_index = line_index

        temp[filename]['Prefetcher'] = pref_name
        # for line_num in range(line_index, len(lines)):
        #     for pref_at in ['cpu0_L2C', 'cpu0_L1D', 'cpu0_L1I', 'cpu0_LLC']:
        #         if pref_at in lines[line_num]:
        #             words = lines[line_num].split()
        #             pref_name = ' '.join(words[1:])
        #             temp[filename]['pref_' + pref_at] = pref_name
        #             prefAvail.append(pref_at)
        #             line_index = line_num
        #             break
        #     if 'Heartbeat CPU 0' in lines[line_num]:
        #         # print('Breaking at heartbeat for '+filename)
        #         break
        # # print(prefAvail)
        # if line_index == last_line_index:
        #     temp[filename]['pref_no'] = 'no'

        for line_num in range(line_index, len(lines)):
            if lines[line_num].startswith('Warmup complete CPU 0 instructions:'):
                line_index = line_num
                break

        for line_num in range(line_index, len(lines)):
            if lines[line_num].startswith('Finished CPU 0 instructions:'):
                words = lines[line_num].split()
                temp[filename]['Time'] = lines[line_num].split('(')[-1].split(':')[-1].rstrip(')')
                for i, word in enumerate(words):
                    if word == 'cycles:':
                        temp[filename]['Cycles'] = words[i+1]
                        break
                line_index = line_num
                break

        for line_num in range(line_index, len(lines)):
            if lines[line_num].startswith('CPU 0 cumulative IPC:'):
                words = lines[line_num].split()
                for i, word in enumerate(words):
                    if word == 'IPC:':
                        temp[filename]['IPC'] = words[i+1]
                        break
                line_index = line_num
                break

        for line_num in range(line_index, len(lines)):
            if lines[line_num].startswith('cpu0_L2C'):
                words = [wd for wd in lines[line_num].split() if len(wd) > 0]
                stat_words = ['LOAD',
                              'PREFETCH',
                              'AVERAGE MISS']
                # 'TOTAL     ACCESS:',
                access_words = ['ACCESS:', 'HIT:', 'MISS:', 'REQUESTED:',
                                'LATENCY:', 'ISSUED:', 'USEFUL:', 'USELESS:']
                for stat_word in stat_words:
                    if stat_word in lines[line_num]:
                        for i, access_word in enumerate(access_words):
                            for j, word in enumerate(words):
                                if word == access_word:
                                    temp[filename][stat_word+' '+access_word.rstrip(':')] = words[j+1]
                # for i, word in enumerate(words):
                #     if word in stat_words:
                #         temp[filename][word.rstrip(':')] = words[i+1]
                line_index = line_num

for f_name in temp.keys():
    out = []
    for key in temp[f_name].keys():
        if type(temp[f_name][key]) is not dict:
            out.append(str(temp[f_name][key]))
        else:
            for s in temp[f_name][key].keys():
                out.append(temp[f_name][key][s])
    print(out)

file = open('Output_file.csv', 'w+', encoding='UTF8', newline='')
wrt = csv.writer(file)
row_head1 = ['Output File']
row_head2 = ['']
for f_name in temp.keys():
    if len(temp[f_name].keys()) < 5:
        continue
    for key in temp[f_name].keys():
        if type(temp[f_name][key]) is not dict:
            row_head1.append(key)
            row_head2.append('')
        else:
            for s in temp[f_name][key].keys():
                row_head1.append(key)
                row_head2.append(s)
    wrt.writerow(row_head1)
    # wrt.writerow(row_head2)
    break

for f_name in temp.keys():
    row_out = [f_name]
    for key in temp[f_name].keys():
        if type(temp[f_name][key]) is not dict:
            row_out.append(temp[f_name][key])
        else:
            for s in temp[f_name][key].keys():
                row_out.append(temp[f_name][key][s])
    wrt.writerow(row_out)

file.close()

        # if type(temp[f_name][key]) is not dict:
        # print(f'{f_name}, {key}, {temp[f_name][key]}')


