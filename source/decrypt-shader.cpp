#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <span>

#include <Common/Format.hpp>

#include <mio/mmap.hpp>

const char Help[]
	= "Decrypts the `Shaders/*.{bin,enc}` files found within Halo CE and PC \n"
	  "Usage: ./decrypt-shader {list of files}\n"
	  "\tfor each file, a {file}.decrypted.bin will be created along side it\n"
	  "\twith the decrypted contents\n";

// Found within the Halo Custom Edition executable
const static std::uint32_t HaloCEKey[4] = {
	0x3FFFFFDD,
	0x00007FC3,
	0x000000E5,
	0x003FFFEF,
};

void TEADecryptBlock(
	const std::span<std::uint32_t, 2> Data,
	std::span<const std::uint32_t, 4> Key)
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

	mio::mmap_sink           DecryptedFile = mio::mmap_sink(OutFile.c_str());
	std::span<std::uint32_t> DecryptedData(
		reinterpret_cast<std::uint32_t*>(DecryptedFile.data()),
		DecryptedFile.size() / sizeof(std::uint32_t));

	if( DecryptedFile.size() & 0x80000007 )
	{
		TEADecryptBlock(
			std::span<std::uint32_t>(
				reinterpret_cast<std::uint32_t*>(
					DecryptedFile.data() + DecryptedFile.size() - 8),
				2)
				.first<2>(),
			HaloCEKey);
	}

	while( DecryptedData.size() >= 2 )
	{
		const std::span<std::uint32_t, 2> CurSpan = DecryptedData.first<2>();
		TEADecryptBlock(CurSpan, HaloCEKey);
		DecryptedData = DecryptedData.subspan(2);
	}

	return true;
}

/* For ImHex
struct VertexShaderBlob
{
	u32 Size;
	u8 Data[Size];
};

VertesShaderBlob shaders[64] @0x00;
*/
bool DumpVertexShaderFile(std::span<const std::byte> ShaderFile)
{
	for( std::uint8_t i = 0; i < 64; ++i )
	{
		const std::uint32_t& ShaderDataSize
			= *reinterpret_cast<const std::uint32_t*>(ShaderFile.data());
		ShaderFile = ShaderFile.subspan(sizeof(uint32_t));

		const std::span<const std::byte> VertexShaderByteCode
			= ShaderFile.subspan(0, ShaderDataSize);
		ShaderFile = ShaderFile.subspan(ShaderDataSize);

		std::ofstream OutFile(Common::Format("vsh.%02zu.dxso", i));

		if( OutFile )
		{
			OutFile.write(
				reinterpret_cast<const char*>(VertexShaderByteCode.data()),
				VertexShaderByteCode.size());
		}
		else
		{
			std::fprintf(stderr, "Error dumping shader");
		}
	}

	return true;
}

struct FragmentShaderEntry
{
	std::uint32_t PermutationCount;
	char          Name[0x80];
};
static_assert(sizeof(FragmentShaderEntry) == 0x84);

// THis is particularly for Halo CE. Will not work with Halo PC
bool DumpFragmentShaderFileCE(const std::span<const std::byte> ShaderFile)
{
	auto StreamSpan = ShaderFile;

	const std::uint32_t& ShaderCount
		= *reinterpret_cast<const std::uint32_t*>(ShaderFile.data());
	StreamSpan = StreamSpan.subspan(sizeof(uint32_t));

	for( std::uint8_t ShaderIdx = 0; ShaderIdx < ShaderCount; ++ShaderIdx )
	{
		const std::uint32_t& ShaderNameLength
			= *reinterpret_cast<const std::uint32_t*>(StreamSpan.data());
		StreamSpan = StreamSpan.subspan(sizeof(uint32_t));

		const auto ShaderNameData = StreamSpan.subspan(0, ShaderNameLength);
		StreamSpan                = StreamSpan.subspan(ShaderNameLength);

		const char* ShaderName = (const char*)ShaderNameData.data();

		const std::uint32_t& ShaderPermutationCount
			= *reinterpret_cast<const std::uint32_t*>(StreamSpan.data());
		StreamSpan = StreamSpan.subspan(sizeof(uint32_t));

		std::printf(
			"%.*s | Permutations: %u\n", ShaderNameLength, ShaderName,
			ShaderPermutationCount);

		for( std::uint8_t PermutationIdx = 0;
			 PermutationIdx < ShaderPermutationCount; ++PermutationIdx )
		{
			const std::uint32_t& PermutationNameLength
				= *reinterpret_cast<const std::uint32_t*>(StreamSpan.data());
			StreamSpan = StreamSpan.subspan(sizeof(uint32_t));

			const auto PermutationNameData
				= StreamSpan.subspan(0, PermutationNameLength);
			StreamSpan = StreamSpan.subspan(PermutationNameLength);

			const char* PermutationName
				= (const char*)PermutationNameData.data();
			std::printf("\t - %.*s\n", PermutationNameLength, PermutationName);

			const std::uint32_t& PermutationShaderDataLength
				= *reinterpret_cast<const std::uint32_t*>(StreamSpan.data());
			StreamSpan = StreamSpan.subspan(sizeof(uint32_t));

			const auto PermutationShaderData
				= StreamSpan.subspan(0, PermutationShaderDataLength * 4);
			StreamSpan = StreamSpan.subspan(PermutationShaderDataLength * 4);
		}
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
		std::fprintf(
			stdout, "%s -> %s: ", InPath.string().c_str(),
			OutPath.string().c_str());
		if( DecryptShader(InPath, OutPath) )
		{
			std::fprintf(stdout, "Done\n");

			mio::mmap_source DecryptedFile = mio::mmap_source(OutPath.c_str());
			std::span<const std::byte> DecryptedData(
				reinterpret_cast<const std::byte*>(DecryptedFile.data()),
				DecryptedFile.size());

			// DumpVertexShaderFile(DecryptedData);
			DumpFragmentShaderFileCE(DecryptedData);
		}
		else
		{
			std::fprintf(stdout, "Failed to decrypt file\n");
		}
	}

	return EXIT_SUCCESS;
}