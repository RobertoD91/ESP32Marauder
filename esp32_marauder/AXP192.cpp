#include "AXP192.h"

AXP192::AXP192() {
}

#if defined(MARAUDER_M5CORE2_AWS)

// Brings up every rail the M5Core2 needs before the panel, the digitiser or
// the SD card are touched, then pulses the shared LCD/touch reset line.
void AXP192::Core2Begin(void) {
    // Running this twice would re-assert the panel reset below and leave the
    // display asleep for the rest of the session.
    if (this->core2_ready) return;

    AXP192_I2C.begin(I2C_SDA, I2C_SCL);
    AXP192_I2C.setClock(400000);

    // Lift the VBUS current limit so the board can charge and run at once
    Write1Byte(0x30, (Read8bit(0x30) & 0x04) | 0x02);

    // GPIO1 (green status LED) and GPIO2 (speaker enable) as NMOS open drain
    Write1Byte(0x92, Read8bit(0x92) & 0xF8);
    Write1Byte(0x93, Read8bit(0x93) & 0xF8);

    // Charge the RTC backup cell
    Write1Byte(0x35, (Read8bit(0x35) & 0x1C) | 0xA2);

    // DC-DC1 = 3.35V (ESP32 and the rest of the 3V3 rail)
    Write1Byte(0x26, (Read8bit(0x26) & 0x80) | (uint8_t)((3350 - 700) / 25));

    // DC-DC3 = 2.8V (LCD backlight, raised later by Core2ScreenBreath)
    Write1Byte(0x27, (Read8bit(0x27) & 0x80) | (uint8_t)((2800 - 700) / 25));

    // LDO2 = 3.3V (LCD logic and touch), LDO3 = 2.0V (vibration motor)
    Write1Byte(0x28, (uint8_t)((((3300 - 1800) / 100) << 4) | ((2000 - 1800) / 100)));

    // Enable EXTEN, LDO2, DC-DC3 and DC-DC1; leave LDO3 (motor) off
    Write1Byte(0x12, (Read8bit(0x12) & 0xB0) | 0x47);

    // GPIO0 as a 3.3V LDO, which is how the bus rail is powered
    Write1Byte(0x91, 0xF0);
    Write1Byte(0x90, 0x02);

    // Charge to 4.2V at 100mA
    Write1Byte(0x33, 0xC0);

    // Enable every ADC channel so the battery readings are valid
    Write1Byte(0x82, 0xFF);

    // 128ms power on, 4s power off
    Write1Byte(0x36, 0x4C);

    // Temperature protection and battery detection
    Write1Byte(0x39, 0xFC);
    Write1Byte(0x32, 0x46);

    // GPIO4 (LCD and touch reset) as NMOS open drain output
    Write1Byte(0x95, (Read8bit(0x95) & 0x72) | 0x84);

    Core2SetLed(false);

    // Reset the panel and the digitiser together
    Core2SetLcdReset(false);
    delay(100);
    Core2SetLcdReset(true);
    delay(100);

    this->core2_ready = true;
}

// Backlight brightness as a percentage, driven by the DC-DC3 rail voltage.
void AXP192::Core2ScreenBreath(uint8_t percent) {
    if (percent == 0) {
        Write1Byte(0x12, Read8bit(0x12) & ~0x02); // DC-DC3 off
        return;
    }

    if (percent > 100) percent = 100;

    uint16_t voltage = 2500 + (uint16_t)(((uint32_t)(3300 - 2500) * percent) / 100);
    Write1Byte(0x27, (Read8bit(0x27) & 0x80) | (uint8_t)((voltage - 700) / 25));
    Write1Byte(0x12, Read8bit(0x12) | 0x02); // DC-DC3 on
}

// GPIO4 drives the shared LCD/touch reset line, low being reset asserted.
void AXP192::Core2SetLcdReset(bool state) {
    uint8_t data = Read8bit(0x96);

    if (state)
        data |= 0x02;
    else
        data &= ~0x02;

    Write1Byte(0x96, data);
}

// GPIO1 sinks the green status LED, so a low level lights it.
void AXP192::Core2SetLed(bool state) {
    uint8_t data = Read8bit(0x94);

    if (state)
        data &= ~0x02;
    else
        data |= 0x02;

    Write1Byte(0x94, data);
}

// The AXP192 has no fuel gauge, so approximate from the pack voltage.
int8_t AXP192::Core2BatteryPercent(void) {
    int voltage_mv = (int)(GetBatVoltage() * 1000.0);

    if (voltage_mv <= 3300) return 0;
    if (voltage_mv >= 4150) return 100;

    return (int8_t)(((voltage_mv - 3300) * 100) / 850);
}

#endif // MARAUDER_M5CORE2_AWS

