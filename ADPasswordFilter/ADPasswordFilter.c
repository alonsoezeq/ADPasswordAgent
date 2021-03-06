#include <stdio.h>
#include <wchar.h>
#include <windows.h>
#include <ntsecapi.h>
#include <ntstatus.h>

#define UNICODE
#define _UNICODE
#define _WIN32_WINNT
#define EXPORT __declspec(dllexport)

wchar_t wcPath[MAX_PATH];

BOOL ReadPath(wchar_t *pwcPath, DWORD *pdwPath)
{
	// Read HKEY_LOCAL_MACHINE\SOFTWARE\ADPasswordFilter\Agent

	HKEY hKey;
	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\ADPasswordFilter", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		if (RegQueryValueExW(hKey, L"Agent", NULL, NULL, (BYTE *)pwcPath, pdwPath) == STATUS_SUCCESS)
		{
			return TRUE;
		}
	}
	RegCloseKey(hKey);
	return FALSE;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	return TRUE;
}

BOOLEAN EXPORT InitializeChangeNotify(void)
{
	wchar_t wcRawPath[MAX_PATH];
	DWORD dwPath = sizeof(wcPath);
	if (ReadPath(wcRawPath, &dwPath))
	{
		// Expand environment strings: %programfiles% -> C:\Program Files
		return ExpandEnvironmentStringsW(wcRawPath, wcPath, MAX_PATH);
	}
	return FALSE;
}

NTSTATUS EXPORT PasswordChangeNotify(PUNICODE_STRING UserName, ULONG RelativeId, PUNICODE_STRING NewPassword)
{
	// Format: "ADPasswordAgent.exe" "Username" "Password"
	wchar_t wcArgv[2048];
	_snwprintf_s(wcArgv, sizeof(wcArgv), (sizeof(wcArgv) / 2) - 1, L"\"%s\" \"%s\" \"%s\"", wcPath, UserName->Buffer, NewPassword->Buffer);

	STARTUPINFOW StartupInfo = { 0 };
	StartupInfo.cb = sizeof(StartupInfo);
	StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
	StartupInfo.wShowWindow = SW_HIDE;
	PROCESS_INFORMATION ProcessInfo;
	if (CreateProcessW(NULL, wcArgv, NULL, NULL, TRUE, 0, NULL, NULL, &StartupInfo, &ProcessInfo))
	{
		CloseHandle(ProcessInfo.hProcess);
		CloseHandle(ProcessInfo.hThread);
	}

	SecureZeroMemory(NewPassword->Buffer, NewPassword->Length);
	SecureZeroMemory(wcArgv, sizeof(wcArgv));

	return STATUS_SUCCESS;
}

BOOLEAN EXPORT PasswordFilter(PUNICODE_STRING AccountName, PUNICODE_STRING FullName, PUNICODE_STRING Password, BOOLEAN SetOperation)
{
	return TRUE;
}
