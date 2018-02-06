PREFIX=/home/xlh/android-ndk-r10e/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86/bin/
CC=$(PREFIX)/arm-linux-androideabi-gcc -fPIE -pie
SYSROOT=/home/xlh/android-ndk-r10e/platforms/android-21/arch-arm
FLAGS=-pie -fPIE --sysroot=$(SYSROOT)
bd:
	$(CC) -fPIE -pie -o bd_stat bd.c --sysroot=$(SYSROOT)
	
clean:
	rm bd_stat