void AXP192::begin(void) {
    AXP192_I2C.begin(21, 22);
    AXP192_I2C.setClock(400000);

    // Set LDO2 & LDO3(TFT_LED & TFT) 3.0V
    Write1Byte(0x28, 0xcc);

    // Set ADC to All Enable
    Write1Byte(0x82, 0xff);

    // Bat charge voltage to 4.2, Current 100MA
    Write1Byte(0x33, 0xc0);

    // Enable Bat,ACIN,VBUS,APS adc
    Write1Byte(0x82, 0xff);

    // Enable Ext, LDO2, LDO3, DCDC1
    Write1Byte(0x12, Read8bit(0x12) | 0x4D);

    // 128ms power on, 4s power off
    Write1Byte(0x36, 0x0C);

    // Set RTC voltage to 3.3V
    Write1Byte(0x91, 0xF0);

    // Set GPIO0 to LDO
    Write1Byte(0x90, 0x02);

    // Disable vbus hold limit
    Write1Byte(0x30, 0x80);

    // Set temperature protection
    Write1Byte(0x39, 0xfc);

    // Enable RTC BAT charge
    Write1Byte(0x35, 0xa2);

    // Enable bat detection
    Write1Byte(0x32, 0x46);

    // ScreenBreath(80);
}

void AXP192::Write1Byte(uint8_t Addr, uint8_t Data) {
    AXP192_I2C.beginTransmission(0x34);
    AXP192_I2C.write(Addr);
    AXP192_I2C.write(Data);
    AXP192_I2C.endTransmission();
}

uint8_t AXP192::Read8bit(uint8_t Addr) {
    AXP192_I2C.beginTransmission(0x34);
    AXP192_I2C.write(Addr);
    AXP192_I2C.endTransmission();
    AXP192_I2C.requestFrom(0x34, 1);
    return AXP192_I2C.read();
}

uint16_t AXP192::Read12Bit(uint8_t Addr) {
    uint16_t Data = 0;
    uint8_t buf[2];
    ReadBuff(Addr, 2, buf);
    Data = ((buf[0] << 4) + buf[1]);  //
    return Data;
}

uint16_t AXP192::Read13Bit(uint8_t Addr) {
    uint16_t Data = 0;
    uint8_t buf[2];
    ReadBuff(Addr, 2, buf);
    Data = ((buf[0] << 5) + buf[1]);  //
    return Data;
}

uint16_t AXP192::Read16bit(uint8_t Addr) {
    uint16_t ReData = 0;
    AXP192_I2C.beginTransmission(0x34);
    AXP192_I2C.write(Addr);
    AXP192_I2C.endTransmission();
    AXP192_I2C.requestFrom(0x34, 2);
    for (int i = 0; i < 2; i++) {
        ReData <<= 8;
        ReData |= AXP192_I2C.read();
    }
    return ReData;
}

uint32_t AXP192::Read24bit(uint8_t Addr) {
    uint32_t ReData = 0;
    AXP192_I2C.beginTransmission(0x34);
    AXP192_I2C.write(Addr);
    AXP192_I2C.endTransmission();
    AXP192_I2C.requestFrom(0x34, 3);
    for (int i = 0; i < 3; i++) {
        ReData <<= 8;
        ReData |= AXP192_I2C.read();
    }
    return ReData;
}

uint32_t AXP192::Read32bit(uint8_t Addr) {
    uint32_t ReData = 0;
    AXP192_I2C.beginTransmission(0x34);
    AXP192_I2C.write(Addr);
    AXP192_I2C.endTransmission();
    AXP192_I2C.requestFrom(0x34, 4);
    for (int i = 0; i < 4; i++) {
        ReData <<= 8;
        ReData |= AXP192_I2C.read();
    }
    return ReData;
}

void AXP192::ReadBuff(uint8_t Addr, uint8_t Size, uint8_t *Buff) {
    AXP192_I2C.beginTransmission(0x34);
    AXP192_I2C.write(Addr);
    AXP192_I2C.endTransmission();
    AXP192_I2C.requestFrom(0x34, (int)Size);
    for (int i = 0; i < Size; i++) {
        *(Buff + i) = AXP192_I2C.read();
    }
}

void AXP192::ScreenBreath(int brightness) {
    if (brightness > 100 || brightness < 0) return;
    int vol     = map(brightness, 0, 100, 2500, 3200);
    vol         = (vol < 1800) ? 0 : (vol - 1800) / 100;
    uint8_t buf = Read8bit(0x28);
    Write1Byte(0x28, ((buf & 0x0f) | ((uint16_t)vol << 4)));
}

void AXP192::ScreenSwitch(bool state) {
    uint8_t brightness;
    if (state == false) {
        brightness = 0;
    } else if (state == true) {
        brightness = 12;
    }
    uint8_t buf = Read8bit(0x28);
    Write1Byte(0x28, ((buf & 0x0f) | (brightness << 4)));
}

