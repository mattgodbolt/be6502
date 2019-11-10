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

void prepRead() {
  // All data in input mode
  for (auto pin_index = 0; pin_index < 8; ++pin_index) {
    pinMode(dataPin(pin_index), INPUT_PULLUP);
  }
  // Write, output enable and chip select
  digitalWrite(notWe + pinOffset, HIGH);
  digitalWrite(notOe + pinOffset, LOW);
  digitalWrite(notCs + pinOffset, LOW);
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
  for (unsigned int i = 0; i < size; ++i) {
    if ((i & 0x3ff) == 0)
    Serial.print(".");
    auto addr = from + i;
    setAddr(addr);
    byte b = readByte();
    auto expected = pgm_read_byte_near(compare + i);
    if (b != expected) {
      char buf[128];
      sprintf(buf, "\r\n!!! Mismatch at 0x%04x: %02x != %02x", addr, b, expected);
      Serial.println(buf);
      return false;
    }
  }
  return true;
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

inline void delayNs(int ns) {
  constexpr int nanosPerClock = 1000000000 / (16 * 1000000);
  constexpr int clocksPerIter = 3; // guess
  constexpr int nsPerIter = clocksPerIter * nanosPerClock;
  for (volatile int i = 0; i < ns; i += nsPerIter) {
    // nothing
  }
}

void write64(unsigned int addr, const byte *b64) {
  byte t64[64];
  for (int i = 0; i < 64; ++i) {
    t64[i] = pgm_read_byte_near(b64 + i);
  }

  for (auto pin_index = 0; pin_index < 8; ++pin_index) {
    pinMode(dataPin(pin_index), OUTPUT);
  }
  digitalWrite(notOe + pinOffset, HIGH);

  for (int i = 0; i < 64; ++i) {
    auto b = t64[i];
    setAddr(addr + i);
    for (auto bit_idx = 0; bit_idx < 8; ++bit_idx) {
      digitalWrite(dataPin(bit_idx), (b & (1 << bit_idx)) ? HIGH : LOW);
    }
    delayNs(50); // data setup
    digitalWrite(notCs + pinOffset, LOW);
    digitalWrite(notWe + pinOffset, LOW);
    delayNs(100); // write pulse width
    digitalWrite(notCs + pinOffset, HIGH);
    digitalWrite(notWe + pinOffset, HIGH);
  }
  delay(10);
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
  Serial.begin(115200);
  Serial.println("Hello!");

  for (auto pin = pinOffset + 1; pin <= pinOffset + 28; ++pin) {
    digitalWrite(pin, HIGH);
    pinMode(pin, OUTPUT);
  }

  Serial.println("Pins initialised");

  Serial.println("Writing...");
  writeBlock(out_rom1_bin, 0x0000, 0x4000);
  writeBlock(out_rom2_bin, 0x4000, 0x4000);
  Serial.print("\r\n");

  Serial.print("Verifying...");
  if (!verify(0x0000, out_rom1_bin, 0x4000)
      || !verify(0x4000, out_rom2_bin, 0x4000)) {
    Serial.println("!FAIL!");
    return;
  }
  Serial.println("\r\nVerification OK!");

  Serial.println("Dumping...");
  read(0x0000, 0x0020);
  read(0x7f00, 0x8000);
  Serial.println("!OK!");
}


void loop() {
}
