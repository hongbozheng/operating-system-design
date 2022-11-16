#!/usr/bin/env python3
# CMD: ./plt_fig.py profile1.data profile2.data Case_study_2_Profile_Data

import sys
import matplotlib.pyplot as plt

PROFILE_DATA_FILE_PREFIX='profile_'
PROFILE_DATA_FILE_SUFFIX='.data'
CASE_STUDY_1_WORK_1_2_PNG_NAME='case_study_1_work_1_2.png'
CASE_STUDY_1_WORK_3_4_PNG_NAME='case_study_1_work_3_4.png'
# CASE_STUDY_2_FILE_NUM=5
# FILE_INDEX_LIST=[1,5,11,19,22]
CASE_STUDY_2_FILE_NUM=6
FILE_INDEX_LIST=[1,5,11,16,20,22]
CASE_STUDY_ANALYSIS_DIR='Case_Study_Analysis/'
CASE_STUDY_2_WORK_5_PNG_NAME='case_study_2_work_5.png'
DEBUG=0

def plt_figure(case, case_study_1_folder):
    time, min_flt, maj_flt, util = [], [], [], []
    print('[INFO]: Reading file %s...'%case_study_1_folder)
    with open(case_study_1_folder+'/'+PROFILE_DATA_FILE_PREFIX+str(case)+PROFILE_DATA_FILE_SUFFIX, 'r') as pro_data_file:
        lines = [line.strip() for line in pro_data_file]
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
        print('[INFO]: Finish reading file %s'%(case_study_1_folder+'/'+PROFILE_DATA_FILE_PREFIX+str(case)+PROFILE_DATA_FILE_SUFFIX))

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
        if case == 1:
            plt.title('Work Process 1: $1024$MB Memory, Random Access, and 50,000 accesses per iteration\n'
                      'Work Process 2: $1024$MB Memory, Random Access, and 10,000 accesses per iteration', fontsize=10, weight='normal')
            plt.savefig(CASE_STUDY_ANALYSIS_DIR+CASE_STUDY_1_WORK_1_2_PNG_NAME, dpi=1000)
            print('[INFO]: Graph saved to %s'%(CASE_STUDY_ANALYSIS_DIR+CASE_STUDY_1_WORK_1_2_PNG_NAME))
        elif case == 2:
            plt.title('Work Process 3: $1024$MB Memory, Random Locality Access, and 50,000 accesses per iteration\n'
                      'Work Process 4: $1024$MB Memory, Locality-based Access, and 10,000 accesses per iteration', fontsize=10, weight='normal')
            plt.savefig(CASE_STUDY_ANALYSIS_DIR+CASE_STUDY_1_WORK_3_4_PNG_NAME, dpi=1000)
            print('[INFO]: Graph saved to %s'%(CASE_STUDY_ANALYSIS_DIR+CASE_STUDY_1_WORK_3_4_PNG_NAME))
        print('[INFO]: Finish plotting graph')

def plt_thrashing(case_study_2_folder):
    ttl_util = []
    for i in FILE_INDEX_LIST:
        time, util = [], []
        print('[INFO]: Reading from file %s...'%(case_study_2_folder+'/'+PROFILE_DATA_FILE_PREFIX+str(i)+PROFILE_DATA_FILE_SUFFIX))
        with open(case_study_2_folder+'/'+PROFILE_DATA_FILE_PREFIX+str(i)+PROFILE_DATA_FILE_SUFFIX, 'r') as pro_data_file:
            lines = [line.strip() for line in pro_data_file]
        for line in lines:
            try:
                line = line.split(sep=' ', maxsplit=-1)
                time.append(int(line[0]))
                util.append(int(line[3]))
            except:
                file_len = int(line[1])
        if DEBUG:
            print('[TIME]:',time)
            print('[UTIL]:',util)
            print('[LEN]:  TIME_LEN %d, UTIL_LEN: %d'%(len(time),len(util)))
        assert file_len == len(time) == len(util)
        ttl_util.append(sum(util)/(time[-1]-time[0]))
        print('[INFO]: Finish reading file %s'%(case_study_2_folder+'/'+PROFILE_DATA_FILE_PREFIX+str(i)+PROFILE_DATA_FILE_SUFFIX))

    if DEBUG:
        print('[TTL_UTIL]:    ',ttl_util)
        print('[TTL_UTIL_LEN]:',len(ttl_util))
    print('[INFO]: Finish reading from %s directory'%case_study_2_folder)
    assert CASE_STUDY_2_FILE_NUM == len(FILE_INDEX_LIST) == len(ttl_util)

    print('[INFO]: Start plotting graph...')
    plt.rcParams["font.family"] = "serif"
    plt.rcParams["font.serif"] = ["Times New Roman"] + plt.rcParams["font.serif"]
    plt.figure(figsize=(10, 6), dpi=1000)
    plt.plot(FILE_INDEX_LIST, ttl_util, color='lightpink')
    plt.xlabel(r'Time (Jiffies)', fontsize=10, weight='normal')
    plt.ylabel(r'Total Utilization of N copies of Work Process 5 (Jiffies)', fontsize=10, weight='normal')
    plt.suptitle('Total Utilization of N copies of Work Process 5 (Jiffies) vs. Time (Jiffies)', fontsize=13, weight='normal')
    plt.title('Work Process 5: $200$MB Memory, Random Locality Access, and 10,000 accesses per iteration', fontsize=10, weight='normal')
    plt.savefig(CASE_STUDY_ANALYSIS_DIR+CASE_STUDY_2_WORK_5_PNG_NAME, dpi=1000)
    print('[INFO]: Graph saved to %s'%(CASE_STUDY_ANALYSIS_DIR+CASE_STUDY_2_WORK_5_PNG_NAME))

def main():
    if len(sys.argv) != 3:
        print('[USAGE]: ./plt_fig.py <case_study_1_profile_data_folder> <case_study_2_profile_data_folder>')
        exit(1)
    plt_figure(case=1, case_study_1_folder=sys.argv[1])
    print('-'*35)
    plt_figure(case=2, case_study_1_folder=sys.argv[1])
    print('-'*35)
    plt_thrashing(sys.argv[2])

if __name__ == '__main__':
    main()