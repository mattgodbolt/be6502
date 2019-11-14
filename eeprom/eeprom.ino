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

template<int Nanos>
inline void delayNs() {
  constexpr int nanosPerClock = 1000000000 / (16 * 1000000);
  for (int i = 0; i < Nanos; i += nanosPerClock) {
    asm __volatile__("nop");
  }
}

constexpr byte dataPin(int bit_idx) {
  return dataPins[bit_idx] + pinOffset;
}

constexpr byte addressPin(int bit_idx) {
  return addressPins[bit_idx] + pinOffset;
}

void setAddr(unsigned int addr) {
  for (int addr_idx = 0; addr_idx < 15; ++addr_idx) {
    digitalWrite(addressPin(addr_idx), (addr & (1 << addr_idx)) ? HIGH : LOW);
  }
}

void prepRead() {
  // All data in input mode
  for (auto pin_index = 0; pin_index < 8; ++pin_index) {
    pinMode(dataPin(pin_index), INPUT);
  }
  // Write, output enable and chip select
  digitalWrite(notWe + pinOffset, HIGH);
  digitalWrite(notOe + pinOffset, LOW);
  digitalWrite(notCs + pinOffset, LOW);
}

void prepWrite() {
  for (auto pin_index = 0; pin_index < 8; ++pin_index) {
    pinMode(dataPin(pin_index), OUTPUT);
  }
  digitalWrite(notOe + pinOffset, HIGH);
}


byte readByte() {
  byte b = 0;
  for (auto bit_idx = 0; bit_idx < 8; ++bit_idx) {
    b |= digitalRead(dataPin(bit_idx)) == HIGH ? (1 << bit_idx) : 0;
  }
  return b;
}

bool verify(unsigned int from, const byte *compare, unsigned int size) {
  prepRead();
  int numFails = 0;
  for (unsigned int i = 0; i < size; ++i) {
    if ((i & 0x3ff) == 0)
      Serial.print(".");
    auto addr = from + i;
    setAddr(addr);
    auto b = readByte();
    auto expected = pgm_read_byte_near(compare + i);
    if (b != expected) {
      char buf[128];
      sprintf(buf, "\r\n!!! Mismatch at 0x%04x: read %02x, expected %02x", addr, b, expected);
      Serial.println(buf);
      if (++numFails == 10) break;
    }
  }
  return numFails == 0;
}

void read(unsigned int from, unsigned int to) {
  prepRead();
  char buf[128];
  for (auto addr = from; addr < to; addr += 16) {
    byte segment[16];
    for (auto offset = 0; offset < 16; ++offset) {
      setAddr(addr + offset);
      segment[offset] = readByte();
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

void writeByte(byte b) {
  for (auto bit_idx = 0; bit_idx < 8; ++bit_idx) {
    digitalWrite(dataPin(bit_idx), (b & (1 << bit_idx)) ? HIGH : LOW);
  }
}

void writeToggle() {
  digitalWrite(notCs + pinOffset, LOW);
  digitalWrite(notWe + pinOffset, LOW);
  delayNs<100>(); // write pulse width
  digitalWrite(notCs + pinOffset, HIGH);
  digitalWrite(notWe + pinOffset, HIGH);
}

void writeLL(unsigned int address, byte value) {
  setAddr(address);
  writeByte(value);
  delayNs<50>(); // data setup (also covers address hold)
  writeToggle();
}

void protect() {
  Serial.println("Setting data protection");
  prepWrite();
  writeLL(0x5555, 0xaa);
  writeLL(0x2aaa, 0x55);
  writeLL(0x5555, 0xa0);
  delay(20);
}

void unprotect() {
  Serial.println("Removing data protection");
  prepWrite();
  writeLL(0x5555, 0xaa);
  writeLL(0x2aaa, 0x55);
  writeLL(0x5555, 0x80);
  writeLL(0x5555, 0xaa);
  writeLL(0x2aaa, 0x55);
  writeLL(0x5555, 0x20);
  delay(20);
}

void write64(unsigned int addr, const byte *b64) {
  byte t64[64];
  memcpy_P(t64, b64, 64);

  prepWrite();
  for (int i = 0; i < 64; ++i) {
    writeLL(addr + i, t64[i]);
  }

  prepRead();
  while (readByte() != t64[63])
    /*spin*/;
}

void writeBlock(const byte *data, unsigned int offset, unsigned int size) {
  char buf[64];
  constexpr auto blockSize = 1024;
  for (unsigned int i = 0; i < size; i += blockSize) {
    sprintf(buf, "%04x : ", i + offset);
    Serial.print(buf);
    for (int sub = 0; sub < blockSize; sub += 64) {
      write64(i + sub + offset, data + i + sub);
      Serial.print(".");
    }
    Serial.print("\r\n");
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

  constexpr auto doWrite = false;
  if (doWrite) {
    unprotect();

    Serial.println("Writing...");
    writeBlock(out_rom1_bin, 0x0000, 0x4000);
    writeBlock(out_rom2_bin, 0x4000, 0x4000);
    Serial.print("\r\n");

    protect();
  }
  Serial.print("Verifying...");
  auto ok = verify(0x0000, out_rom1_bin, 0x4000)
            && verify(0x4000, out_rom2_bin, 0x4000);
  //auto ok = false;

  Serial.println("Dumping...");
  //  for (int i = 0; i < 10; ++i)
  read(0x0000, 0x0100);
  read(0x10c0, 0x10d0);
  read(0x7ff0, 0x8000);
  if (ok) {
    Serial.println("!OK!");
  } else {
    Serial.println("!FAIL!");
  }

  //  prepRead();
  //  setAddr(0x1011);
  //  for (int i = 0; i < 64; ++i) {
  //      char buf[6];
  //      sprintf(buf, "%02x", readByte());
  //      Serial.println(buf);
  //    }
}


void loop() {
}
