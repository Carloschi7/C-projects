//ENCRYPTER.H ver 1.01 (c++17 required)

//This document is currently under development, this means there could be some hidden bugs.
//Because of this, every irresponsible use of this header or every data loss issues or any kind of problem
//is on the behalf of the user.
//When the class Encrypter is being used, be sure to leave safe mode on, as it provides some
//safety measures against data loss of which creating a backup file is the main one.
//However, I worked as hard as I could to make this class as stable and functional as I could for this early build

//!!!USING METHODS DEFINED IN THIS NAMESPACE CAN BE REALLY DANGEROUS AS IT CAN CAUSE PERMANENT DATA LOSS
//OR OTHER MAJOR ISSUES, PLEASE USE THEM WITH CAUTION!!!
//ALSO, THIS PIECE OF SOFTWARE IN NOT YET SUPPOSED TO WORK ON NON WINDOWS PLATFORMS AT THE MOMENT

#pragma once
#include <iostream>
#include <string>
#include <fstream>
#include <unordered_map>
#include <random>
#include <cassert>
#include <thread>
#include <filesystem>


namespace Unsafe
{
	class Encrypter;

	enum class RetVal
	{
		OK = 0,
		KEY_NOT_VALID,
		FILE_NOT_FOUND,
		BACKUP_FILE_NOT_DEFINED,
		FILE_MOVED_DURING_PROCESS,
		FILE_ALREADY_ENCRYPTED,
		FILE_ALREADY_DECRYPTED,
		INVALID_CONFIGURATION,
		UNAUTHORIZED_CHANGE,
	};

	//Key used to encrypt and decrypt file:
	//Every char contained in the file will be mapped to another one
	//via the injective function implemented by the m_DirectMap object
	//m_InverseMap is the same as m_DirectMap but reversed to perform decryptions
	//The key can be Manually configured or randomly generated with a mersenne-twisted,
	//high-quality 64bit number generator via the GenKey method.
	//Key Generated via the GenKey method will always be the same with the
	//same seed, so thats a basic alternative for storing the key
	//IF YOU ARE ENCRYPTING MANY FILES, BE SURE TO STORE SAFELY YOUR ENCRYPTION KEY
	//AS IT IS THE ONLY WAY OF GETTING BACK TO THE ORIGINAL FILE.
	class EncryptionKey
	{
	private:
		friend class Encrypter;
		std::unordered_map<char, char> m_DirectMap{};
		std::unordered_map<char, char> m_InverseMap{};
	public:
		EncryptionKey()
		{
			m_DirectMap.reserve(256);
			m_InverseMap.reserve(256);
		}
		EncryptionKey(const EncryptionKey&) = default;

		void SetMapped(char _where, char c)
		{
			m_DirectMap[_where] = c;
			m_InverseMap[c] = _where;
		}
		bool CheckValidity()
		{
			//The mapping function must be injective, otherwise decryption
			//would not be possible
			std::vector<char> found;
			for (uint32_t i = 0; i < 256; i++)
			{
				for (char c : found)
				{
					if (m_DirectMap[i] == c)
						return false;
				}

				found.push_back(m_DirectMap[i]);
			}


			return true;
		}

		//Baisc key gen based on seed
		static EncryptionKey GenKey(uint64_t seed)
		{
			EncryptionKey key;
			std::mt19937_64 rand_gen;
			rand_gen.seed(seed);

			std::vector<char> cache;
			cache.resize(256);
			for (uint32_t i = 0; i < 256; i++)
				cache[i] = i;

			for (uint32_t i = 0; !cache.empty(); i++)
			{
				uint64_t index = rand_gen() % cache.size();
				key.SetMapped(i, cache[index]);
				cache.erase(cache.begin() + index);
			}

			return key;
		}
	};

	//Instance of the main class which encrypts and decrypts files
	//Features:
	//-> Multithreading implementation for faster encrypting/decrypting
	//-> Safe mode which implements backups while the file is encrypted,
	//	 forbids multiple encryptions and decryptions and disables some
	//	 changes while the file is encrypted
	//-> Uses c++17 std::filestystem utilities to manage some operations
	//-> Minimizes the amount of ram used to operate with any file size (default 10MB)
	//-> All of the encrypting operations are previously operated over a temporary file
	//	 (also in unsafe mode) which is then copied into the default one and deleted.
	//	 This was done to prevent any issues if the operating system 
	//	 suddently shuts down for any reason
	class Encrypter
	{
	private:
		using directory = std::filesystem::directory_entry;
		using path = std::filesystem::path;

