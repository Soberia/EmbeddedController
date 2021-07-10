#ifndef EC_H
#define EC_H

#include "map"

#include "driver.hpp"

auto constexpr VERSION = "0.1";

constexpr BYTE LITTLE_ENDIAN = 0;
constexpr BYTE BIG_ENDIAN = 1;

constexpr BYTE READ = 0;
constexpr BYTE WRITE = 1;

constexpr BYTE EC_OBF = 0x01;  // Output Buffer Full
constexpr BYTE EC_IBF = 0x02;  // Input Buffer Full
constexpr BYTE EC_DATA = 0x62; // Data Port
constexpr BYTE EC_SC = 0x66;   // Status/Command Port
constexpr BYTE RD_EC = 0x80;   // Read Embedded Controller
constexpr BYTE WR_EC = 0x81;   // Write Embedded Controller

typedef std::map<BYTE, BYTE> EC_DUMP;

/**
 * Implementation of ACPI embedded controller specification to access the EC's RAM
 * @see https://uefi.org/specs/ACPI/6.4/12_ACPI_Embedded_Controller_Interface_Specification/ACPI_Embedded_Controller_Interface_Specification.html
*/
class EmbeddedController
{
public:
    BYTE scPort;
    BYTE dataPort;
    BYTE endianness;
    BOOL driverLoaded = FALSE;
    BOOL driverFileExist = FALSE;

    /**
     * @param scPort Embedded Controller Status/Command port.
     * @param dataPort Embedded Controller Data port.
     * @param endianness Byte order of read and write operations, could be `LITTLE_ENDIAN` or `BIG_ENDIAN`.
     * @param retry Number of retires for failed read or write operations.
     * @param timeout Waiting threshold for reading EC's OBF and IBF flags.
    */
    EmbeddedController(
        BYTE scPort = EC_SC,
        BYTE dataPort = EC_DATA,
        BYTE endianness = LITTLE_ENDIAN,
        UINT16 retry = 5,
        UINT16 timeout = 100);

    /** Close the driver resources */
    VOID close();

    /**
     * Generate a dump of all registers.
     * @return Map of register's address and value.
     */
    EC_DUMP dump();

    /** Print generated dump of all registers */
    VOID printDump();

    /**
     * Store generated dump of all registers to the disk.
     * @param output Path of output file.
     */
    VOID saveDump(std::string output = "dump.bin");

    /**
     * Read EC register as BYTE.
     * @param bRegister Address of register.
     * @return Value of register.
     */
    BYTE readByte(BYTE bRegister);

    /**
     * Read EC register as WORD.
     * @param bRegister Address of register.
     * @return Value of register.
     */

    WORD readWord(BYTE bRegister);

    /**
     * Read EC register as DWORD.
     * @param bRegister Address of register.
     * @return Value of register.
     */
    DWORD readDword(BYTE bRegister);

    /**
     * Write EC register as BYTE.
     * @param bRegister Address of register.
     * @param value Value of register.
     * @return Successfulness of operation.
     */
    BOOL writeByte(BYTE bRegister, BYTE value);

    /**
     * Write EC register as WORD.
     * @param bRegister Address of register.
     * @param value Value of register.
     * @return Successfulness of operation.
     */
    BOOL writeWord(BYTE bRegister, WORD value);

    /**
     * Write EC register as DWORD.
     * @param bRegister Address of register.
     * @param value Value of register.
     * @return Successfulness of operation.
     */
    BOOL writeDword(BYTE bRegister, DWORD value);

protected:
    UINT16 retry;
    UINT16 timeout;
    Driver driver;

    /**
     * Perform a read or write operation.
     * @param mode Type of operation.
     * @param bRegister Address of register.
     * @param value Value of register.
     * @return Successfulness of operation.
     */
    BOOL operation(BYTE mode, BYTE bRegister, BYTE *value);

    /**
     * Check EC status for permission to read or write.
     * @param flag Type of flag.
     * @return Whether allowed to perform read or write.
     */
    BOOL status(BYTE flag);
};

#endif
