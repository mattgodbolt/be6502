constexpr auto clockPin = 2;
constexpr auto dataPin0 = 22;
constexpr auto addrPin0 = 30;
constexpr auto readNotWritePin = 52;

void read6502() {
  byte data = 0;
  for (auto bit_idx = 0; bit_idx < 8; ++bit_idx) {
    data |= digitalRead(dataPin0 + bit_idx) << bit_idx;
  }
  unsigned short addr = 0;
  for (auto bit_idx = 0; bit_idx < 16; ++bit_idx) {
    addr |= digitalRead(addrPin0 + bit_idx) << bit_idx;
  }
  char buf[64];
  auto rnw = digitalRead(readNotWritePin);
  sprintf(buf, "%04x %c %02x", addr, rnw ? 'r' : 'W', data);
  Serial.println(buf);
}

void setup() {
  Serial.begin(57600);
  Serial.println("6502 ICE");
  for (auto pin = dataPin0; pin < dataPin0 + 8; ++pin)
    pinMode(pin, INPUT_PULLUP);
  for (auto pin = addrPin0; pin < addrPin0 + 16; ++pin)
    pinMode(pin, INPUT_PULLUP);
  pinMode(readNotWritePin, INPUT_PULLUP);
  pinMode(clockPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(clockPin), read6502, RISING);
}


void loop() {
}
