#pragma once

namespace dn
{
	typedef uint8_t uint8;
	typedef uint16_t uint16;
	typedef uint32_t uint32;
	typedef uint64_t uint64;
	typedef uint32 PAKHANDLE;

#define DNPAKLOGGER "dn-pak"
#define INIT_DNPAK_LOGGER el::Logger* dnPakLogger = el::Loggers::getLogger("dn-pak");
#define PAKHANDLE_INVALID 0


	struct PakHeader;
	struct PakFileTableEntry;
	class OPENPAKFILEHANDLE;
	class cPakData;
	class cPak;

	typedef struct PakHeader
	{
		uint8 hMagic[256];
		uint32 hVersion;
		uint32 hFileCount;
		uint32 hFileTableOffset;
	} PakHeader;

	const size_t szPakHeader = sizeof(uint8) * 256 +
		sizeof(uint32) * 3;

	typedef struct PakFileTableEntry
	{
		std::string eFilePath;
		uint32 eDiskSize;
		uint32 eDecompressedSize;
		uint32 eCompressedSize;
		uint32 eDataOffset;
		uint32 eUnknownA;
		uint8 eReserved[40];

		PakFileTableEntry();
		PakFileTableEntry(void* ignored);
	} PakFileTableEntry;

	const size_t szPakFileTableEntry = 256 +
		sizeof(uint32) * 5 +
		sizeof(uint8) * 40;

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
		typedef std::unordered_map<std::string, PakFileTableEntry> PakFileTable;
	private:
		PakHeader _header;
		PakFileTable _fileTable;
		std::unique_ptr<OPENPAKFILEHANDLE> _handle;
		std::mutex _readMutex;

		void _ReadHeader(const std::wstring file);
		void _ReadIndex(const std::wstring file);

	public:
		explicit cPak(LPCWSTR file);
		~cPak() = default;

		std::unique_ptr<cPakData> GetFileData(const std::string fileName);

		std::vector<PakFileTableEntry> GetFiles();

		PakFileTable GetFileTable();
	};


	class cPakManager
	{
	private:
		typedef std::unordered_map<std::string, PAKHANDLE> HandleMap;

		struct FileEntry
		{
			std::wstring fullPath;
			std::wstring fileName;

			FileEntry(std::wstring fp, std::wstring fn)
			{
				fullPath = fp;
				fileName = fn;
			}
		};


		packed_freelist<cPak> _loadedPaks;
		HandleMap _pathToPakMap;

		static bool comparePakFileNames(FileEntry a, FileEntry b);

	public:
		cPakManager();
		~cPakManager();

		PAKHANDLE LoadPak(std::wstring pathToPak);

		bool LoadDir(std::wstring pathToDir);

		bool FileExists(const std::string& path);

		std::unique_ptr<cPakData> GetFileData(const std::string& path);

		void Unload();
	};

	int _memcpyIncr(void* dest, const void* src, const size_t sz);
	bool _endsWith(std::wstring const &a, std::wstring const &suffix);
	bool _startsWith(std::wstring const &a, std::wstring const &b);


}