bool AXP192::GetBatState() {
    if (Read8bit(0x01) | 0x20)
        return true;
    else
        return false;
}
//---------coulombcounter_from_here---------
// enable: void EnableCoulombcounter(void);
// disable: void DisableCOulombcounter(void);
// stop: void StopCoulombcounter(void);
// clear: void ClearCoulombcounter(void);
// get charge data: uint32_t GetCoulombchargeData(void);
// get discharge data: uint32_t GetCoulombdischargeData(void);
// get coulomb val affter calculation: float GetCoulombData(void);
//------------------------------------------
void AXP192::EnableCoulombcounter(void) {
    Write1Byte(0xB8, 0x80);
}

void AXP192::DisableCoulombcounter(void) {
    Write1Byte(0xB8, 0x00);
}

void AXP192::StopCoulombcounter(void) {
    Write1Byte(0xB8, 0xC0);
}

void AXP192::ClearCoulombcounter(void) {
    Write1Byte(0xB8, 0xA0);
}

uint32_t AXP192::GetCoulombchargeData(void) {
    return Read32bit(0xB0);
}

uint32_t AXP192::GetCoulombdischargeData(void) {
    return Read32bit(0xB4);
}

float AXP192::GetCoulombData(void) {
    uint32_t coin  = 0;
    uint32_t coout = 0;

    coin  = GetCoulombchargeData();
    coout = GetCoulombdischargeData();

    // c = 65536 * current_LSB * (coin - coout) / 3600 / ADC rate
    // Adc rate can be read from 84H ,change this variable if you change the ADC
    // reate
    float ccc = 65536 * 0.5 * (int32_t)(coin - coout) / 3600.0 / 25.0;

    return ccc;
}
//----------coulomb_end_at_here----------

uint16_t AXP192::GetVbatData(void) {
    uint16_t vbat = 0;
    uint8_t buf[2];
    ReadBuff(0x78, 2, buf);
    vbat = ((buf[0] << 4) + buf[1]);  // V
    return vbat;
}

uint16_t AXP192::GetVinData(void) {
    uint16_t vin = 0;
    uint8_t buf[2];
    ReadBuff(0x56, 2, buf);
    vin = ((buf[0] << 4) + buf[1]);  // V
    return vin;
}

uint16_t AXP192::GetIinData(void) {
    uint16_t iin = 0;
    uint8_t buf[2];
    ReadBuff(0x58, 2, buf);
    iin = ((buf[0] << 4) + buf[1]);
    return iin;
}

uint16_t AXP192::GetVusbinData(void) {
    uint16_t vin = 0;
    uint8_t buf[2];
    ReadBuff(0x5a, 2, buf);
    vin = ((buf[0] << 4) + buf[1]);  // V
    return vin;
}

uint16_t AXP192::GetIusbinData(void) {
    uint16_t iin = 0;
    uint8_t buf[2];
    ReadBuff(0x5C, 2, buf);
    iin = ((buf[0] << 4) + buf[1]);
    return iin;
}

uint16_t AXP192::GetIchargeData(void) {
    uint16_t icharge = 0;
    uint8_t buf[2];
    ReadBuff(0x7A, 2, buf);
    icharge = (buf[0] << 5) + buf[1];
    return icharge;
}

uint16_t AXP192::GetIdischargeData(void) {
    uint16_t idischarge = 0;
    uint8_t buf[2];
    ReadBuff(0x7C, 2, buf);
    idischarge = (buf[0] << 5) + buf[1];
    return idischarge;
}

uint16_t AXP192::GetTempData(void) {
    uint16_t temp = 0;
    uint8_t buf[2];
    ReadBuff(0x5e, 2, buf);
    temp = ((buf[0] << 4) + buf[1]);
    return temp;
}

uint32_t AXP192::GetPowerbatData(void) {
    uint32_t power = 0;
    uint8_t buf[3];
    ReadBuff(0x70, 2, buf);
    power = (buf[0] << 16) + (buf[1] << 8) + buf[2];
    return power;
}

uint16_t AXP192::GetVapsData(void) {
    uint16_t vaps = 0;
    uint8_t buf[2];
    ReadBuff(0x7e, 2, buf);
    vaps = ((buf[0] << 4) + buf[1]);
    return vaps;
}

void AXP192::SetSleep(void) {
    uint8_t buf = Read8bit(0x31);
    buf         = (1 << 3) | buf;
    Write1Byte(0x31, buf);
    Write1Byte(0x90, 0x00);
    Write1Byte(0x12, 0x09);
    // Write1Byte(0x12, 0x00);
    Write1Byte(0x12, Read8bit(0x12) & 0xA1);  // Disable all outputs but DCDC1
}

