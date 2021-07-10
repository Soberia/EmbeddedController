//-----------------------------------------------------------------------------
//     Author : hiyohiyo
//       Mail : hiyohiyo@crystalmark.info
//        Web : http://openlibsys.org/
//    License : The modified BSD license
//
//                          Copyright 2007 OpenLibSys.org. All rights reserved.
//-----------------------------------------------------------------------------

#include <iostream>
#include <tchar.h>
#include <windows.h>
#include <VersionHelpers.h>

#include "driver.hpp"

BOOL DriverManager::manage(LPCTSTR DriverId, LPCTSTR DriverPath, USHORT Function)
{
	BOOL rCode = FALSE;
	DWORD error = NO_ERROR;
	SC_HANDLE hService = NULL;
	SC_HANDLE hSCManager = NULL;

	if (DriverId == NULL || DriverPath == NULL)
		return FALSE;

	hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (hSCManager == NULL)
		return FALSE;

	switch (Function)
	{
	case OLS_DRIVER_INSTALL:
		if (installDriver(hSCManager, DriverId, DriverPath))
			rCode = startDriver(hSCManager, DriverId);
		break;
	case OLS_DRIVER_REMOVE:
		if (!isSystemInstallDriver(hSCManager, DriverId, DriverPath))
		{
			stopDriver(hSCManager, DriverId);
			rCode = removeDriver(hSCManager, DriverId);
		}
		break;
	case OLS_DRIVER_SYSTEM_INSTALL:
		if (isSystemInstallDriver(hSCManager, DriverId, DriverPath))
			rCode = TRUE;
		else
		{
			if (!openDriver())
			{
				stopDriver(hSCManager, DriverId);
				removeDriver(hSCManager, DriverId);
				if (installDriver(hSCManager, DriverId, DriverPath))
					startDriver(hSCManager, DriverId);
				openDriver();
			}

			hService = OpenService(hSCManager, DriverId, SERVICE_ALL_ACCESS);
			if (hService != NULL)
			{
				rCode = ChangeServiceConfig(
					hService,
					SERVICE_KERNEL_DRIVER,
					SERVICE_AUTO_START,
					SERVICE_ERROR_NORMAL,
					DriverPath,
					NULL,
					NULL,
					NULL,
					NULL,
					NULL,
					NULL);
				CloseServiceHandle(hService);
			}
		}
		break;
	case OLS_DRIVER_SYSTEM_UNINSTALL:
		if (!isSystemInstallDriver(hSCManager, DriverId, DriverPath))
			rCode = TRUE;
		else
		{
			if (gHandle != INVALID_HANDLE_VALUE)
			{
				CloseHandle(gHandle);
				gHandle = INVALID_HANDLE_VALUE;
			}

			if (stopDriver(hSCManager, DriverId))
				rCode = removeDriver(hSCManager, DriverId);
		}
		break;
	default:
		rCode = FALSE;
		break;
	}

	if (hSCManager != NULL)
		CloseServiceHandle(hSCManager);

	return rCode;
}

BOOL DriverManager::installDriver(SC_HANDLE hSCManager, LPCTSTR DriverId, LPCTSTR DriverPath)
{
	SC_HANDLE hService = NULL;
	BOOL rCode = FALSE;
	DWORD error = NO_ERROR;

	hService = CreateService(
		hSCManager,
		DriverId,
		DriverId,
		SERVICE_ALL_ACCESS,
		SERVICE_KERNEL_DRIVER,
		SERVICE_DEMAND_START,
		SERVICE_ERROR_NORMAL,
		DriverPath,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL);

	if (hService == NULL)
	{
		error = GetLastError();
		if (error == ERROR_SERVICE_EXISTS)
			rCode = TRUE;
	}
	else
	{
		rCode = TRUE;
		CloseServiceHandle(hService);
	}

	return rCode;
}

BOOL DriverManager::removeDriver(SC_HANDLE hSCManager, LPCTSTR DriverId)
{
	SC_HANDLE hService = NULL;
	BOOL rCode = FALSE;

	hService = OpenService(hSCManager, DriverId, SERVICE_ALL_ACCESS);
	if (hService == NULL)
		rCode = TRUE;
	else
	{
		rCode = DeleteService(hService);
		CloseServiceHandle(hService);
	}

	return rCode;
}

BOOL DriverManager::startDriver(SC_HANDLE hSCManager, LPCTSTR DriverId)
{
	SC_HANDLE hService = NULL;
	BOOL rCode = FALSE;
	DWORD error = NO_ERROR;

	hService = OpenService(hSCManager, DriverId, SERVICE_ALL_ACCESS);
	if (hService != NULL)
		if (!StartService(hService, 0, NULL))
		{
			error = GetLastError();
			if (error == ERROR_SERVICE_ALREADY_RUNNING)
				rCode = TRUE;
		}
		else
			rCode = TRUE;
	CloseServiceHandle(hService);

	return rCode;
}

BOOL DriverManager::stopDriver(SC_HANDLE hSCManager, LPCTSTR DriverId)
{
	SC_HANDLE hService = NULL;
	BOOL rCode = FALSE;
	SERVICE_STATUS serviceStatus;
	DWORD error = NO_ERROR;

	hService = OpenService(hSCManager, DriverId, SERVICE_ALL_ACCESS);
	if (hService != NULL)
	{
		rCode = ControlService(hService, SERVICE_CONTROL_STOP, &serviceStatus);
		error = GetLastError();
		CloseServiceHandle(hService);
	}

	return rCode;
}

