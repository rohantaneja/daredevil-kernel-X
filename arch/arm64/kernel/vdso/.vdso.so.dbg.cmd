cmd_arch/arm64/kernel/vdso/vdso.so.dbg := /home/nikolay/aosp/prebuild/aarch64-linux-android-4.9/bin/aarch64-linux-android-gcc -Wp,-MD,arch/arm64/kernel/vdso/.vdso.so.dbg.d  -nostdinc -isystem /home/nikolay/aosp/prebuild/aarch64-linux-android-4.9/bin/../lib/gcc/aarch64-linux-android/4.9.x/include -I/home/nikolay/aosp/Android7A7000/arch/arm64/include -Iarch/arm64/include/generated  -Iinclude -I/home/nikolay/aosp/Android7A7000/arch/arm64/include/uapi -Iarch/arm64/include/generated/uapi -I/home/nikolay/aosp/Android7A7000/include/uapi -Iinclude/generated/uapi -include /home/nikolay/aosp/Android7A7000/include/linux/kconfig.h -D__KERNEL__ -mlittle-endian -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Wno-format-security -fno-delete-null-pointer-checks -mtune=cortex-a53 -ffast-math -march=armv8-a -std=gnu89 -O2 -ffast-math -mgeneral-regs-only -fno-pic -mtune=cortex-a53 -march=armv8-a -O2 -ftree-vectorize -Wframe-larger-than=1400 -fno-stack-protector -Wno-unused-but-set-variable -fno-omit-frame-pointer -fno-optimize-sibling-calls -fno-var-tracking-assignments -g -Wdeclaration-after-statement -Wno-pointer-sign -fno-strict-overflow -fconserve-stack -shared -fno-common -fno-builtin -nostdlib -Wl,-soname=linux-vdso.so.1 -Wl,--hash-style=sysv -Wl,-shared    -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(vdso.so)"  -D"KBUILD_MODNAME=KBUILD_STR(vdso.so)" -Wl,-n -Wl,-T arch/arm64/kernel/vdso/vdso.lds arch/arm64/kernel/vdso/gettimeofday.o arch/arm64/kernel/vdso/note.o arch/arm64/kernel/vdso/sigreturn.o -o arch/arm64/kernel/vdso/vdso.so.dbg