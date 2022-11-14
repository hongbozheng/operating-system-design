DEV_MAJOR_NUM=423
DEV_MINOR_NUM=0
CASE_STUDY_1_PROFILE_DATA_DIR="linux/Case_Study_1_Profile_Data"
CASE_STUDY_2_PROFILE_DATA_DIR="linux/Case_Study_2_Profile_Data"

rmmod mp3
make
insmod mp3.ko
mknod node c $DEV_MAJOR_NUM $DEV_MINOR_NUM
nice ./work 1024 R 50000 & nice ./work 1024 R 10000
wait
./monitor > profile1.data
nice ./work 1024 R 50000 & nice ./work 1024 L 10000
wait
./monitor > profile2.data
cp profile* ../../$CASE_STUDY_1_PROFILE_DATA_DIR

for i in 1 5 11 19 24
do
  for _ in $(seq 1 $i); do nice ./work 200 R 10000 & done
  wait
  ./monitor > profile_${i}.data
done
cp profile* ../../$CASE_STUDY_2_PROFILE_DATA_DIR