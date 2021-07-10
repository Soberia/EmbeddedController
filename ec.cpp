#include <map>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <windows.h>

#include "ec.hpp"
#include "driver.hpp"

EmbeddedController::EmbeddedController(
    BYTE scPort,
    BYTE dataPort,
    BYTE endianness,
    UINT16 retry,
    UINT16 timeout)
{
    this->scPort = scPort;
    this->dataPort = dataPort;
    this->endianness = endianness;
    this->retry = retry;
    this->timeout = timeout;

    this->driver = Driver();
    if (this->driver.initialize())
        this->driverLoaded = TRUE;

    this->driverFileExist = driver.driverFileExist;
}

VOID EmbeddedController::close()
{
    this->driver.deinitialize();
    this->driverLoaded = FALSE;
}

EC_DUMP EmbeddedController::dump()
{
    EC_DUMP _dump;
    for (UINT16 column = 0x00; column <= 0xF0; column += 0x10)
        for (UINT16 row = 0x00; row <= 0x0F; row++)
        {
            UINT16 address = column + row;
            _dump.insert(std::pair<BYTE, BYTE>(address, this->readByte(address)));
        }

    return _dump;
}

VOID EmbeddedController::printDump()
{
    std::stringstream stream;
    stream << std::hex << std::uppercase << std::setfill('0')
           << " # | 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F" << std::endl
           << "---|------------------------------------------------" << std::endl
           << "00 | ";

    for (auto const &[address, value] : this->dump())
    {
        UINT16 nextAddress = address + 0x01;
        stream << std::setw(2) << (UINT16)value << " ";
        if (nextAddress % 0x10 == 0x00) // End of row
            stream << std::endl
                   << nextAddress << " | ";
    }

    std::string result = stream.str();
    std::cout << std::endl
              << result.substr(0, result.size() - 7) // Removing last 7 characters
              << std::endl;
}

VOID EmbeddedController::saveDump(std::string output)
{
    std::ofstream file(output, std::ios::out | std::ios::binary);
    if (file)
    {
        for (auto const &[address, value] : this->dump())
            file << this->readByte(address);
        file.close();
    }
}

BYTE EmbeddedController::readByte(BYTE bRegister)
{
    BYTE result = 0x00;
    this->operation(READ, bRegister, &result);
    return result;
}

WORD EmbeddedController::readWord(BYTE bRegister)
{
    BYTE firstByte = 0x00;
    BYTE secondByte = 0x00;
    WORD result = 0x00;

    if (this->operation(READ, bRegister, &firstByte) &&
        this->operation(READ, bRegister + 0x01, &secondByte))
    {
        if (endianness == BIG_ENDIAN)
            std::swap(firstByte, secondByte);
        result = firstByte | (secondByte << 8);
    }

    return result;
}

DWORD EmbeddedController::readDword(BYTE bRegister)
{
    BYTE firstByte = 0x00;
    BYTE secondByte = 0x00;
    BYTE thirdByte = 0x00;
    BYTE fourthByte = 0x00;
    DWORD result = 0x00;

    if (this->operation(READ, bRegister, &firstByte) &&
        this->operation(READ, bRegister + 0x01, &secondByte) &&
        this->operation(READ, bRegister + 0x02, &thirdByte) &&
        this->operation(READ, bRegister + 0x03, &fourthByte))
    {
        if (endianness == BIG_ENDIAN)
        {
            std::swap(firstByte, fourthByte);
            std::swap(secondByte, thirdByte);
        }
        result = firstByte |
                 (secondByte << 8) |
                 (thirdByte << 16) |
                 (fourthByte << 24);
    }

    return result;
}

BOOL EmbeddedController::writeByte(BYTE bRegister, BYTE value)
{
    return this->operation(WRITE, bRegister, &value);
}

BOOL EmbeddedController::writeWord(BYTE bRegister, WORD value)
{
    BYTE firstByte = value & 0xFF;
    BYTE secondByte = value >> 8;

    if (endianness == BIG_ENDIAN)
        std::swap(firstByte, secondByte);

    if (this->operation(WRITE, bRegister, &firstByte) &&
        this->operation(WRITE, bRegister + 0x01, &secondByte))
        return TRUE;
    return FALSE;
}

BOOL EmbeddedController::writeDword(BYTE bRegister, DWORD value)
{
    BYTE firstByte = value & 0xFF;
    BYTE secondByte = (value >> 8) & 0xFF;
    BYTE thirdByte = (value >> 16) & 0xFF;
    BYTE fourthByte = value >> 24;

    if (endianness == BIG_ENDIAN)
    {
        std::swap(firstByte, fourthByte);
        std::swap(secondByte, thirdByte);
    }

    if (this->operation(WRITE, bRegister, &firstByte) &&
        this->operation(WRITE, bRegister + 0x01, &secondByte) &&
        this->operation(WRITE, bRegister + 0x02, &thirdByte) &&
        this->operation(WRITE, bRegister + 0x03, &fourthByte))
        return TRUE;
    return FALSE;
}

BOOL EmbeddedController::operation(BYTE mode, BYTE bRegister, BYTE *value)
{
    BOOL isRead = mode == READ;
    BYTE operationType = isRead ? RD_EC : WR_EC;

    for (UINT16 i = 0; i < this->retry; i++)
        if (this->status(EC_IBF)) // Wait until IBF is free
        {
            this->driver.writeIoPortByte(this->scPort, operationType); // Write operation type to the Status/Command port
            if (this->status(EC_IBF))                                  // Wait until IBF is free
            {
                this->driver.writeIoPortByte(this->dataPort, bRegister); // Write register address to the Data port
                if (this->status(EC_IBF))                                // Wait until IBF is free
                    if (isRead)
                    {
                        if (this->status(EC_OBF)) // Wait until OBF is full
                        {
                            *value = this->driver.readIoPortByte(this->dataPort); // Read from the Data port
                            return TRUE;
                        }
                    }
                    else
                    {
                        this->driver.writeIoPortByte(this->dataPort, *value); // Write to the Data port
                        return TRUE;
                    }
            }
        }

    return FALSE;
}

BOOL EmbeddedController::status(BYTE flag)
{
    BOOL done = flag == EC_OBF ? 0x01 : 0x00;
    for (UINT16 i = 0; i < this->timeout; i++)
    {
        BYTE result = this->driver.readIoPortByte(this->scPort);
        // First and second bit of returned value represent
        // the status of OBF and IBF flags respectively
        if (((done ? ~result : result) & flag) == 0)
            return TRUE;
    }

    return FALSE;
}
