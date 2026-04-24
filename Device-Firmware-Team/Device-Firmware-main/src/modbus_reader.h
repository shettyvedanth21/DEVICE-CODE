/**
 * modbus_reader.h — Non-blocking Modbus step machine.
 * Reads one register pair per tick(). Writes into g_meter struct.
 * Increments modbusErrors counter on every failed read.
 */
#pragma once

namespace ModbusReader {
    void init();
    void tick();   // reads one register pair per call — non-blocking
}
