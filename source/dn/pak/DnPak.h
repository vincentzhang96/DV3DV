#pragma once
#include "../../stdafx.h"
#include <mutex>
#include "../../ppac/PhoenixPAC.h"

namespace dn
{
	typedef uint8_t uint8;
	typedef uint16_t uint16;
	typedef uint32_t uint32;
	typedef uint64_t uint64;

	struct PakHeader;
	struct PakFileTableEntry;
	class OPENPAKFILEHANDLE;
	class cPakData;
	class cPak;

	typedef struct PakHeader
	{
		uint8 hMagic[1024];
		uint32 hVersion;
		uint32 hFileCount;
		uint32 hFileTableOffset;
	} PakHeader;

	typedef struct PakFileTableEntry
	{
		uint8 eFilePath[256];
		uint32 eDiskSize;
		uint32 eDecompressedSize;
		uint32 eCompressedSize;
		uint32 eDataOffset;
		uint32 eUnknownA;
		uint8 eReserved[40];
	} PakFileTableEntry;

	class OPENPAKFILEHANDLE
	{
	public:
		explicit OPENPAKFILEHANDLE(HANDLE handle, std::wstring name);
		~OPENPAKFILEHANDLE();
		HANDLE _handle;
		std::wstring _name;
	};

	class cPakData
	{
	public:
		cPakData(std::string name, uint32 size);
		~cPakData();
		std::string _name;
		uint32 _size;
		std::vector<uint8> _data;
	};

	class cPak
	{
	private:
		PakHeader _header;
		std::vector<PakFileTableEntry> _fileTable;
		std::unique_ptr<OPENPAKFILEHANDLE> _handle;
		std::mutex _readMutex;

		bool _ReadHeader(const std::wstring file);
		void _ReadIndex(const bool needSwp, const std::wstring file);

	public:
		explicit cPak(LPCWSTR file);
		~cPak() = default;

		std::unique_ptr<cPakData> GetFileData(const std::string fileName);

		std::vector<PakFileTableEntry> GetEntries();
	};
}
