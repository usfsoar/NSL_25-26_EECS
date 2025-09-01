#include <EEPROM.h>

#define EEPROM_SIZE       1024
#define SLOT_COUNT        3
#define BASE_ADDRESS      0
#define SLOT_SAVE_ADDRESS 900
#define MAGIC_NUMBER      0xA5A5

// ─── Struct to hold magic + 13 floats ──────────────────────────────────────────
struct FlightData {
  uint16_t magic = MAGIC_NUMBER;  // magic number for validity check
  float var1  = 0.0;
  float var2  = 0.0;
  float var3  = 0.0;
  float var4  = 0.0;
  float var5  = 0.0;
  float var6  = 0.0;
  float var7  = 0.0;
  float var8  = 0.0;
  float var9  = 0.0;
  float var10 = 0.0;
  float var11 = 0.0;
  float var12 = 0.0;
  float var13 = 0.0;  // your “state machine” variable
};

FlightData flightData;
int currentSlot = 0;

// ─── Low-level EEPROM actions ─────────────────────────────────────────────────
void save_to_eeprom() {
  flightData.magic = MAGIC_NUMBER;
  int addr = BASE_ADDRESS + (currentSlot * sizeof(FlightData));
  EEPROM.put(addr,             flightData);
  EEPROM.put(SLOT_SAVE_ADDRESS, currentSlot);
  EEPROM.commit();
  Serial.println("→ [EEPROM] Saved.");
}

void restore_from_eeprom() {
  // NOTE: assumes currentSlot already restored
  int addr = BASE_ADDRESS + (currentSlot * sizeof(FlightData));
  EEPROM.get(addr, flightData);
  Serial.println("→ [EEPROM] Data loaded from slot " + String(currentSlot));
}

void reset_eeprom() {
  Serial.println("[EEPROM] Resetting all used bytes...");
  // Erase flightData slots
  for (int i = 0; i < SLOT_COUNT * sizeof(FlightData); i++) {
    EEPROM.write(BASE_ADDRESS + i, 0xFF);
  }
  // Erase saved slot index
  for (int i = 0; i < sizeof(int); i++) {
    EEPROM.write(SLOT_SAVE_ADDRESS + i, 0xFF);
  }
  EEPROM.commit();
  Serial.println("[EEPROM] Erase complete.");
}

// ─── Helpers ───────────────────────────────────────────────────────────────────
void increment_flight_data() {
  flightData.var1  += 1.0;
  flightData.var2  += 1.0;
  flightData.var3  += 1.0;
  flightData.var4  += 1.0;
  flightData.var5  += 1.0;
  flightData.var6  += 1.0;
  flightData.var7  += 1.0;
  flightData.var8  += 1.0;
  flightData.var9  += 1.0;
  flightData.var10 += 1.0;
  flightData.var11 += 1.0;
  flightData.var12 += 1.0;
  flightData.var13 += 1.0;
}

void display_flight_data(const FlightData& d) {
  Serial.print("  Magic: 0x"); Serial.println(d.magic, HEX);
  Serial.print("  var1:  "); Serial.println(d.var1);
  Serial.print("  var2:  "); Serial.println(d.var2);
  Serial.print("  var3:  "); Serial.println(d.var3);
  Serial.print("  var4:  "); Serial.println(d.var4);
  Serial.print("  var5:  "); Serial.println(d.var5);
  Serial.print("  var6:  "); Serial.println(d.var6);
  Serial.print("  var7:  "); Serial.println(d.var7);
  Serial.print("  var8:  "); Serial.println(d.var8);
  Serial.print("  var9:  "); Serial.println(d.var9);
  Serial.print("  var10: "); Serial.println(d.var10);
  Serial.print("  var11: "); Serial.println(d.var11);
  Serial.print("  var12: "); Serial.println(d.var12);
  Serial.print("  var13: "); Serial.println(d.var13);
}

void display_all_slots() {
  Serial.println("[EEPROM] Dumping all slots:");
  for (int s = 0; s < SLOT_COUNT; s++) {
    FlightData tmp;
    int addr = BASE_ADDRESS + (s * sizeof(FlightData));
    EEPROM.get(addr, tmp);
    Serial.println(" Slot " + String(s) + ":");
    display_flight_data(tmp);
  }
}

// ─── Arduino entrypoints ────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  if (!EEPROM.begin(EEPROM_SIZE)) {
    Serial.println("Failed to init EEPROM"); while (1);
  }

  // 1) Restore currentSlot (might be garbage on a fresh upload)
  EEPROM.get(SLOT_SAVE_ADDRESS, currentSlot);
  if (currentSlot < 0 || currentSlot >= SLOT_COUNT) {
    currentSlot = 0;
  }
  Serial.println("Startup: currentSlot = " + String(currentSlot));

  // 2) Load flightData (also garbage if never saved)
  int addr = BASE_ADDRESS + (currentSlot * sizeof(FlightData));
  EEPROM.get(addr, flightData);
  Serial.print("Startup: read magic = 0x");
  Serial.println(flightData.magic, HEX);

  // 3) If magic is wrong, *seed* slot 0 with defaults
  if (flightData.magic != MAGIC_NUMBER) {
    Serial.println("Magic mismatch → Seeding EEPROM with default data");
    flightData = FlightData();  // resets all floats to 0.0 + magic
    currentSlot = 0;
    save_to_eeprom();
    display_all_slots();
  }
  else {
    // 4) magic OK: now check var13 range for restore
    if (flightData.var13 >= 5.0 && flightData.var13 <= 10.0) {
      Serial.println("var13 in [5,10] → Restoring slot");
      restore_from_eeprom();
    } else {
      Serial.println("var13 out of [5,10] → Skip restore");
    }
    display_all_slots();
  }
}

void loop() {
  // Press 'r' to erase EEPROM & reboot
  if (Serial.available()) {
    char c = Serial.read();
    if (c=='r' || c=='R') {
      reset_eeprom();
      Serial.println("User triggered reset → Rebooting...");
      delay(200);
      ESP.restart();
    }
  }

  delay(3000);
  increment_flight_data();
  Serial.print("Loop: var13 = ");
  Serial.println(flightData.var13);

  // Only save if var13 in [5,10]
  if (flightData.var13 >= 5.0 && flightData.var13 <= 10.0) {
    save_to_eeprom();
  } else {
    Serial.println("var13 out of [5,10] → Not saving");
  }

  // Advance slot circularly
  currentSlot = (currentSlot + 1) % SLOT_COUNT;
}
