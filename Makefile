BEEBASM:=beebasm/beebasm

.PHONY: all
all: out/rom1.h out/rom2.h out/rom.bin

out/rom1.h: out/rom1.bin
	xxd -i $^ $@

out/rom2.h: out/rom2.bin
	xxd -i $^ $@

out/rom.bin: rom.6502 | $(BEEBASM)
	mkdir -p out
	$(BEEBASM) -i $^ -o $@

out/rom1.bin: out/rom.bin
	dd if=$^ of=$@ bs=16384 count=1 skip=0
out/rom2.bin: out/rom.bin
	dd if=$^ of=$@ bs=16384 count=1 skip=1

$(BEEBASM):
	$(MAKE) -C beebasm/src code

.PHONY: program
program: out/rom.bin
	minipro -w out/rom.bin -p AT28C256

.PHONY: ice
ice:
	arduino --upload 6502ice/6502ice.ino
	minicom -D /dev/ttyACM0 -b 57600
