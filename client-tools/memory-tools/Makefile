DIR1=PAGE_USED
DIR2=SCHEDULE_SWAP
SRC_FILE1=${DIR1}/hypercall.c
SRC_FILE2=${DIR2}/swap_calculate.c
OUTPUT1=${DIR1}/hypercall
OUTPUT2=${DIR2}/swap_calculate

all:
	gcc -w -g -O2 -lxenctrl -lxenstore ${SRC_FILE1} -o ${OUTPUT1}
	gcc -w -g -O2 -lxenctrl -lxenstore ${SRC_FILE2} -o ${OUTPUT2}
