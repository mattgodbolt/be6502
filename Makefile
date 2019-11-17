BEEBASM:=beebasm/beebasm

.PHONY: all
all: out/rom.bin

out/rom.bin: rom.6502 | $(BEEBASM)
	mkdir -p out
	$(BEEBASM) -i $^ -o $@

$(BEEBASM):
	$(MAKE) -C beebasm/src code

.PHONY: program
program: out/rom.bin
	minipro -w out/rom.bin -p AT28C256

.PHONY: ice
ice:
	arduino --upload 6502ice/6502ice.ino
	minicom -D /dev/ttyACM0 -b 57600
