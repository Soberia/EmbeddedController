# **📋 How to use**
You can either copy the source code directly to your project or using compiled [Dynamic-Link Library (DLL)](https://github.com/Soberia/EmbeddedController/releases) version.
You need to compile your app with `c++17` flag or newer versions.
This app uses `WinRing0` driver to access the hardware, make sure you place `WinRing0x64.sys` or `WinRing0.sys` beside your binary files.
Your program has to run with administrator privileges to work properly.

Include `ec.hpp` header file and initialize an object from `EmbeddedController` class.
```cpp
#include <iostream>
#include <windows.h>

#include "ec.hpp"

int main()
{
    EmbeddedController ec = EmbeddedController();

    // Making sure driver file loaded successfully
    if (ec.driverFileExist && ec.driverLoaded)
    {
        // Your rest of code palces in here
        // ...

        // Free up the resources at the end
        ec.close();
    }

}
```

### **Public Methods**
* `EmbeddedController(BYTE scPort = EC_SC, BYTE dataPort = EC_DATA, BYTE endianness = LITTLE_ENDIAN, UINT16 retry = 5, UINT16 timeout = 100)`
    </br>
    If read or write operations often fails, you should increase the `retry` and `timeout` values.
    * `scPort`: Embedded Controller Status/Command port, default value is `0x66`
    * `dataPort`: Embedded Controller Data port, default value is `0x62`
    * `endianness`: Byte order of read and write operations, could be `LITTLE_ENDIAN` or `BIG_ENDIAN`, default value is `LITTLE_ENDIAN`
    * `retry`: Number of retires for failed read or write operations, default value is `5`
    * `timeout`: Waiting threshold for reading EC's OBF and IBF flags, default value is `100`

* `VOID close()`
    </br>
    Close the driver resources

* `EC_DUMP dump()`
    </br>
    Generate a `map` object of all registers
    </br>
    `return`: `map` object of register's address and value
    ```cpp
    EC_DUMP dump = ec.dump();
    BYTE value = dump.find(0x20)->second; // Accessing value of 0x20 register
    ```

* `VOID printDump()`
    </br>
    Print generated dump of all registers

* `VOID saveDump(std::string output = "dump.bin")`
    </br>
    Store generated dump of all registers to the disk
    </br>
    `output`: Path of output file, default is in the current directory

* `BYTE readByte(BYTE bRegister)`
    </br>
    Read EC register as `BYTE`
    </br>
    `bRegister`: Address of register
    </br>
    `return`: Value of register
    ```cpp
    BYTE value = ec.readByte(0x20);
    std::cout << std::hex << (INT)value; // Print value of register 0x20
    ```

* `WORD readWord(BYTE bRegister)`
    </br>
    Read EC register as `WORD`
    </br>
    `bRegister`: Address of register
    </br>
    `return`: Value of register
    ```cpp
    WORD value = ec.readWord(0x20);
    std::cout << std::hex << (INT)value; // Print value of register 0x20 and 0x21 in Little Endian byte order
    ```

* `DWORD readDword(BYTE bRegister)`
    </br>
    Read EC register as `DWORD`
    </br>
    `bRegister`: Address of register
    </br>
    `return`: Value of register
    ```cpp
    ec.endianness = BIG_ENDIAN;
    DWORD value = ec.readDword(0x20);
    std::cout << std::hex << (INT)value; // Print value of register 0x20, 0x21, 0x22 and 0x23 in Big Endian byte order
    ```

* `BOOL writeByte(BYTE bRegister, BYTE value)`
    </br>
    Write EC register as `BYTE`
    </br>
    `bRegister`: Address of register
    </br>
    `value`: Value of register
    </br>
    `return`: `TRUE` if the opreation was successful, `FALSE` otherwise
    ```cpp
    ec.writeByte(0x20, 0xAA); // Write 0xAA to register 0x20
    ```

* `BOOL writeWord(BYTE bRegister, WORD value)`
    </br>
    Write EC register as `WORD`
    </br>
    `bRegister`: Address of register
    </br>
    `value`: Value of register
    </br>
    `return`: `TRUE` if the opreation was successful, `FALSE` otherwise
    ```cpp
    // Write 0xBB to register 0x20 and 0xAA to register 0x21 in Little Endian byte order
    ec.writeWord(0x20, 0xAABB);
    ```

* `BOOL writeDword(BYTE bRegister, DWORD value)`
    </br>
    Write EC register as `DWORD`
    </br>
    `bRegister`: Address of register
    </br>
    `value`: Value of register
    </br>
    `return`: `TRUE` if the opreation was successful, `FALSE` otherwise
    ```cpp
    // Write 0xAA to register 0x20, 0xBB to register 0x21, 0xCC to register 0x22
    // and 0xDD to register 0x23 in Big Endian byte order
    ec.endianness = BIG_ENDIAN;
    ec.writeDword(0x20, 0xAABBCCDD);
    ```

# **⚠️ Disclaimer**
**Author of this software is not responsible for damage of any kind, use it at your own risk!**