		path m_FilePath;
		path m_BackupFilePath;
		EncryptionKey m_Key;
		bool bValid, bSafeModeEnabled;
		int32_t m_EncryptionCounter;
		//10MB of maximum binary string size loaded at a time
		//to allow encryptions/decriptions of big files also on devices with
		//little amount of ram
		static constexpr uint64_t m_MaxProcessedBytes = 10000000;
	public:
		Encrypter() :
			Encrypter("", EncryptionKey())
		{}

		Encrypter(const std::string& filename, const EncryptionKey& key) :
			m_FilePath(path(filename)),
			m_Key(key),
			bValid(m_Key.CheckValidity() && !m_FilePath.string().empty()),
			bSafeModeEnabled(true),
			m_EncryptionCounter(0)
		{}

		RetVal SetFile(const std::string& filename)
		{
			//In safemode, we forbid some changes if there is a file being encrypted
			if (bSafeModeEnabled && m_EncryptionCounter != 0)
				return RetVal::UNAUTHORIZED_CHANGE;

			m_FilePath = path(filename);
			bValid = m_Key.CheckValidity() && !m_FilePath.empty();
			return RetVal::OK;
		}

		RetVal SetKey(const EncryptionKey& key)
		{
			if (bSafeModeEnabled && m_EncryptionCounter != 0)
				return RetVal::UNAUTHORIZED_CHANGE;

			m_Key = key;
			bool bValidity = m_Key.CheckValidity();
			if (!bValidity)
			{
				std::cout << "The inserted key is not valid" << std::endl;
				bValid = false;
				return RetVal::KEY_NOT_VALID;
			}

			bValid = bValidity && !m_FilePath.empty();
			return RetVal::OK;
		}

		//Enabled by default
		RetVal EnableSafeMode()
		{
			if (m_EncryptionCounter != 0)
				return RetVal::UNAUTHORIZED_CHANGE;

			bSafeModeEnabled = true;
			return RetVal::OK;
		}

		//WARNING:
		//By calling this function your are disabling safety measures introduced in the code. like:
		//1)Avoiding issuing multiple encryptions/decryptions
		//2)Saving a backup of the original file
		//This all comes with a little performance boost
		RetVal DisableSafeMode()
		{
			if (m_EncryptionCounter != 0)
				return RetVal::UNAUTHORIZED_CHANGE;

			bSafeModeEnabled = false;
			return RetVal::OK;
		}

		RetVal Encrypt()
		{
			if (!bValid)
				return RetVal::INVALID_CONFIGURATION;

			if (bSafeModeEnabled && std::filesystem::exists(_BackupPath()))
				return RetVal::FILE_ALREADY_ENCRYPTED;

			std::fstream f(m_FilePath, std::ios::in | std::ios::binary);
			if (!f.is_open())
				return RetVal::FILE_NOT_FOUND;

			uint64_t file_size = _FileSize(f);
			f.close();

			if (bSafeModeEnabled)
			{
				if (file_size > m_MaxProcessedBytes)
				{
					uint32_t full_iterations = file_size / m_MaxProcessedBytes;

					uint32_t i;
					for (i = 0; i < full_iterations; i++)
						_PartialBackupSave(m_MaxProcessedBytes * i, m_MaxProcessedBytes);

					//Last iteration
					std::streampos off = m_MaxProcessedBytes * i;
					_PartialBackupSave(off, file_size - off);
				}
				else
				{
					_PartialBackupSave(0, file_size);
				}
			}
			
			if (file_size > m_MaxProcessedBytes)
			{
				uint32_t full_iterations = file_size / m_MaxProcessedBytes;

				uint32_t i;
				for (i = 0; i < full_iterations; i++)
					_PartialEncrypt(m_MaxProcessedBytes * i, m_MaxProcessedBytes);

				//Last iteration
				std::streampos off = m_MaxProcessedBytes * i;
				_PartialEncrypt(off, file_size - off);
			}
			else
			{
				_PartialEncrypt(0, file_size);
			}

			const auto copyOptions = std::filesystem::copy_options::update_existing;
			std::filesystem::copy(_TempPath(), std::filesystem::absolute(m_FilePath), copyOptions);
			std::filesystem::remove(_TempPath());

			if(bSafeModeEnabled) m_EncryptionCounter++;

			return RetVal::OK;
		}