uint8_t AXP192::GetWarningLeve(void) {
    AXP192_I2C.beginTransmission(0x34);
    AXP192_I2C.write(0x47);
    AXP192_I2C.endTransmission();
    AXP192_I2C.requestFrom(0x34, 1);
    uint8_t buf = AXP192_I2C.read();
    return (buf & 0x01);
}

// -- sleep
void AXP192::DeepSleep(uint64_t time_in_us) {
    SetSleep();

    if (time_in_us > 0) {
        esp_sleep_enable_timer_wakeup(time_in_us);
    } else {
        esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
    }
    (time_in_us == 0) ? esp_deep_sleep_start() : esp_deep_sleep(time_in_us);
}

void AXP192::LightSleep(uint64_t time_in_us) {
    SetSleep();

    if (time_in_us > 0) {
        esp_sleep_enable_timer_wakeup(time_in_us);
    } else {
        esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
    }
    esp_light_sleep_start();
}

// 0 not press, 0x01 long press, 0x02 press
uint8_t AXP192::GetBtnPress() {
    uint8_t state = Read8bit(0x46);
    if (state) {
        Write1Byte(0x46, 0x03);
    }
    return state;
}

uint8_t AXP192::GetWarningLevel(void) {
    return Read8bit(0x47) & 0x01;
}

float AXP192::GetBatVoltage() {
    float ADCLSB    = 1.1 / 1000.0;
    uint16_t ReData = Read12Bit(0x78);
    return ReData * ADCLSB;
}

float AXP192::GetBatCurrent() {
    float ADCLSB        = 0.5;
    uint16_t CurrentIn  = Read13Bit(0x7A);
    uint16_t CurrentOut = Read13Bit(0x7C);
    return (CurrentIn - CurrentOut) * ADCLSB;
}

float AXP192::GetVinVoltage() {
    float ADCLSB    = 1.7 / 1000.0;
    uint16_t ReData = Read12Bit(0x56);
    return ReData * ADCLSB;
}

float AXP192::GetVinCurrent() {
    float ADCLSB    = 0.625;
    uint16_t ReData = Read12Bit(0x58);
    return ReData * ADCLSB;
}

float AXP192::GetVBusVoltage() {
    float ADCLSB    = 1.7 / 1000.0;
    uint16_t ReData = Read12Bit(0x5A);
    return ReData * ADCLSB;
}

float AXP192::GetVBusCurrent() {
    float ADCLSB    = 0.375;
    uint16_t ReData = Read12Bit(0x5C);
    return ReData * ADCLSB;
}

float AXP192::GetTempInAXP192() {
    float ADCLSB             = 0.1;
    const float OFFSET_DEG_C = -144.7;
    uint16_t ReData          = Read12Bit(0x5E);
    return OFFSET_DEG_C + ReData * ADCLSB;
}

float AXP192::GetBatPower() {
    float VoltageLSB = 1.1;
    float CurrentLCS = 0.5;
    uint32_t ReData  = Read24bit(0x70);
    return VoltageLSB * CurrentLCS * ReData / 1000.0;
}

float AXP192::GetBatChargeCurrent() {
    float ADCLSB    = 0.5;
    uint16_t ReData = Read12Bit(0x7A);
    return ReData * ADCLSB;
}
float AXP192::GetAPSVoltage() {
    float ADCLSB    = 1.4 / 1000.0;
    uint16_t ReData = Read12Bit(0x7E);
    return ReData * ADCLSB;
}

float AXP192::GetBatCoulombInput() {
    uint32_t ReData = Read32bit(0xB0);
    return ReData * 65536 * 0.5 / 3600 / 25.0;
}

float AXP192::GetBatCoulombOut() {
    uint32_t ReData = Read32bit(0xB4);
    return ReData * 65536 * 0.5 / 3600 / 25.0;
}

void AXP192::SetCoulombClear() {
    Write1Byte(0xB8, 0x20);
}

void AXP192::SetLDO2(bool State) {
    uint8_t buf = Read8bit(0x12);
    if (State == true)
        buf = (1 << 2) | buf;
    else
        buf = ~(1 << 2) & buf;
    Write1Byte(0x12, buf);
}

// Cut all power, except for LDO1 (RTC)
void AXP192::PowerOff() {
    Write1Byte(0x32, Read8bit(0x32) | 0x80);  // MSB for Power Off
}

void AXP192::SetPeripherialsPower(uint8_t state) {
    if (!state)
        Write1Byte(0x10, Read8bit(0x10) & 0XFB);
    else if (state)
        Write1Byte(0x10, Read8bit(0x10) | 0X04);
    // uint8_t data;
    // Set EXTEN to enable 5v boost
}