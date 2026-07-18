nasm -f elf64 kernel/bootloader.s -o bin/bootloader.o && 
nasm -f elf64 kernel/interrputs.s -o bin/interrputs.o && 

clang -m64 -c kernel/kernel.c -o bin/kernel.o \
	-ffreestanding -O3 -Wall -mno-red-zone --target=x86_64-elf && 

ld -m elf_x86_64 -Ttext 0x7C00\
	bin/bootloader.o bin/kernel.o bin/interrputs.o -o bin/kernel.elf \
	--image-base=0 --oformat binary &&

dd if=/dev/zero of=bin/mini_uia.img bs=512 count=2880 &&
dd if=bin/kernel.elf of=bin/mini_uia.img conv=notrunc &&

exec qemu-system-x86_64 -snapshot -drive format=raw,file=bin/mini_uia.img -display curses -cpu qemu64,+sse,+sse2