		RetVal Decrypt()
		{
			if (!bValid)
				return RetVal::INVALID_CONFIGURATION;

			std::fstream f(m_FilePath, std::ios::in | std::ios::out | std::ios::binary);
			if (!f.is_open())
				return RetVal::FILE_NOT_FOUND;

			uint64_t file_size = _FileSize(f);
			f.close();


			if (file_size > m_MaxProcessedBytes)
			{
				uint32_t full_iterations = file_size / m_MaxProcessedBytes;

				uint32_t i;
				for (i = 0; i < full_iterations; i++)
					_PartialDecrypt(m_MaxProcessedBytes * i, m_MaxProcessedBytes);

				//Last iteration
				std::streampos off = m_MaxProcessedBytes * i;
				_PartialDecrypt(off, file_size - off);
			}
			else
			{
				_PartialDecrypt(0, file_size);
			}

			const auto copyOptions = std::filesystem::copy_options::update_existing;
			std::filesystem::copy(_TempPath(), std::filesystem::absolute(m_FilePath), copyOptions);
			std::filesystem::remove(_TempPath());
			if(bSafeModeEnabled)
				std::filesystem::remove(_BackupPath());

			if (bSafeModeEnabled) m_EncryptionCounter--;

			return RetVal::OK;
		}

		//Copies the _backup file into the original
		RetVal LoadBackup()
		{
			std::string _filebuf;
			std::fstream file;

			m_BackupFilePath = _BackupPath();
			if (m_BackupFilePath.string().empty())
				return RetVal::BACKUP_FILE_NOT_DEFINED;

			file.open(m_BackupFilePath, std::ios::in | std::ios::binary);
			if(!file.is_open())
				return RetVal::BACKUP_FILE_NOT_DEFINED;

			_filebuf.resize(_FileSize(file));
			file.read((char*)_filebuf.c_str(), _filebuf.size());
			file.close();
			file.open(m_FilePath, std::ios::out | std::ios::binary);
			if (!file.is_open())
				return RetVal::FILE_NOT_FOUND;

			file.write(_filebuf.c_str(), _filebuf.size());
			file.close();

			return RetVal::OK;
		}
	private:
		path _BackupPath()
		{
			if (m_FilePath.empty()) return path();

			std::string local_path = std::filesystem::absolute(m_FilePath).string();
			//File name with the \\ at the beginning
			std::string file_name = local_path.substr(
				local_path.find_last_of('\\'),
				local_path.find_last_of('.') - local_path.find_last_of('\\'));
			std::string extension = local_path.substr(local_path.find_last_of('.'));

			directory local_dir(path(local_path.substr(0, local_path.find_last_of('\\'))));
			return path(local_dir.path().string() + file_name + "_backup" + extension);
		}

		path _TempPath()
		{
			if (m_FilePath.empty()) return path();

			std::string local_path = std::filesystem::absolute(m_FilePath).string();
			//File name with the \\ at the beginning
			std::string file_name = local_path.substr(
				local_path.find_last_of('\\'),
				local_path.find_last_of('.') - local_path.find_last_of('\\'));
			std::string extension = local_path.substr(local_path.find_last_of('.'));

			directory local_dir(path(local_path.substr(0, local_path.find_last_of('\\'))));
			return path(local_dir.path().string() + file_name + "_temp" + extension);
		}

		RetVal _PartialBackupSave(std::streampos start, uint64_t size)
		{
			std::string _filebuf;
			_filebuf.resize(size);
			std::fstream file(m_FilePath, std::ios::in | std::ios::binary);

			if (!file.is_open())
				return RetVal::FILE_NOT_FOUND;

			file.seekg(start);
			file.read((char*)_filebuf.c_str(), _filebuf.size());
			file.close();

			if(start == 0)
				file.open(_BackupPath(), std::ios::out | std::ios::binary);
			else
				file.open(_BackupPath(), std::ios::out | std::ios::binary | std::ios::app);

			if (!file.is_open())
				return RetVal::FILE_NOT_FOUND;

			file.seekp(start);
			file.write(_filebuf.c_str(), _filebuf.size());
			file.close();
			return RetVal::OK;
		}

