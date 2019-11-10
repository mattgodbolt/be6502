PROGMEM const
#include "rom1.h"
PROGMEM const
#include "rom2.h"
constexpr auto pinOffset = 21;

constexpr byte addressPins[15] = {10, 9, 8, 7, 6, 5, 4, 3, 25, 24, 21, 23, 2, 26, 1};
constexpr byte dataPins[8] = {11, 12, 13, 15, 16, 17, 18, 19};
constexpr byte notCs = 20;
constexpr byte notOe = 22;
constexpr byte notWe = 27;

byte dataPin(int bit_idx) {
  return dataPins[bit_idx] + pinOffset;
}

byte addressPin(int bit_idx) {
  return addressPins[bit_idx] + pinOffset;
}

void setAddr(unsigned int addr) {
  for (int addr_idx = 0; addr_idx < 15; ++addr_idx) {
    digitalWrite(addressPin(addr_idx), (addr & (1 << addr_idx)) ? HIGH : LOW);
  }
}

void read(unsigned int from, unsigned int to) {
  // All data in input mode
  for (auto pin_index = 0; pin_index < 8; ++pin_index) {
    pinMode(dataPin(pin_index), INPUT_PULLUP);
  }
  Serial.println("Data pins input");
  // Write, output enable and chip select
  digitalWrite(notWe + pinOffset, HIGH);
  digitalWrite(notOe + pinOffset, LOW);
  digitalWrite(notCs + pinOffset, LOW);
  Serial.println("Pins setup");
  char buf[128];
  for (auto addr = from; addr < to; addr += 16) {
    byte segment[16];
    for (auto offset = 0; offset < 16; ++offset) {
      setAddr(addr + offset);
      byte b = 0;
      for (auto bit_idx = 0; bit_idx < 8; ++bit_idx) {
        b |= digitalRead(dataPin(bit_idx)) == HIGH ? (1 << bit_idx) : 0;
      }
      segment[offset] = b;
    }
    sprintf(buf, "%04x : %02x %02x %02x %02x %02x %02x %02x %02x    %02x %02x %02x %02x %02x %02x %02x %02x",
            addr,
            segment[0], segment[1], segment[2], segment[3],
            segment[4], segment[5], segment[6], segment[7],
            segment[8], segment[9], segment[10], segment[11],
            segment[12], segment[13], segment[14], segment[15]);
    Serial.println(buf);
  }
  Serial.println("done");
}

void write(unsigned int addr, byte b) {
  digitalWrite(notOe + pinOffset, HIGH);
  setAddr(addr);
  digitalWrite(notCs + pinOffset, LOW);
  // All data in output mode
  for (auto pin_index = 0; pin_index < 8; ++pin_index) {
    pinMode(dataPin(pin_index), OUTPUT);
  }
  for (auto bit_idx = 0; bit_idx < 8; ++bit_idx) {
    digitalWrite(dataPin(bit_idx), (b & (1 << bit_idx)) ? HIGH : LOW);
  }
  digitalWrite(notWe + pinOffset, LOW);
  delayMicroseconds(1);
  digitalWrite(notWe + pinOffset, HIGH);
  delay(20);
}

void write64(unsigned int addr, const byte *b64) {
  byte t64[64];
  for (int i = 0; i < 64; ++i) {
    t64[i] = pgm_read_byte_near(b64 + i);
  }
  for (int i = 0; i < 64; ++i) {
    write(addr + i, t64[i]);
  }
}

void setup() {
  Serial.begin(57600);
  Serial.println("Hello!");

  for (auto pin = pinOffset + 1; pin <= pinOffset + 28; ++pin) {
    digitalWrite(pin, HIGH);
    pinMode(pin, OUTPUT);
  }

  Serial.println("Pins initialised");

  Serial.println("Writing...");
  for (unsigned int i = 0; i < 256; i += 64) {
    Serial.print(".");
    write64(i, out_rom1_bin + i);
  }
  Serial.print("\n");
  for (unsigned int i = 16386-64; i < 16384; i += 64) {
    Serial.print(".");
    write64(i + 16384, out_rom2_bin + i);
  }

  Serial.println("Reading...");
  read(0x0000, 0x0100);
  read(0x7fe0, 0x8000);
}


void loop() {
}
