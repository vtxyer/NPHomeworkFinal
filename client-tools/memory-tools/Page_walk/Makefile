file=analyze.c
cc=gcc
output=analyze
all:
#	${cc} -g -O2 -L/root/libvmi/libvmi/.libs /root/libvmi/libvmi/.libs/libvmi.so -lglib-2.0 -lm -lxenctrl -lxenstore -Wl,-rpath -Wl,/root/libvmi/libvmi/.libs -Wl,-rpath -Wl,/usr/local/lib   ${file} -o ${output}
	${cc} -g -O2 -L/root/libvmi/libvmi/.libs /root/libvmi/libvmi/.libs/libvmi.so  -lm -lxenctrl -lxenstore ${file} hashtab.c -o ${output}
