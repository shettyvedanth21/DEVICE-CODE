#include "modbus_reader.h"
#include "config.h"
#include "meter_data.h"
#include <ModbusMaster.h>

static ModbusMaster node;
static bool g_serialRead = false;  // one-shot: meter serial read after settle

static void preTransmission()  { digitalWrite(PIN_RS485_DE_RE, HIGH); }
static void postTransmission() { digitalWrite(PIN_RS485_DE_RE, LOW);  }

static float regsToFloat(uint16_t hi, uint16_t lo) {
    uint32_t v = ((uint32_t)hi << 16) | lo;
    float f;
    memcpy(&f, &v, sizeof(f));
    return f;
}

static bool readReg(uint16_t addr, volatile float& out) {
    if (node.readInputRegisters(addr, 2) == node.ku8MBSuccess) {
        out = regsToFloat(node.getResponseBuffer(0), node.getResponseBuffer(1));
        return true;
    }
    g_meter.modbusErrors++;
    return false;
}

enum class Step {
    VN1, VN2, VN3, VN, VL,
    I1, I2, I3, IA,
    PA, PF, FREQ, KWH, KVAR, KVAH, RUNHOURS
};
static Step g_step = Step::VN1;

void ModbusReader::init() {
    pinMode(PIN_RS485_DE_RE, OUTPUT);
    digitalWrite(PIN_RS485_DE_RE, LOW);

    Serial2.begin(MODBUS_BAUD, MODBUS_SERIAL_CFG, PIN_RS485_RX, PIN_RS485_TX);
    node.begin(MODBUS_SLAVE_ID, Serial2);
    node.preTransmission(preTransmission);
    node.postTransmission(postTransmission);
    // Meter serial read deferred to tick() after MODBUS_SETTLE_MS
}

void ModbusReader::tick() {
    // One-shot: read meter serial after RS485 transceiver has settled
    if (!g_serialRead) {
        if (millis() < MODBUS_SETTLE_MS) return;
        g_serialRead = true;
        if (node.readInputRegisters(REG_METER_SERIAL, 2) == node.ku8MBSuccess) {
            g_meter.meterSerial = ((uint32_t)node.getResponseBuffer(0) << 16)
                                 | node.getResponseBuffer(1);
            Serial.printf("[Modbus] Meter serial: %u\n", g_meter.meterSerial);
        } else {
            Serial.println("[Modbus] Failed to read meter serial");
        }
        return;  // skip regular read on settle tick
    }

    // One register pair per tick — no blocking, no delay()
    switch (g_step) {
        case Step::VN1:     readReg(REG_VN1,      g_meter.v1);        g_step = Step::VN2;      break;
        case Step::VN2:     readReg(REG_VN2,      g_meter.v2);        g_step = Step::VN3;      break;
        case Step::VN3:     readReg(REG_VN3,      g_meter.v3);        g_step = Step::VN;       break;
        case Step::VN:      readReg(REG_VN,       g_meter.vAvg);      g_step = Step::VL;       break;
        case Step::VL:      readReg(REG_VL,       g_meter.vLine);     g_step = Step::I1;       break;
        case Step::I1:      readReg(REG_I1,       g_meter.i1);        g_step = Step::I2;       break;
        case Step::I2:      readReg(REG_I2,       g_meter.i2);        g_step = Step::I3;       break;
        case Step::I3:      readReg(REG_I3,       g_meter.i3);        g_step = Step::IA;       break;
        case Step::IA:      readReg(REG_IA,       g_meter.iAvg);      g_step = Step::PA;       break;
        case Step::PA: {
            float kw = 0.0f;
            if (readReg(REG_PA, kw)) g_meter.powerW = kw * 1000.0f;
            g_step = Step::PF;
            break;
        }
        case Step::PF:      readReg(REG_PF,       g_meter.pf);        g_step = Step::FREQ;     break;
        case Step::FREQ:    readReg(REG_FREQ,      g_meter.freqHz);    g_step = Step::KWH;      break;
        case Step::KWH:     readReg(REG_KWH,       g_meter.energyKwh); g_step = Step::KVAR;    break;
        case Step::KVAR:    readReg(REG_KVAR,      g_meter.kvar);      g_step = Step::KVAH;     break;
        case Step::KVAH:    readReg(REG_KVAH,      g_meter.kvah);      g_step = Step::RUNHOURS; break;
        case Step::RUNHOURS:readReg(REG_RUNHOURS,  g_meter.runHours);  g_step = Step::VN1;      break;
    }
}
