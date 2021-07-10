//-----------------------------------------------------------------------------
//     Author : hiyohiyo
//       Mail : hiyohiyo@crystalmark.info
//        Web : http://openlibsys.org/
//    License : The modified BSD license
//
//                          Copyright 2007 OpenLibSys.org. All rights reserved.
//-----------------------------------------------------------------------------

#ifndef DRIVER_H
#define DRIVER_H

// Driver Name
#define OLS_DRIVER_ID _T("WinRing0_1_2_0")
#define OLS_DRIVER_FILE_NAME_WIN_NT _T("WinRing0.sys")
#define OLS_DRIVER_FILE_NAME_WIN_NT_X64 _T("WinRing0x64.sys")

// IOCTL Function Code
#define OLS_TYPE 40000 // The Device type code
#define IOCTL_OLS_GET_REFCOUNT CTL_CODE(OLS_TYPE, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_OLS_READ_IO_PORT_BYTE CTL_CODE(OLS_TYPE, 0x833, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_OLS_WRITE_IO_PORT_BYTE CTL_CODE(OLS_TYPE, 0x836, METHOD_BUFFERED, FILE_WRITE_ACCESS)

// DLL Status Code
constexpr BYTE OLS_DLL_NO_ERROR = 0;
constexpr BYTE OLS_DLL_DRIVER_NOT_LOADED = 1;
constexpr BYTE OLS_DLL_DRIVER_NOT_FOUND = 2;
constexpr BYTE OLS_DLL_DRIVER_NOT_LOADED_ON_NETWORK = 3;
constexpr BYTE OLS_DLL_UNKNOWN_ERROR = 4;

// Driver Type
constexpr BYTE OLS_DRIVER_TYPE_UNKNOWN = 0;
constexpr BYTE OLS_DRIVER_TYPE_WIN_NT = 1;
constexpr BYTE OLS_DRIVER_TYPE_WIN_NT_X64 = 2;

// Driver Install Status Code
constexpr BYTE OLS_DRIVER_INSTALL = 1;
constexpr BYTE OLS_DRIVER_REMOVE = 2;
constexpr BYTE OLS_DRIVER_SYSTEM_INSTALL = 3;
constexpr BYTE OLS_DRIVER_SYSTEM_UNINSTALL = 4;

#pragma pack(push, 4)

typedef struct _OLS_WRITE_IO_PORT_INPUT
{
	ULONG PortNumber;
	UCHAR CharData;
} OLS_WRITE_IO_PORT_INPUT;

#pragma pack(pop)

class DriverManager
{
public:
	BOOL manage(LPCTSTR DriverId, LPCTSTR DriverPath, USHORT Function);

protected:
	HANDLE gHandle = INVALID_HANDLE_VALUE;

	BOOL installDriver(SC_HANDLE hSCManager, LPCTSTR DriverId, LPCTSTR DriverPath);
	BOOL removeDriver(SC_HANDLE hSCManager, LPCTSTR DriverId);
	BOOL startDriver(SC_HANDLE hSCManager, LPCTSTR DriverId);
	BOOL stopDriver(SC_HANDLE hSCManager, LPCTSTR DriverId);
	BOOL isSystemInstallDriver(SC_HANDLE hSCManager, LPCTSTR DriverId, LPCTSTR DriverPath);
	BOOL openDriver();
};

class Driver : public DriverManager
{
public:
	BOOL bResult;
	DWORD bytesReturned;
	BOOL driverFileExist;

	BOOL WINAPI initialize();
	VOID WINAPI deinitialize();
	BYTE WINAPI readIoPortByte(BYTE port);
	VOID WINAPI writeIoPortByte(BYTE port, BYTE value);

protected:
	BYTE driverFileExistence();

private:
	TCHAR gDriverFileName[MAX_PATH];
	TCHAR gDriverPath[MAX_PATH];
	BOOL gInitDll = FALSE;
	BYTE gDllStatus = OLS_DLL_UNKNOWN_ERROR;
	BYTE gDriverType = OLS_DRIVER_TYPE_UNKNOWN;
};

#endif