BOOL DriverManager::isSystemInstallDriver(SC_HANDLE hSCManager, LPCTSTR DriverId, LPCTSTR DriverPath)
{
	SC_HANDLE hService = NULL;
	BOOL rCode = FALSE;
	DWORD dwSize;
	LPQUERY_SERVICE_CONFIG lpServiceConfig;

	hService = OpenService(hSCManager, DriverId, SERVICE_ALL_ACCESS);
	if (hService != NULL)
	{
		QueryServiceConfig(hService, NULL, 0, &dwSize);
		lpServiceConfig = (LPQUERY_SERVICE_CONFIG)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize);
		QueryServiceConfig(hService, lpServiceConfig, dwSize, &dwSize);

		if (lpServiceConfig->dwStartType == SERVICE_AUTO_START)
			rCode = TRUE;

		CloseServiceHandle(hService);
		HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, lpServiceConfig);
	}

	return rCode;
}

BOOL DriverManager::openDriver()
{
	gHandle = CreateFile(
		_T("\\\\.\\") OLS_DRIVER_ID,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (gHandle == INVALID_HANDLE_VALUE)
		return FALSE;

	return TRUE;
}

BOOL WINAPI Driver::initialize()
{
	if (gInitDll == FALSE)
	{
		if (driverFileExistence() == OLS_DLL_NO_ERROR)
		{
			for (int i = 0; i < 4; i++)
			{
				if (openDriver())
				{
					gDllStatus = OLS_DLL_NO_ERROR;
					break;
				}

				manage(OLS_DRIVER_ID, gDriverPath, OLS_DRIVER_REMOVE);
				if (!manage(OLS_DRIVER_ID, gDriverPath, OLS_DRIVER_INSTALL))
				{
					gDllStatus = OLS_DLL_DRIVER_NOT_LOADED;
					continue;
				}

				if (openDriver())
				{
					gDllStatus = OLS_DLL_NO_ERROR;
					break;
				}

				Sleep(100 * i);
			}
		}
		gInitDll = TRUE;
	}

	return gDllStatus == OLS_DLL_NO_ERROR;
}

VOID WINAPI Driver::deinitialize()
{
	BOOL isHandel = gHandle != INVALID_HANDLE_VALUE;
	if (gInitDll == TRUE && isHandel)
	{
		DWORD length;
		DWORD refCount = 0;
		DWORD result = DeviceIoControl(
			gHandle,
			IOCTL_OLS_GET_REFCOUNT,
			NULL,
			0,
			&refCount,
			sizeof(refCount),
			&length,
			NULL);

		if (refCount == 1)
		{
			CloseHandle(gHandle);
			gHandle = INVALID_HANDLE_VALUE;
			manage(OLS_DRIVER_ID, gDriverPath, OLS_DRIVER_REMOVE);
		}

		if (isHandel)
		{
			CloseHandle(gHandle);
			gHandle = INVALID_HANDLE_VALUE;
		}
		gInitDll = FALSE;
	}
}

BYTE WINAPI Driver::readIoPortByte(BYTE port)
{
	BYTE value = 0;
	bResult = DeviceIoControl(
		gHandle,
		IOCTL_OLS_READ_IO_PORT_BYTE,
		&port,
		sizeof(port),
		&value,
		sizeof(value),
		&bytesReturned,
		NULL);

	return value;
}

VOID WINAPI Driver::writeIoPortByte(BYTE port, BYTE value)
{
	OLS_WRITE_IO_PORT_INPUT inBuf;
	inBuf.PortNumber = port;
	inBuf.CharData = value;
	bResult = DeviceIoControl(
		gHandle,
		IOCTL_OLS_WRITE_IO_PORT_BYTE,
		&inBuf,
		offsetof(OLS_WRITE_IO_PORT_INPUT, CharData) + sizeof(inBuf.CharData),
		NULL,
		0,
		&bytesReturned,
		NULL);
}

BYTE Driver::driverFileExistence()
{
	TCHAR *ptr;
	TCHAR root[4];
	TCHAR dir[MAX_PATH];
	HANDLE hFile;
	WIN32_FIND_DATA findData;

	if (gDriverType == OLS_DRIVER_TYPE_UNKNOWN && IsWindowsVersionOrGreater(5, 0, 0))
	{
		gDllStatus = OLS_DLL_NO_ERROR;
		gDriverType = OLS_DRIVER_TYPE_WIN_NT_X64;
		_tcscpy_s(gDriverFileName, MAX_PATH, OLS_DRIVER_FILE_NAME_WIN_NT_X64);
#ifndef _WIN64
		BOOL wow64 = FALSE;
		IsWow64Process(GetCurrentProcess(), &wow64);
		if (!wow64)
		{
			gDriverType = OLS_DRIVER_TYPE_WIN_NT;
			_tcscpy_s(gDriverFileName, MAX_PATH, OLS_DRIVER_FILE_NAME_WIN_NT);
		}
#endif
	}

	GetModuleFileName(NULL, dir, MAX_PATH);
	if ((ptr = _tcsrchr(dir, '\\')) != NULL)
		*ptr = '\0';
	wsprintf(gDriverPath, _T("%s\\%s"), dir, gDriverFileName);

	// Check file existence
	hFile = FindFirstFile(gDriverPath, &findData);
	if (hFile != INVALID_HANDLE_VALUE)
		FindClose(hFile);
	else
		return OLS_DLL_DRIVER_NOT_FOUND;

	// Check file is not on network location
	root[0] = gDriverPath[0];
	root[1] = ':';
	root[2] = '\\';
	root[3] = '\0';
	if (root[0] == '\\' || GetDriveType((LPCTSTR)root) == DRIVE_REMOTE)
		return OLS_DLL_DRIVER_NOT_LOADED_ON_NETWORK;

	driverFileExist = TRUE;
	return OLS_DLL_NO_ERROR;
}