		RetVal _PartialEncrypt(std::streampos start, uint64_t size)
		{
			std::string _filebuf;

			_filebuf.resize(size);
			std::fstream file;
			file.open(m_FilePath.c_str(), std::ios::in | std::ios::out | std::ios::binary);
			if (!file.is_open())
				return RetVal::FILE_NOT_FOUND;

			file.seekg(start);
			file.read((char*)_filebuf.c_str(), _filebuf.size());
			file.close();

#ifdef _DEBUG
			auto tp1 = std::chrono::steady_clock::now();
#endif
			constexpr uint32_t Tc = 4; //Thread count
			std::thread t[Tc];
			for (uint32_t i = 0; i < Tc - 1; i++)
				t[i] = std::thread(&Encrypter::_Encrypt, this, &_filebuf, (_filebuf.size() / Tc) * i, (_filebuf.size() / Tc) * (i + 1));

			t[Tc - 1] = std::thread(&Encrypter::_Encrypt, this, &_filebuf, (_filebuf.size() / Tc) * (Tc - 1), _filebuf.size());

			//Join every thread
			for (uint32_t i = 0; i < Tc; i++) t[i].join();

#ifdef _DEBUG
			auto tp2 = std::chrono::steady_clock::now();
			std::cout << "Encryption process lasted at file offset " << start << ": " <<
				std::chrono::duration<float>(tp2 - tp1).count() << " seconds" << std::endl;
#endif

			if (start == 0)
				file.open(_TempPath(), std::ios::out | std::ios::binary);
			else
				file.open(_TempPath(), std::ios::out | std::ios::binary | std::ios::app);

			if (!file.is_open())
				return RetVal::FILE_MOVED_DURING_PROCESS;

			file.seekp(start);

			file.write(_filebuf.c_str(), _filebuf.size());
			file.close();

			return RetVal::OK;
		}

		RetVal _PartialDecrypt(std::streampos start, uint64_t size)
		{
			std::string _filebuf;

			_filebuf.resize(size);
			std::fstream file;
			file.open(m_FilePath.c_str(), std::ios::in | std::ios::binary);
			if (!file.is_open())
				return RetVal::FILE_NOT_FOUND;

			file.seekg(start);
			file.read((char*)_filebuf.c_str(), _filebuf.size());
			file.close();

#ifdef _DEBUG
			auto tp1 = std::chrono::steady_clock::now();
#endif
			constexpr uint32_t Tc = 4; //Thread count
			std::thread t[Tc];
			for (uint32_t i = 0; i < Tc - 1; i++)
				t[i] = std::thread(&Encrypter::_Decrypt, this, &_filebuf, (_filebuf.size() / Tc) * i, (_filebuf.size() / Tc) * (i + 1));

			t[Tc - 1] = std::thread(&Encrypter::_Decrypt, this, &_filebuf, (_filebuf.size() / Tc) * (Tc - 1), _filebuf.size());

			//Join every thread
			for (uint32_t i = 0; i < Tc; i++) t[i].join();

#ifdef _DEBUG
			auto tp2 = std::chrono::steady_clock::now();
			std::cout << "Decryption process lasted at file offset " << start << ": " <<
				std::chrono::duration<float>(tp2 - tp1).count() << " seconds" << std::endl;
#endif

			if(start == 0)
				file.open(_TempPath(), std::ios::out | std::ios::binary);
			else
				file.open(_TempPath(), std::ios::out | std::ios::binary | std::ios::app);
			
			if (!file.is_open())
				return RetVal::FILE_MOVED_DURING_PROCESS;

			file.seekp(start);
			file.write(_filebuf.c_str(), _filebuf.size());
			file.close();

			return RetVal::OK;
		}

		void _Encrypt(std::string* _filebuf, uint32_t start, uint32_t end)
		{
			for (uint64_t i = start; i < end; i++)
			{
				_filebuf->at(i) = m_Key.m_DirectMap[_filebuf->at(i)];
			}
		}

		void _Decrypt(std::string* _filebuf, uint32_t start, uint32_t end)
		{
			for (uint64_t i = start; i < end; i++)
			{
				_filebuf->at(i) = m_Key.m_InverseMap[_filebuf->at(i)];
			}
		}

		std::streampos _FileSize(std::fstream& file)
		{
			std::streampos off = file.tellg();
			file.seekg(0, std::ios::end);
			std::streampos file_size = file.tellg();
			file.seekg(off);
			return file_size;
		}
	};
}
