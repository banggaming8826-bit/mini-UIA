# 1. Biên dịch file Assembly thành định dạng ELF 64-bit
nasm -f elf64 kernel/bootloader.s -o bin/bootloader.o && 
nasm -f elf64 kernel/interrputs.s -o bin/interrputs.o && 

# 2. Biên dịch file C với các flag đặc biệt cho Hệ điều hành
# -ffreestanding: Bảo GCC rằng đây là code chạy trực tiếp trên phần cứng, không có thư viện chuẩn.
# -mno-red-zone: Tắt tính năng tối ưu stack của x86_64 để tránh xung đột ngắt phần cứng.
clang -m64 -c kernel/kernel.c -o bin/kernel.o \
	-ffreestanding -O3 -Wall -mno-red-zone --target=x86_64-elf && 

# 3. Dùng Linker để nối Bootloader và Kernel lại tại địa chỉ 0x7C00
ld -m elf_x86_64 -Ttext 0x7C00\
	bin/bootloader.o bin/kernel.o bin/interrputs.o -o bin/kernel.elf \
	--image-base=0 --oformat binary &&

# 4. Tạo file đĩa mềm mini_uia.img từ file nhị phân tổng hợp
dd if=/dev/zero of=bin/mini_uia.img bs=512 count=2880 &&
dd if=bin/kernel.elf of=bin/mini_uia.img conv=notrunc &&

# 5. Khởi động QEMU hưởng thức thành quả
exec qemu-system-x86_64 -snapshot -drive format=raw,file=bin/mini_uia.img -display curses -cpu qemu64,+sse,+sse2 -d int -D exec.txt
