#!/usr/bin/env python3
# CMD: ./thrashing_locality.py profile1.data profile2.data

import sys
import matplotlib.pyplot as plt

CASE_STUDY_ANALYSIS_DIR='Case_Study_Analysis/'
CASE_STUDY_1_WORK12='case_study_1_work_1&2.png'
CASE_STUDY_1_WORK34='case_study_1_work_3&4.png'
DEBUG=0

def _2_rand_axs(pro_1_data):
    time, min_flt, maj_flt, util = [], [], [], []
    print('[INFO]: Reading file %s...'%pro_1_data)
    with open(pro_1_data, 'r') as pro_1_data_file:
        lines = [line.strip() for line in pro_1_data_file]
        for line in lines:
            try:
                line = line.split(sep=' ', maxsplit=-1)
                time.append(int(line[0]))
                min_flt.append(int(line[1]))
                maj_flt.append(int(line[2]))
                util.append(int(line[3]))
            except:
                file_len = int(line[1])
        if DEBUG:
            print('[TIME]:   ',time)
            print('[MIN_FLT]:',min_flt)
            print('[MAJ_FLT]:',maj_flt)
            print('[UTIL]:   ',util)
            print('[LEN]:     TIME_LEN %d, MIN_FLT_LEN %d, MAJ_FLT_LEN %d, UTIL_LEN: %d'%(len(time),len(min_flt),len(maj_flt),len(util)))
        assert file_len == len(time) == len(min_flt) == len(maj_flt) == len(util)
        print('[INFO]: Finish reading file %s'%pro_1_data)

        # start_time = time[0]
        # time = [t - start_time for t in time]
        total_flt = [min_f+maj_f for min_f,maj_f in zip(min_flt, maj_flt)]
        cumulative_flt = [sum(total_flt[0:x:1]) for x in range(0, len(total_flt))]
        if DEBUG:
            print('[TIME]:          ',time)
            print('[TOTAL_FLT]:     ',total_flt)
            print('[CUMULATIVE_FLT]:',cumulative_flt)
        assert file_len == len(time) == len(total_flt) == len(cumulative_flt)

        print('[INFO]: Start plotting graph...')
        plt.rcParams["font.family"] = "serif"
        plt.rcParams["font.serif"] = ["Times New Roman"] + plt.rcParams["font.serif"]
        plt.figure(figsize=(10, 6), dpi=1000)
        plt.plot(time, cumulative_flt, color='lightpink')
        plt.xlabel(r'Time (Jiffies)', fontsize=10, weight='normal')
        plt.ylabel(r'Cumulative Page Faults (Major Faults + Minor Faults)', fontsize=10, weight='normal')
        plt.suptitle('Cumulative Page Faults (Major Faults + Minor Faults) vs. Time (Jiffies)', fontsize=13, weight='normal')
        plt.title('Work Process 1: $1024$MB Memory, Random Access, and 50,000 accesses per iteration\n'
                  'Work Process 2: $1024$MB Memory, Random Access, and 10,000 accesses per iteration', fontsize=10, weight='normal')
        plt.savefig(CASE_STUDY_ANALYSIS_DIR+CASE_STUDY_1_WORK12, dpi=1000)
        print('[INFO]: Finish plotting graph')
        print('[INFO]: Graph saved to %s'%(CASE_STUDY_ANALYSIS_DIR+CASE_STUDY_1_WORK12))

def _1_rand_axs_1_loc_axs(pro_2_data):
    time, min_flt, maj_flt, util = [],[],[],[]
    print('[INFO]: Reading file %s...'%pro_2_data)
    with open(pro_2_data, 'r') as pro_2_data_file:
        lines = [line.strip() for line in pro_2_data_file]
        for line in lines:
            try:
                line = line.split(sep=' ', maxsplit=-1)
                time.append(int(line[0]))
                min_flt.append(int(line[1]))
                maj_flt.append(int(line[2]))
                util.append(int(line[3]))
            except:
                file_len = int(line[1])
        if DEBUG:
            print('[TIME]:   ',time)
            print('[MIN_FLT]:',min_flt)
            print('[MAJ_FLT]:',maj_flt)
            print('[UTIL]:   ',util)
            print('[LEN]:     TIME_LEN %d, MIN_FLT_LEN %d, MAJ_FLT_LEN %d, UTIL_LEN: %d'%(len(time),len(min_flt),len(maj_flt),len(util)))
        assert file_len == len(time) == len(min_flt) == len(maj_flt) == len(util)
        print('[INFO]: Finish reading file %s'%pro_2_data)

        # start_time = time[0]
        # time = [t-start_time for t in time]
        total_flt = [min_f+maj_f for min_f,maj_f in zip(min_flt,maj_flt)]
        cumulative_flt = [sum(total_flt[0:x:1]) for x in range(0, len(total_flt))]
        if DEBUG:
            print('[TIME]:          ',time)
            print('[TOTAL_FLT]:     ',total_flt)
            print('[CUMULATIVE_FLT]:',cumulative_flt)
        assert file_len == len(time) == len(total_flt) == len(cumulative_flt)

        print('[INFO]: Start plotting graph...')
        plt.rcParams["font.family"] = "serif"
        plt.rcParams["font.serif"] = ["Times New Roman"] + plt.rcParams["font.serif"]
        plt.figure(figsize=(10, 6), dpi=1000)
        plt.plot(time, cumulative_flt, color='lightpink')
        plt.xlabel(r'Time (Jiffies)', fontsize=10, weight='normal')
        plt.ylabel(r'Cumulative Page Faults (Major Faults + Minor Faults)', fontsize=10, weight='normal')
        plt.ylabel(r'Cumulative Page Faults (Major Faults + Minor Faults)', fontsize=10, weight='normal')
        plt.suptitle('Cumulative Page Faults (Major Faults + Minor Faults) vs. Time (Jiffies)', fontsize=13, weight='normal')
        plt.title('Work Process 3: $1024$MB Memory, Random Access, and 50,000 accesses per iteration\n'
                  'Work Process 4: $1024$MB Memory, Locality-based Access, and 10,000 accesses per iteration', fontsize=10, weight='normal')
        plt.savefig(CASE_STUDY_ANALYSIS_DIR+CASE_STUDY_1_WORK34, dpi=1000)
        print('[INFO]: Finish plotting graph')
        print('[INFO]: Graph saved to %s'%(CASE_STUDY_ANALYSIS_DIR+CASE_STUDY_1_WORK34))

def main():
    if len(sys.argv) != 3:
        print('[USAGE]: ./thrashing_locality.py <profile1.data_file> <profile2.data_file>')
        exit(1)
    _2_rand_axs(sys.argv[1])
    print('-'*35)
    _1_rand_axs_1_loc_axs(sys.argv[2])

if __name__ == '__main__':
    main()