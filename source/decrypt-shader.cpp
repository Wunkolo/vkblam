#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <span>

#include <mio/mmap.hpp>

const char Help[]
	= "Decrypts the `Shaders/*.{bin,enc}` files found within Halo CE and PC \n"
	  "Usage: ./decrypt-shader ";

// Found within the Halo Custom Edition executable
const static std::uint32_t Key[4] = {
	0x3FFFFFDD,
	0x00007FC3,
	0x000000E5,
	0x003FFFEF,
};

void TEADecrypt(
	std::span<std::uint32_t, 2> Data, std::span<const std::uint32_t, 4> Key)
{
	const std::uint32_t Delta = 0x61C88647;
	std::uint32_t       Sum   = 0xC6EF3720;

	std::uint32_t V0 = Data[0];
	std::uint32_t V1 = Data[1];

	for( std::uint8_t i = 0; i < 32; i++ )
	{
		V1 -= (Sum + V0) ^ ((Key[2] + (V0 << 4) ^ Key[3] + (V0 >> 5)));
		V0 -= (Sum + V1) ^ ((Key[0] + (V1 << 4) ^ Key[1] + (V1 >> 5)));
		Sum += Delta;
	}
	Data[0] = V0;
	Data[1] = V1;
}

// - If the file is larger than `0x80000007`, then decrypt the last 8 bytes
// - For each group of 8 bytes decrypt each 8 bytes
// - The last 32 bytes of the decrypted file are an ascii MD5 hash of all of the
// data before it
bool DecryptShader(
	const std::filesystem::path& InFile, const std::filesystem::path& OutFile)
{
	std::error_code ErrorCode;
	if( !std::filesystem::exists(InFile) )
	{
		// File does not exists
		return false;
	}
	if( std::filesystem::file_size(InFile) < 8 )
	{
		// File is not valid
		return false;
	}

	std::filesystem::create_directories(OutFile.parent_path(), ErrorCode);
	if( ErrorCode )
	{
		std::fprintf(
			stderr, "create_directories(%s): %s",
			OutFile.parent_path().string().c_str(),
			ErrorCode.message().c_str());
		return false;
	}
	std::filesystem::copy_file(
		InFile, OutFile, std::filesystem::copy_options::overwrite_existing,
		ErrorCode);
	if( ErrorCode )
	{
		std::fprintf(
			stderr, "copy_file(%s -> %s): %s", InFile.string().c_str(),
			OutFile.string().c_str(), ErrorCode.message().c_str());
		return false;
	}

	auto                     DecryptedFile = mio::mmap_sink(OutFile.c_str());
	std::span<std::uint32_t> DecryptedData(
		reinterpret_cast<std::uint32_t*>(DecryptedFile.data()),
		DecryptedFile.size() / sizeof(std::uint32_t));

	while( DecryptedData.size() > 2 )
	{
		auto CurSpan = DecryptedData.first<2>();
		TEADecrypt(CurSpan, Key);
		DecryptedData = DecryptedData.subspan(2);
	}

	DecryptedFile.sync(ErrorCode);

	if( ErrorCode )
	{
		std::fprintf(
			stderr, "sync(%s): %s", OutFile.string().c_str(),
			ErrorCode.message().c_str());
		return false;
	}

	return true;
}

int main(int argc, char* argv[])
{
	if( argc <= 1 )
	{
		// Not enough arguments
		return EXIT_FAILURE;
	}

	for( int i = 1; i < argc; ++i )
	{
		const std::filesystem::path InPath = argv[i];
		const std::filesystem::path OutPath
			= std::filesystem::path(InPath).replace_extension(".decrypted.bin");
		DecryptShader(InPath, OutPath);
	}

	return EXIT_SUCCESS;
}