#include "Blam/Enums.hpp"
#include "Blam/Types.hpp"
#include "Blam/Util.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <map>
#include <span>

#include <Common/Alignment.hpp>
#include <Common/Format.hpp>
#include <Common/Literals.hpp>

#include <Vulkan/Debug.hpp>
#include <Vulkan/DescriptorHeap.hpp>
#include <Vulkan/DescriptorUpdateBatch.hpp>
#include <Vulkan/Memory.hpp>
#include <Vulkan/Pipeline.hpp>
#include <Vulkan/StreamBuffer.hpp>
#include <Vulkan/VulkanAPI.hpp>

#include <VkBlam/Shaders/ShaderEnvironment.hpp>
#include <VkBlam/VkBlam.hpp>

#include <mio/mmap.hpp>

#include <cmrc/cmrc.hpp>

CMRC_DECLARE(vkblam);
auto DataFS = cmrc::vkblam::get_filesystem();

#include <Blam/Blam.hpp>

#include "stb_image_write.h"

#define CAPTURE
#ifdef CAPTURE
#include <dlfcn.h>
#include <renderdoc_app.h>
RENDERDOC_API_1_4_1* rdoc_api = NULL;
#endif

static constexpr glm::uvec2              RenderSize = {1024, 1024};
static constexpr vk::SampleCountFlagBits RenderSamples
	= vk::SampleCountFlagBits::e4;

vk::UniqueRenderPass CreateMainRenderPass(
	vk::Device              Device,
	vk::SampleCountFlagBits SampleCount = vk::SampleCountFlagBits::e1);

vk::UniqueFramebuffer CreateMainFrameBuffer(
	vk::Device Device, vk::ImageView Color, vk::ImageView DepthAA,
	vk::ImageView ColorAA, glm::uvec2 ImageSize, vk::RenderPass RenderPass);

std::tuple<vk::UniquePipeline, vk::UniquePipelineLayout> CreateGraphicsPipeline(
	vk::Device Device, std::span<const vk::PushConstantRange> PushConstants,
	std::span<const vk::DescriptorSetLayout> SetLayouts,
	vk::ShaderModule VertModule, vk::ShaderModule FragModule,
	vk::RenderPass  RenderPass,
	vk::PolygonMode PolygonMode = vk::PolygonMode::eFill);

std::string FormatDeviceCaps(vk::PhysicalDevice PhysicalDevice);

int main(int argc, char* argv[])
{
	using namespace Common::Literals;

	if( argc < 3 )
	{
		// Not enough arguments
		return EXIT_FAILURE;
	}

#ifdef CAPTURE
	void* mod = dlopen("librenderdoc.so", RTLD_NOW);
	char* msg = dlerror();
	if( msg )
		std::puts(msg);
	if( mod )
	{
		pRENDERDOC_GetAPI RENDERDOC_GetAPI
			= (pRENDERDOC_GetAPI)dlsym(mod, "RENDERDOC_GetAPI");
		int ret
			= RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_4_1, (void**)&rdoc_api);
		rdoc_api->SetCaptureOptionU32(eRENDERDOC_Option_APIValidation, 1);
		rdoc_api->SetCaptureOptionU32(eRENDERDOC_Option_CaptureCallstacks, 1);
		rdoc_api->SetCaptureOptionU32(eRENDERDOC_Option_SaveAllInitials, 1);
		rdoc_api->SetCaptureOptionU32(eRENDERDOC_Option_DebugOutputMute, 0);
		assert(ret == 1);
	}
#endif

	std::filesystem::path CurPath(argv[1]);
	std::filesystem::path BitmapPath(argv[2]);
	auto                  MapFile    = mio::mmap_source(CurPath.c_str());
	auto                  BitmapFile = mio::mmap_source(BitmapPath.c_str());

	Blam::MapFile CurMap(std::span<const std::byte>(
		reinterpret_cast<const std::byte*>(MapFile.data()), MapFile.size()));

	std::fputs(Blam::ToString(CurMap.MapHeader).c_str(), stdout);
	std::fputs(Blam::ToString(CurMap.TagIndexHeader).c_str(), stdout);

	glm::f32vec3 WorldBoundMax(std::numeric_limits<float>::min());
	glm::f32vec3 WorldBoundMin(std::numeric_limits<float>::max());

	//// Create Instance
	vk::InstanceCreateInfo InstanceInfo = {};

	vk::ApplicationInfo ApplicationInfo = {};
	ApplicationInfo.apiVersion          = VK_API_VERSION_1_1;

	ApplicationInfo.pEngineName   = "VkBlam";
	ApplicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);

	ApplicationInfo.pApplicationName   = "VkBlam";
	ApplicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);

	InstanceInfo.pApplicationInfo = &ApplicationInfo;

	static const char* InstanceExtensions[]
		= {VK_EXT_DEBUG_UTILS_EXTENSION_NAME};

	InstanceInfo.ppEnabledExtensionNames = InstanceExtensions;
	InstanceInfo.enabledExtensionCount   = std::size(InstanceExtensions);

	vk::UniqueInstance Instance = {};

	if( auto CreateResult = vk::createInstanceUnique(InstanceInfo);
		CreateResult.result == vk::Result::eSuccess )
	{
		Instance = std::move(CreateResult.value);
	}
	else
	{
		std::fprintf(
			stderr, "Error creating Vulkan instance: %s\n",
			vk::to_string(CreateResult.result).c_str());
		return EXIT_FAILURE;
	}
	VULKAN_HPP_DEFAULT_DISPATCHER.init(Instance.get());

	//// Pick physical device
	vk::PhysicalDevice PhysicalDevice = {};

	if( auto EnumerateResult = Instance->enumeratePhysicalDevices();
		EnumerateResult.result == vk::Result::eSuccess )
	{
		for( const auto& CurPhysicalDevice : EnumerateResult.value )
		{
			// Just pick the first physical device
			if( CurPhysicalDevice.getProperties().deviceType
					== vk::PhysicalDeviceType::eDiscreteGpu
				&& CurPhysicalDevice.getProperties().vendorID == 0x10DE )
			{
				PhysicalDevice = CurPhysicalDevice;
				break;
			}
		}
	}
	else
	{
		std::fprintf(
			stderr, "Error enumerating physical devices: %s\n",
			vk::to_string(EnumerateResult.result).c_str());
		return EXIT_FAILURE;
	}

	std::fprintf(
		stdout,
		"---\n"
		"%s"
		"---\n",
		FormatDeviceCaps(PhysicalDevice).c_str());

	// We're putting images/buffers right after each other, so we need to
	// ensure they are far apart enough to not be considered aliasing
	const std::size_t BufferImageGranularity
		= PhysicalDevice.getProperties().limits.bufferImageGranularity;

	//// Create Device
	vk::DeviceCreateInfo DeviceInfo = {};

	static const char* DeviceExtensions[]
		= {VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME};
	DeviceInfo.ppEnabledExtensionNames = DeviceExtensions;
	DeviceInfo.enabledExtensionCount   = std::size(DeviceExtensions);

	vk::StructureChain<
		vk::PhysicalDeviceFeatures2,
		vk::PhysicalDeviceTimelineSemaphoreFeatures>
		DeviceFeatureChain = {};

	auto& DeviceFeatures
		= DeviceFeatureChain.get<vk::PhysicalDeviceFeatures2>().features;
	DeviceFeatures.sampleRateShading = true;
	DeviceFeatures.wideLines         = true;
	DeviceFeatures.fillModeNonSolid  = true;

	auto& DeviceTimelineFeatures
		= DeviceFeatureChain.get<vk::PhysicalDeviceTimelineSemaphoreFeatures>();
	DeviceTimelineFeatures.timelineSemaphore = true;

	DeviceInfo.pNext = &DeviceFeatureChain.get();

	static const float QueuePriority = 1.0f;

	vk::DeviceQueueCreateInfo QueueInfo = {};
	QueueInfo.queueFamilyIndex          = 0;
	QueueInfo.queueCount                = 1;
	QueueInfo.pQueuePriorities          = &QueuePriority;

	DeviceInfo.queueCreateInfoCount = 1;
	DeviceInfo.pQueueCreateInfos    = &QueueInfo;

	vk::UniqueDevice Device = {};
	if( auto CreateResult = PhysicalDevice.createDeviceUnique(DeviceInfo);
		CreateResult.result == vk::Result::eSuccess )
	{
		Device = std::move(CreateResult.value);
	}
	else
	{
		std::fprintf(
			stderr, "Error creating logical device: %s\n",
			vk::to_string(CreateResult.result).c_str());
		return EXIT_FAILURE;
	}

	VULKAN_HPP_DEFAULT_DISPATCHER.init(Device.get());

#ifdef CAPTURE
	if( rdoc_api )
		rdoc_api->StartFrameCapture(
			*(void**)(VkInstance)(*Instance.operator->()), NULL);
#endif

	// Main Rendering queue
	vk::Queue RenderQueue = Device->getQueue(0, 0);

	//// Main Render Pass
	vk::UniqueRenderPass MainRenderPass
		= CreateMainRenderPass(Device.get(), RenderSamples);

	// Buffers
	Vulkan::StreamBuffer StreamBuffer(
		Device.get(), PhysicalDevice, RenderQueue, 0, 64_MiB);

	//// Create Default Sampler
	vk::SamplerCreateInfo SamplerInfo{};
	SamplerInfo.magFilter               = vk::Filter::eLinear;
	SamplerInfo.minFilter               = vk::Filter::eLinear;
	SamplerInfo.mipmapMode              = vk::SamplerMipmapMode::eLinear;
	SamplerInfo.addressModeU            = vk::SamplerAddressMode::eRepeat;
	SamplerInfo.addressModeV            = vk::SamplerAddressMode::eRepeat;
	SamplerInfo.addressModeW            = vk::SamplerAddressMode::eRepeat;
	SamplerInfo.mipLodBias              = 0.0f;
	SamplerInfo.anisotropyEnable        = VK_FALSE;
	SamplerInfo.maxAnisotropy           = 1.0f;
	SamplerInfo.compareEnable           = VK_FALSE;
	SamplerInfo.compareOp               = vk::CompareOp::eAlways;
	SamplerInfo.minLod                  = 0.0f;
	SamplerInfo.maxLod                  = VK_LOD_CLAMP_NONE;
	SamplerInfo.borderColor             = vk::BorderColor::eFloatOpaqueWhite;
	SamplerInfo.unnormalizedCoordinates = VK_FALSE;

	vk::UniqueSampler DefaultSampler = {};
	if( auto CreateResult = Device->createSamplerUnique(SamplerInfo);
		CreateResult.result == vk::Result::eSuccess )
	{
		DefaultSampler = std::move(CreateResult.value);
	}
	else
	{
		std::fprintf(
			stderr, "Error creating default sampler: %s\n",
			vk::to_string(CreateResult.result).c_str());
		return EXIT_FAILURE;
	}
	Vulkan::SetObjectName(
		Device.get(), DefaultSampler.get(), "Default Sampler");

	vk::UniqueBuffer       StagingBuffer              = {};
	vk::UniqueDeviceMemory StagingBufferMemory        = {};
	std::size_t            StagingBufferWritePosition = 0;

	vk::BufferCreateInfo StagingBufferInfo = {};
	StagingBufferInfo.size                 = std::max(
						128_MiB, RenderSize.x * RenderSize.y * sizeof(std::uint32_t));
	StagingBufferInfo.usage = vk::BufferUsageFlagBits::eTransferDst
							| vk::BufferUsageFlagBits::eTransferSrc;

	if( auto CreateResult = Device->createBufferUnique(StagingBufferInfo);
		CreateResult.result == vk::Result::eSuccess )
	{
		StagingBuffer = std::move(CreateResult.value);
	}
	else
	{
		std::fprintf(
			stderr, "Error creating staging buffer: %s\n",
			vk::to_string(CreateResult.result).c_str());
		return EXIT_FAILURE;
	}

	Vulkan::SetObjectName(
		Device.get(), StagingBuffer.get(), "Staging Buffer( %s )",
		Common::FormatByteCount(StagingBufferInfo.size).c_str());

	// Allocate memory for staging buffer
	{
		const vk::MemoryRequirements StagingBufferMemoryRequirements
			= Device->getBufferMemoryRequirements(StagingBuffer.get());

		vk::MemoryAllocateInfo StagingBufferAllocInfo = {};
		StagingBufferAllocInfo.allocationSize
			= StagingBufferMemoryRequirements.size;
		StagingBufferAllocInfo.memoryTypeIndex = Vulkan::FindMemoryTypeIndex(
			PhysicalDevice, StagingBufferMemoryRequirements.memoryTypeBits,
			vk::MemoryPropertyFlagBits::eHostVisible
				| vk::MemoryPropertyFlagBits::eHostCoherent);

		if( auto AllocResult
			= Device->allocateMemoryUnique(StagingBufferAllocInfo);
			AllocResult.result == vk::Result::eSuccess )
		{
			StagingBufferMemory = std::move(AllocResult.value);
		}
		else
		{
			std::fprintf(
				stderr, "Error allocating memory for staging buffer: %s\n",
				vk::to_string(AllocResult.result).c_str());
			return EXIT_FAILURE;
		}

		if( auto BindResult = Device->bindBufferMemory(
				StagingBuffer.get(), StagingBufferMemory.get(), 0);
			BindResult == vk::Result::eSuccess )
		{
			// Successfully binded memory to buffer
		}
		else
		{
			std::fprintf(
				stderr, "Error binding memory to staging buffer: %s\n",
				vk::to_string(BindResult).c_str());
			return EXIT_FAILURE;
		}
	}

	std::span<std::byte> StagingBufferData;
	if( auto MapResult = Device->mapMemory(
			StagingBufferMemory.get(), 0, StagingBufferInfo.size);
		MapResult.result == vk::Result::eSuccess )
	{
		StagingBufferData = std::span<std::byte>(
			reinterpret_cast<std::byte*>(MapResult.value),
			StagingBufferInfo.size);
	}
	else
	{
		std::fprintf(
			stderr, "Error mapping staging buffer memory: %s\n",
			vk::to_string(MapResult.result).c_str());
		return EXIT_FAILURE;
	}

	std::unordered_map<vk::Image, vk::BufferImageCopy> ImageUploads;

	vk::UniqueBuffer VertexBuffer;
	//// Create Vertex buffer heap
	vk::BufferCreateInfo VertexBufferInfo = {};
	// VertexBufferInfo.size  = VertexHeapIndexOffset * sizeof(Blam::Vertex);
	VertexBufferInfo.size  = 32_MiB;
	VertexBufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer
						   | vk::BufferUsageFlagBits::eTransferDst;

	if( auto CreateResult = Device->createBufferUnique(VertexBufferInfo);
		CreateResult.result == vk::Result::eSuccess )
	{
		VertexBuffer = std::move(CreateResult.value);
	}
	else
	{
		std::fprintf(
			stderr, "Error creating vertex buffer: %s\n",
			vk::to_string(CreateResult.result).c_str());
		return EXIT_FAILURE;
	}

	Vulkan::SetObjectName(
		Device.get(), VertexBuffer.get(), "Vertex Buffer( %s )",
		Common::FormatByteCount(VertexBufferInfo.size).c_str());

	vk::UniqueBuffer LightmapVertexBuffer;
	//// Create Vertex buffer heap
	vk::BufferCreateInfo LightmapVertexBufferInfo = {};
	LightmapVertexBufferInfo.size                 = 32_MiB;
	//= VertexHeapIndexOffset * sizeof(Blam::LightmapVertex);
	LightmapVertexBufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer
								   | vk::BufferUsageFlagBits::eTransferDst;

	if( auto CreateResult
		= Device->createBufferUnique(LightmapVertexBufferInfo);
		CreateResult.result == vk::Result::eSuccess )
	{
		LightmapVertexBuffer = std::move(CreateResult.value);
	}
	else
	{
		std::fprintf(
			stderr, "Error creating lightmap vertex buffer: %s\n",
			vk::to_string(CreateResult.result).c_str());
		return EXIT_FAILURE;
	}

	Vulkan::SetObjectName(
		Device.get(), LightmapVertexBuffer.get(),
		"Lightmap Vertex Buffer( %s )",
		Common::FormatByteCount(LightmapVertexBufferInfo.size).c_str());

	vk::UniqueBuffer IndexBuffer;
	//// Create Index buffer heap
	vk::BufferCreateInfo IndexBufferInfo = {};
	// IndexBufferInfo.size  = IndexHeapIndexOffset * sizeof(std::uint16_t);
	IndexBufferInfo.size  = 16_MiB;
	IndexBufferInfo.usage = vk::BufferUsageFlagBits::eIndexBuffer
						  | vk::BufferUsageFlagBits::eTransferDst;

	if( auto CreateResult = Device->createBufferUnique(IndexBufferInfo);
		CreateResult.result == vk::Result::eSuccess )
	{
		IndexBuffer = std::move(CreateResult.value);
	}
	else
	{
		std::fprintf(
			stderr, "Error creating Index buffer: %s\n",
			vk::to_string(CreateResult.result).c_str());
		return EXIT_FAILURE;
	}
	Vulkan::SetObjectName(
		Device.get(), IndexBuffer.get(), "Index Buffer( %s )",
		Common::FormatByteCount(IndexBufferInfo.size).c_str());

	// Contains _both_ the vertex buffer and the index buffer
	vk::UniqueDeviceMemory GeometryBufferHeapMemory = {};

	if( auto [Result, Value] = Vulkan::CommitBufferHeap(
			Device.get(), PhysicalDevice,
			std::array{
				VertexBuffer.get(), IndexBuffer.get(),
				LightmapVertexBuffer.get()});
		Result == vk::Result::eSuccess )
	{
		GeometryBufferHeapMemory = std::move(Value);
	}
	else
	{
		std::fprintf(
			stderr, "Error committing vertex/index memory: %s\n",
			vk::to_string(Result).c_str());
		return EXIT_FAILURE;
	}

	// Parameters used for drawing
	struct LightmapMesh
	{
		std::uint32_t VertexIndexOffset;
		std::uint32_t IndexCount;
		std::uint32_t IndexOffset;

		std::optional<std::uint32_t> BasemapTag;

		// Some lightmap meshes don't have a lightmap!
		std::optional<std::uint32_t> LightmapTag;
		std::optional<std::uint32_t> LightmapIndex;
	};

	std::vector<LightmapMesh> LightmapMeshs;

	{
		// Offset is in elements, not bytes
		std::uint32_t VertexHeapIndexOffset = 0;
		std::uint32_t IndexHeapIndexOffset  = 0;

		for( const Blam::Tag<Blam::TagClass::Scenario>::StructureBSP&
				 CurBSPEntry : CurMap.GetScenarioBSPs() )
		{
			const std::span<const std::byte> BSPData
				= CurBSPEntry.GetSBSPData(MapFile.data());
			const Blam::Tag<Blam::TagClass::ScenarioStructureBsp>& ScenarioBSP
				= CurBSPEntry.GetSBSP(MapFile.data());

			WorldBoundMin.x
				= glm::min(WorldBoundMin.x, ScenarioBSP.WorldBoundsX[0]);
			WorldBoundMin.y
				= glm::min(WorldBoundMin.y, ScenarioBSP.WorldBoundsY[0]);
			WorldBoundMin.z
				= glm::min(WorldBoundMin.z, ScenarioBSP.WorldBoundsZ[0]);

			WorldBoundMax.x
				= glm::max(WorldBoundMax.x, ScenarioBSP.WorldBoundsX[1]);
			WorldBoundMax.y
				= glm::max(WorldBoundMax.y, ScenarioBSP.WorldBoundsY[1]);
			WorldBoundMax.z
				= glm::max(WorldBoundMax.z, ScenarioBSP.WorldBoundsZ[1]);
			// Lightmap
			for( const auto& CurLightmap : ScenarioBSP.Lightmaps.GetSpan(
					 BSPData.data(), CurBSPEntry.BSPVirtualBase) )
			{
				const auto& LightmapTextureTag
					= CurMap.GetTag<Blam::TagClass::Bitmap>(
						ScenarioBSP.LightmapTexture.TagID);
				const std::int16_t LightmapTextureIndex
					= CurLightmap.LightmapIndex;

				const auto Surfaces = ScenarioBSP.Surfaces.GetSpan(
					BSPData.data(), CurBSPEntry.BSPVirtualBase);
				for( const auto& CurMaterial : CurLightmap.Materials.GetSpan(
						 BSPData.data(), CurBSPEntry.BSPVirtualBase) )
				{

					std::printf(
						"Shader(%s): %s | Permutation: %04X\n",
						Blam::FormatTagClass(CurMaterial.Shader.Class).c_str(),
						CurMap.GetTagName(CurMaterial.Shader.TagID).data(),
						CurMaterial.ShaderPermutation);

					auto& CurLightmapMesh = LightmapMeshs.emplace_back();
					//// Vertex Buffer data
					{
						// Copy vertex data into the staging buffer
						const std::span<const Blam::Vertex> CurVertexData
							= CurMaterial.GetVertices(
								BSPData.data(), CurBSPEntry.BSPVirtualBase);

						// Queue up staging buffer copy
						StreamBuffer.QueueBufferUpload(
							std::as_bytes(CurVertexData), VertexBuffer.get(),
							VertexHeapIndexOffset * sizeof(Blam::Vertex));

						// Add the offset needed to begin indexing into
						// this particular part of the vertex buffer,
						// used when drawing
						CurLightmapMesh.VertexIndexOffset
							= VertexHeapIndexOffset;

						if( CurMaterial.Shader.Class
							== Blam::TagClass::ShaderEnvironment )
						{
							auto* BasemapTag = CurMap.GetTag<
								Blam::TagClass::ShaderEnvironment>(
								CurMaterial.Shader.TagID);
							CurLightmapMesh.BasemapTag
								= BasemapTag->BaseMap.TagID;
						}

						if( ScenarioBSP.LightmapTexture.TagID != -1
							&& LightmapTextureIndex != -1 )
						{
							CurLightmapMesh.LightmapTag
								= ScenarioBSP.LightmapTexture.TagID;
							CurLightmapMesh.LightmapIndex
								= LightmapTextureIndex;
						}

						//// Lightmap vertex buffer data
						{
							const std::span<const Blam::LightmapVertex>
								CurLightmapVertexData
								= CurMaterial.GetLightmapVertices(
									BSPData.data(), CurBSPEntry.BSPVirtualBase);
							// Queue up staging buffer copy
							StreamBuffer.QueueBufferUpload(
								std::as_bytes(CurLightmapVertexData),
								LightmapVertexBuffer.get(),
								VertexHeapIndexOffset
									* sizeof(Blam::LightmapVertex));
						}

						VertexHeapIndexOffset += CurVertexData.size();
					}

					//// Index Buffer data
					{
						const std::span<const std::byte> CurIndexData
							= std::as_bytes(Surfaces.subspan(
								CurMaterial.SurfacesIndexStart,
								CurMaterial.SurfacesCount));

						CurLightmapMesh.IndexCount
							= CurMaterial.SurfacesCount * 3;

						CurLightmapMesh.IndexOffset = IndexHeapIndexOffset;

						// Queue up staging buffer copy
						StreamBuffer.QueueBufferUpload(
							std::as_bytes(CurIndexData), IndexBuffer.get(),
							IndexHeapIndexOffset * sizeof(std::uint16_t));

						// Increment offsets
						IndexHeapIndexOffset += CurMaterial.SurfacesCount * 3;
					}
				}
			}
		}

		Vulkan::SetObjectName(
			Device.get(), GeometryBufferHeapMemory.get(),
			"Geometry Buffer Memory");
	}

	// Render Target images
	vk::UniqueImage RenderImage;

	vk::UniqueImage RenderImageAA;

	vk::UniqueImage RenderImageDepth;

	// Render-image, R8G8B8A8_SRGB
	vk::ImageCreateInfo RenderImageInfo = {};
	RenderImageInfo.imageType           = vk::ImageType::e2D;
	RenderImageInfo.format              = vk::Format::eR8G8B8A8Srgb;
	RenderImageInfo.extent      = vk::Extent3D(RenderSize.x, RenderSize.y, 1);
	RenderImageInfo.mipLevels   = 1;
	RenderImageInfo.arrayLayers = 1;
	RenderImageInfo.samples     = vk::SampleCountFlagBits::e1;
	RenderImageInfo.tiling      = vk::ImageTiling::eOptimal;
	RenderImageInfo.usage       = vk::ImageUsageFlagBits::eColorAttachment
						  | vk::ImageUsageFlagBits::eTransferSrc;
	RenderImageInfo.sharingMode   = vk::SharingMode::eExclusive;
	RenderImageInfo.initialLayout = vk::ImageLayout::eUndefined;

	// Render-image(MSAA), R8G8B8A8_SRGB
	vk::ImageCreateInfo RenderImageAAInfo = {};
	RenderImageAAInfo.imageType           = vk::ImageType::e2D;
	RenderImageAAInfo.format              = vk::Format::eR8G8B8A8Srgb;
	RenderImageAAInfo.samples             = RenderSamples;
	RenderImageAAInfo.extent      = vk::Extent3D(RenderSize.x, RenderSize.y, 1);
	RenderImageAAInfo.mipLevels   = 1;
	RenderImageAAInfo.arrayLayers = 1;
	RenderImageAAInfo.tiling      = vk::ImageTiling::eOptimal;
	RenderImageAAInfo.usage       = vk::ImageUsageFlagBits::eColorAttachment;
	RenderImageAAInfo.sharingMode = vk::SharingMode::eExclusive;
	RenderImageAAInfo.initialLayout = vk::ImageLayout::eUndefined;

	// Render-image-depth(MSAA), D32_sfloat
	vk::ImageCreateInfo RenderImageDepthInfo = {};
	RenderImageDepthInfo.imageType           = vk::ImageType::e2D;
	RenderImageDepthInfo.format              = vk::Format::eD32Sfloat;
	RenderImageDepthInfo.samples             = RenderSamples;
	RenderImageDepthInfo.extent = vk::Extent3D(RenderSize.x, RenderSize.y, 1);
	RenderImageDepthInfo.mipLevels   = 1;
	RenderImageDepthInfo.arrayLayers = 1;
	RenderImageDepthInfo.tiling      = vk::ImageTiling::eOptimal;
	RenderImageDepthInfo.usage
		= vk::ImageUsageFlagBits::eDepthStencilAttachment;
	RenderImageDepthInfo.sharingMode   = vk::SharingMode::eExclusive;
	RenderImageDepthInfo.initialLayout = vk::ImageLayout::eUndefined;

	if( auto CreateResult = Device->createImageUnique(RenderImageInfo);
		CreateResult.result == vk::Result::eSuccess )
	{
		RenderImage = std::move(CreateResult.value);
	}
	else
	{
		std::fprintf(
			stderr, "Error creating render target: %s\n",
			vk::to_string(CreateResult.result).c_str());
		return EXIT_FAILURE;
	}
	Vulkan::SetObjectName(
		Device.get(), RenderImage.get(), "Render Image Resolve");

	if( auto CreateResult = Device->createImageUnique(RenderImageAAInfo);
		CreateResult.result == vk::Result::eSuccess )
	{
		RenderImageAA = std::move(CreateResult.value);
	}
	else
	{
		std::fprintf(
			stderr, "Error creating render target: %s\n",
			vk::to_string(CreateResult.result).c_str());
		return EXIT_FAILURE;
	}
	Vulkan::SetObjectName(
		Device.get(), RenderImageAA.get(), "Render Image(AA)");

	if( auto CreateResult = Device->createImageUnique(RenderImageDepthInfo);
		CreateResult.result == vk::Result::eSuccess )
	{
		RenderImageDepth = std::move(CreateResult.value);
	}
	else
	{
		std::fprintf(
			stderr, "Error creating render target: %s\n",
			vk::to_string(CreateResult.result).c_str());
		return EXIT_FAILURE;
	}
	Vulkan::SetObjectName(
		Device.get(), RenderImageDepth.get(), "Render Image Depth(AA)");

	std::vector<vk::Image> ImageHeapTargets = {};
	ImageHeapTargets.push_back(RenderImage.get());
	ImageHeapTargets.push_back(RenderImageAA.get());
	ImageHeapTargets.push_back(RenderImageDepth.get());

	// Allocate all the memory we need for these images up-front into a single
	// heap.
	vk::UniqueDeviceMemory ImageHeapMemory = {};

	if( auto [Result, Value] = Vulkan::CommitImageHeap(
			Device.get(), PhysicalDevice, ImageHeapTargets);
		Result == vk::Result::eSuccess )
	{
		ImageHeapMemory = std::move(Value);
	}
	else
	{
		std::fprintf(
			stderr, "Error committing image memory: %s\n",
			vk::to_string(Result).c_str());
		return EXIT_FAILURE;
	}

	//// Image Views
	// Create the image views for the render-targets
	vk::ImageViewCreateInfo ImageViewInfoTemplate = {};
	ImageViewInfoTemplate.viewType                = vk::ImageViewType::e2D;
	ImageViewInfoTemplate.components.r            = vk::ComponentSwizzle::eR;
	ImageViewInfoTemplate.components.g            = vk::ComponentSwizzle::eG;
	ImageViewInfoTemplate.components.b            = vk::ComponentSwizzle::eB;
	ImageViewInfoTemplate.components.a            = vk::ComponentSwizzle::eA;
	ImageViewInfoTemplate.subresourceRange.aspectMask
		= vk::ImageAspectFlagBits::eColor;
	ImageViewInfoTemplate.subresourceRange.baseMipLevel   = 0;
	ImageViewInfoTemplate.subresourceRange.levelCount     = 1;
	ImageViewInfoTemplate.subresourceRange.baseArrayLayer = 0;
	ImageViewInfoTemplate.subresourceRange.layerCount     = 1;

	ImageViewInfoTemplate.image  = RenderImage.get();
	ImageViewInfoTemplate.format = RenderImageInfo.format;

	vk::UniqueImageView RenderImageView = {};
	if( auto CreateResult
		= Device->createImageViewUnique(ImageViewInfoTemplate);
		CreateResult.result == vk::Result::eSuccess )
	{
		RenderImageView = std::move(CreateResult.value);
	}
	else
	{
		std::fprintf(
			stderr, "Error creating render target view: %s\n",
			vk::to_string(CreateResult.result).c_str());
		return EXIT_FAILURE;
	}

	ImageViewInfoTemplate.image           = RenderImageAA.get();
	ImageViewInfoTemplate.format          = RenderImageAAInfo.format;
	vk::UniqueImageView RenderImageAAView = {};
	if( auto CreateResult
		= Device->createImageViewUnique(ImageViewInfoTemplate);
		CreateResult.result == vk::Result::eSuccess )
	{
		RenderImageAAView = std::move(CreateResult.value);
	}
	else
	{
		std::fprintf(
			stderr, "Error creating render target view: %s\n",
			vk::to_string(CreateResult.result).c_str());
		return EXIT_FAILURE;
	}

	ImageViewInfoTemplate.image  = RenderImageDepth.get();
	ImageViewInfoTemplate.format = RenderImageDepthInfo.format;
	ImageViewInfoTemplate.subresourceRange.aspectMask
		= vk::ImageAspectFlagBits::eDepth;
	vk::UniqueImageView RenderImageDepthView = {};
	if( auto CreateResult
		= Device->createImageViewUnique(ImageViewInfoTemplate);
		CreateResult.result == vk::Result::eSuccess )
	{
		RenderImageDepthView = std::move(CreateResult.value);
	}
	else
	{
		std::fprintf(
			stderr, "Error creating render target view: %s\n",
			vk::to_string(CreateResult.result).c_str());
		return EXIT_FAILURE;
	}

	//// MainFrameBuffer
	vk::UniqueFramebuffer RenderFramebuffer = CreateMainFrameBuffer(
		Device.get(), RenderImageView.get(), RenderImageDepthView.get(),
		RenderImageAAView.get(), RenderSize, MainRenderPass.get());

	// Main Shader modules
	const cmrc::file DefaultVertShaderData
		= DataFS.open("shaders/Default.vert.spv");
	const cmrc::file DefaultFragShaderData
		= DataFS.open("shaders/Default.frag.spv");
	const cmrc::file UnlitFragShaderData
		= DataFS.open("shaders/Unlit.frag.spv");

	vk::UniqueShaderModule DefaultVertexShaderModule
		= Vulkan::CreateShaderModule(
			Device.get(),
			std::span<const std::uint32_t>(
				reinterpret_cast<const std::uint32_t*>(
					DefaultVertShaderData.begin()),
				DefaultVertShaderData.size() / sizeof(std::uint32_t)));
	vk::UniqueShaderModule DefaultFragmentShaderModule
		= Vulkan::CreateShaderModule(
			Device.get(),
			std::span<const std::uint32_t>(
				reinterpret_cast<const std::uint32_t*>(
					DefaultFragShaderData.begin()),
				DefaultFragShaderData.size() / sizeof(std::uint32_t)));
	vk::UniqueShaderModule UnlitFragmentShaderModule
		= Vulkan::CreateShaderModule(
			Device.get(),
			std::span<const std::uint32_t>(
				reinterpret_cast<const std::uint32_t*>(
					UnlitFragShaderData.begin()),
				UnlitFragShaderData.size() / sizeof(std::uint32_t)));

	Vulkan::DescriptorHeap DebugDrawDescriptorPool
		= Vulkan::DescriptorHeap::Create(
			  Device.get(), {{vk::DescriptorSetLayoutBinding(
								0, vk::DescriptorType::eCombinedImageSampler, 1,
								vk::ShaderStageFlagBits::eFragment)}})
			  .value();

	auto [DebugDrawPipeline, DebugDrawPipelineLayout] = CreateGraphicsPipeline(
		Device.get(),
		{{vk::PushConstantRange(
			vk::ShaderStageFlagBits::eAllGraphics, 0,
			sizeof(VkBlam::CameraGlobals))}},
		{{DebugDrawDescriptorPool.GetDescriptorSetLayout(),
		  DebugDrawDescriptorPool.GetDescriptorSetLayout()}},
		DefaultVertexShaderModule.get(), DefaultFragmentShaderModule.get(),
		MainRenderPass.get());

	Vulkan::DescriptorHeap UnlitDescriptorPool
		= Vulkan::DescriptorHeap::Create(
			  Device.get(), {{vk::DescriptorSetLayoutBinding(
								0, vk::DescriptorType::eCombinedImageSampler, 1,
								vk::ShaderStageFlagBits::eFragment)}})
			  .value();

	auto [UnlitDrawPipeline, UnlitDrawPipelineLayout] = CreateGraphicsPipeline(
		Device.get(),
		{{vk::PushConstantRange(
			vk::ShaderStageFlagBits::eAllGraphics, 0,
			sizeof(VkBlam::CameraGlobals))}},
		{{UnlitDescriptorPool.GetDescriptorSetLayout(),
		  UnlitDescriptorPool.GetDescriptorSetLayout()}},
		DefaultVertexShaderModule.get(), UnlitFragmentShaderModule.get(),
		MainRenderPass.get(), vk::PolygonMode::eLine);

	//////// Stream in all images

	// TagID -> BitmapTag[indexofimage] -> vulkan image
	VkBlam::BitmapHeapT BitmapHeap;

	// Queue up image uploads
	CurMap.VisitTagClass<Blam::TagClass::Bitmap>(
		[&](const Blam::TagIndexEntry&               TagEntry,
			const Blam::Tag<Blam::TagClass::Bitmap>& Bitmap) -> void {
			// std::printf("%s\n", CurMap.GetTagName(TagEntry.TagID).data());
			for( std::size_t CurSubTextureIdx = 0;
				 CurSubTextureIdx < Bitmap.Bitmaps.Count; ++CurSubTextureIdx )
			{
				const auto& CurSubTexture = Bitmap.Bitmaps.GetSpan(
					MapFile.data(),
					CurMap.TagHeapVirtualBase)[CurSubTextureIdx];
				const auto PixelData = std::span<const std::byte>(
					reinterpret_cast<const std::byte*>(BitmapFile.data())
						+ CurSubTexture.PixelDataOffset,
					CurSubTexture.PixelDataSize);

				vk::ImageCreateInfo ImageInfo = {};
				ImageInfo.imageType = VkBlam::BlamToVk(CurSubTexture.Type);
				ImageInfo.format    = VkBlam::BlamToVk(CurSubTexture.Format);
				ImageInfo.extent    = vk::Extent3D(
					   CurSubTexture.Width, CurSubTexture.Height,
					   CurSubTexture.Depth);
				ImageInfo.mipLevels
					= std::max<std::uint16_t>(CurSubTexture.MipmapCount, 1);
				ImageInfo.arrayLayers
					= CurSubTexture.Type == Blam::BitmapEntryType::CubeMap ? 6
																		   : 1;
				ImageInfo.samples = vk::SampleCountFlagBits::e1;
				ImageInfo.tiling  = vk::ImageTiling::eOptimal;
				ImageInfo.usage   = vk::ImageUsageFlagBits::eSampled
								| vk::ImageUsageFlagBits::eTransferDst
								| vk::ImageUsageFlagBits::eTransferSrc;
				ImageInfo.sharingMode   = vk::SharingMode::eExclusive;
				ImageInfo.initialLayout = vk::ImageLayout::eUndefined;

				if( CurSubTexture.Type == Blam::BitmapEntryType::CubeMap )
				{
					ImageInfo.flags = vk::ImageCreateFlagBits::eCubeCompatible;
				}

				auto& ImageDest
					= BitmapHeap.Images[TagEntry.TagID][CurSubTextureIdx];

				if( auto CreateResult = Device->createImageUnique(ImageInfo);
					CreateResult.result == vk::Result::eSuccess )
				{
					ImageDest = std::move(CreateResult.value);
				}
				else
				{
					std::fprintf(
						stderr, "Error creating image: %s\n",
						vk::to_string(CreateResult.result).c_str());
					return;
				}
				Vulkan::SetObjectName(
					Device.get(), ImageDest.get(), "Bitmap %08X[%2zu] | %s",
					TagEntry.TagID, CurSubTextureIdx,
					CurMap.GetTagName(TagEntry.TagID).data());
			}
		});

	// Allocate and bind memory for all bitmaps
	vk::UniqueDeviceMemory BitmapHeapMemory = {};
	{
		std::vector<vk::Image> Bitmaps;
		for( const auto& CurBitmap : BitmapHeap.Images )
		{
			for( const auto& CurSubBitmap : CurBitmap.second )
			{
				Bitmaps.emplace_back(CurSubBitmap.second.get());
			}
		}
		if( auto [Result, Value]
			= Vulkan::CommitImageHeap(Device.get(), PhysicalDevice, Bitmaps);
			Result == vk::Result::eSuccess )
		{
			BitmapHeapMemory = std::move(Value);
		}
		else
		{
			std::fprintf(
				stderr, "Error committing bitmap memory: %s\n",
				vk::to_string(Result).c_str());
			return EXIT_FAILURE;
		}
	}

	// All images are now created and binded to memory
	Vulkan::DescriptorUpdateBatch DescriptorUpdateBatch
		= Vulkan::DescriptorUpdateBatch::Create(Device.get()).value();
	{

		CurMap.VisitTagClass<Blam::TagClass::Bitmap>(
			[&](const Blam::TagIndexEntry&               TagEntry,
				const Blam::Tag<Blam::TagClass::Bitmap>& Bitmap) -> void {
				for( std::size_t CurSubTextureIdx = 0;
					 CurSubTextureIdx < Bitmap.Bitmaps.Count;
					 ++CurSubTextureIdx )
				{
					const auto& CurSubTexture = Bitmap.Bitmaps.GetSpan(
						MapFile.data(),
						CurMap.TagHeapVirtualBase)[CurSubTextureIdx];
					const auto PixelData = std::span<const std::byte>(
						reinterpret_cast<const std::byte*>(BitmapFile.data())
							+ CurSubTexture.PixelDataOffset,
						CurSubTexture.PixelDataSize);

					auto& ImageDest
						= BitmapHeap.Images[TagEntry.TagID][CurSubTextureIdx];

					const std::size_t MipCount
						= std::max<std::uint16_t>(CurSubTexture.MipmapCount, 1);
					const std::size_t LayerCount
						= CurSubTexture.Type == Blam::BitmapEntryType::CubeMap
							? 6
							: 1;

					// Upload image data
					const std::size_t BlockSize
						= vk::blockSize(VkBlam::BlamToVk(CurSubTexture.Format));
					const std::array<std::uint8_t, 3> BlockExtent
						= vk::blockExtent(
							VkBlam::BlamToVk(CurSubTexture.Format));

					std::size_t PixelDataOff = 0;

					auto CurExtent = vk::Extent3D(
						CurSubTexture.Width, CurSubTexture.Height,
						CurSubTexture.Depth);
					for( std::size_t CurMip = 0; CurMip < MipCount; ++CurMip )
					{
						for( std::size_t CurLayer = 0; CurLayer < LayerCount;
							 ++CurLayer )
						{
							const std::array<std::uint32_t, 3> CurBlockCount = {
								std::max(1u, CurExtent.width / BlockExtent[0]),
								std::max(1u, CurExtent.height / BlockExtent[1]),
								std::max(1u, CurExtent.depth / BlockExtent[2])};

							const std::size_t CurPixelDataSize
								= CurBlockCount[0] * CurBlockCount[1]
								* CurBlockCount[2] * BlockSize;

							StreamBuffer.QueueImageUpload(
								PixelData.subspan(
									PixelDataOff, CurPixelDataSize),
								ImageDest.get(), vk::Offset3D(0, 0, 0),
								CurExtent,
								vk::ImageSubresourceLayers(
									vk::ImageAspectFlagBits::eColor, CurMip,
									CurLayer, 1));

							PixelDataOff += CurPixelDataSize;
						}

						CurExtent.width  = std::max(1u, CurExtent.width / 2);
						CurExtent.height = std::max(1u, CurExtent.height / 2);
						CurExtent.depth  = std::max(1u, CurExtent.depth / 2);
					}

					// Create image view
					vk::ImageViewCreateInfo BitmapImageViewInfo = {};
					BitmapImageViewInfo.image = ImageDest.get();
					switch( CurSubTexture.Type )
					{
					default:
					case Blam::BitmapEntryType::Texture2D:
					{
						BitmapImageViewInfo.viewType = vk::ImageViewType::e2D;
						break;
					}
					case Blam::BitmapEntryType::Texture3D:
					{
						BitmapImageViewInfo.viewType = vk::ImageViewType::e3D;
						break;
					}
					case Blam::BitmapEntryType::CubeMap:
					{
						BitmapImageViewInfo.viewType = vk::ImageViewType::eCube;
						break;
					}
					}
					BitmapImageViewInfo.format
						= VkBlam::BlamToVk(CurSubTexture.Format);
					;
					BitmapImageViewInfo.components.r = vk::ComponentSwizzle::eR;
					BitmapImageViewInfo.components.g = vk::ComponentSwizzle::eG;
					BitmapImageViewInfo.components.b = vk::ComponentSwizzle::eB;
					BitmapImageViewInfo.components.a = vk::ComponentSwizzle::eA;
					BitmapImageViewInfo.subresourceRange.aspectMask
						= vk::ImageAspectFlagBits::eColor;
					BitmapImageViewInfo.subresourceRange.baseMipLevel = 0;
					BitmapImageViewInfo.subresourceRange.levelCount = MipCount;
					BitmapImageViewInfo.subresourceRange.baseArrayLayer = 0;
					BitmapImageViewInfo.subresourceRange.layerCount
						= LayerCount;

					auto& BitmapImageView
						= BitmapHeap.Views[TagEntry.TagID][CurSubTextureIdx];

					if( auto CreateResult
						= Device->createImageViewUnique(BitmapImageViewInfo);
						CreateResult.result == vk::Result::eSuccess )
					{
						BitmapImageView = std::move(CreateResult.value);
					}
					else
					{
						std::fprintf(
							stderr, "Error bitmap view: %s\n",
							vk::to_string(CreateResult.result).c_str());
						return;
					}

					Vulkan::SetObjectName(
						Device.get(), BitmapImageView.get(),
						"Bitmap View %08X[%2zu] | %s", TagEntry.TagID,
						CurSubTextureIdx,
						CurMap.GetTagName(TagEntry.TagID).data());

					// Create descriptor set
					vk::DescriptorSet& TargetSet
						= BitmapHeap.Sets[TagEntry.TagID][CurSubTextureIdx];

					if( auto NewSet
						= DebugDrawDescriptorPool.AllocateDescriptorSet();
						NewSet )
					{
						TargetSet = NewSet.value();
					}
					else
					{
						std::fprintf(
							stderr, "Error allocating bitmap descriptor set\n");
						return;
					}

					Vulkan::SetObjectName(
						Device.get(), TargetSet,
						"Bitmap Descriptor Set %08X[%2zu] | %s", TagEntry.TagID,
						CurSubTextureIdx,
						CurMap.GetTagName(TagEntry.TagID).data());

					DescriptorUpdateBatch.AddImageSampler(
						TargetSet, 0, BitmapImageView.get(),
						DefaultSampler.get(),
						vk::ImageLayout::eShaderReadOnlyOptimal);
				}
			});
	}

	// const auto CheckBitmapRef
	// 	= [&CurMap, &(BitmapHeap.Images)](
	// 		  std::uint32_t TagID, const char* Name) -> void {
	// 	if( BitmapHeap.Images.contains(TagID) )
	// 	{
	// 		std::printf("\t-%s: %s\n", Name, CurMap.GetTagName(TagID).data());
	// 	}
	// 	else
	// 	{
	// 		std::printf("\t-%s: \e[31m%08X\e[0m\n", Name, TagID);
	// 	}
	// };

	VkBlam::ShaderEnvironment ShaderEnvironments(
		Device.get(), BitmapHeap, DescriptorUpdateBatch);

	CurMap.VisitTagClass<Blam::TagClass::ShaderEnvironment>(
		[&](const Blam::TagIndexEntry& TagEntry,
			const Blam::Tag<Blam::TagClass::ShaderEnvironment>&
				ShaderEnvironment) -> void {
			ShaderEnvironments.RegisterShader(TagEntry, ShaderEnvironment);
		});

	DescriptorUpdateBatch.Flush();

	//// Create Command Pool
	vk::CommandPoolCreateInfo CommandPoolInfo = {};
	CommandPoolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
	CommandPoolInfo.queueFamilyIndex = 0;

	vk::UniqueCommandPool CommandPool = {};
	if( auto CreateResult = Device->createCommandPoolUnique(CommandPoolInfo);
		CreateResult.result == vk::Result::eSuccess )
	{
		CommandPool = std::move(CreateResult.value);
	}
	else
	{
		std::fprintf(
			stderr, "Error creating command pool: %s\n",
			vk::to_string(CreateResult.result).c_str());
		return EXIT_FAILURE;
	}

	//// Create Command Buffer
	vk::CommandBufferAllocateInfo CommandBufferInfo = {};
	CommandBufferInfo.commandPool                   = CommandPool.get();
	CommandBufferInfo.level              = vk::CommandBufferLevel::ePrimary;
	CommandBufferInfo.commandBufferCount = 1;

	vk::UniqueCommandBuffer CommandBuffer = {};

	if( auto AllocateResult
		= Device->allocateCommandBuffersUnique(CommandBufferInfo);
		AllocateResult.result == vk::Result::eSuccess )
	{
		CommandBuffer = std::move(AllocateResult.value[0]);
	}
	else
	{
		std::fprintf(
			stderr, "Error allocating command buffer: %s\n",
			vk::to_string(AllocateResult.result).c_str());
		return EXIT_FAILURE;
	}

	if( auto BeginResult = CommandBuffer->begin(vk::CommandBufferBeginInfo{});
		BeginResult != vk::Result::eSuccess )
	{
		std::fprintf(
			stderr, "Error beginning command buffer: %s\n",
			vk::to_string(BeginResult).c_str());
		return EXIT_FAILURE;
	}

	{
		Vulkan::DebugLabelScope DebugCopyScope(
			CommandBuffer.get(), {1.0, 0.0, 1.0, 1.0}, "Frame");

		{
			Vulkan::DebugLabelScope DebugCopyScope(
				CommandBuffer.get(), {1.0, 1.0, 0.0, 1.0}, "Main Render Pass");

			vk::RenderPassBeginInfo RenderBeginInfo   = {};
			RenderBeginInfo.renderPass                = MainRenderPass.get();
			static const vk::ClearValue ClearColors[] = {
				vk::ClearColorValue(
					std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f}),
				vk::ClearDepthStencilValue(1.0f, 0),
				vk::ClearColorValue(
					std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f}),
			};
			RenderBeginInfo.pClearValues             = ClearColors;
			RenderBeginInfo.clearValueCount          = std::size(ClearColors);
			RenderBeginInfo.renderArea.extent.width  = RenderSize.x;
			RenderBeginInfo.renderArea.extent.height = RenderSize.y;
			RenderBeginInfo.framebuffer              = RenderFramebuffer.get();
			CommandBuffer->beginRenderPass(
				RenderBeginInfo, vk::SubpassContents::eInline);

			vk::Viewport Viewport = {};
			Viewport.width        = RenderSize.x;
			Viewport.height       = -float(RenderSize.y);
			Viewport.x            = 0.0f;
			Viewport.y            = RenderSize.y;
			Viewport.minDepth     = 0.0f;
			Viewport.maxDepth     = 1.0f;
			CommandBuffer->setViewport(0, {Viewport});
			// Scissor
			vk::Rect2D Scissor    = {};
			Scissor.extent.width  = RenderSize.x;
			Scissor.extent.height = RenderSize.y;
			CommandBuffer->setScissor(0, {Scissor});
			// Draw
			CommandBuffer->bindPipeline(
				vk::PipelineBindPoint::eGraphics, DebugDrawPipeline.get());

			VkBlam::CameraGlobals CameraGlobals = {};

			const glm::vec3 WorldCenter
				= glm::mix(WorldBoundMin, WorldBoundMax, 0.5);

			const glm::f32 MaxExtent
				= glm::compMax(glm::xyz(WorldBoundMax - WorldBoundMin)) / 2.0f;

			CameraGlobals.View = glm::lookAt<glm::f32>(
				glm::vec3(WorldBoundMax.x, WorldBoundMax.y, MaxExtent) * 1.5f,
				// glm::vec3(WorldCenter.x, WorldCenter.y, WorldBoundMax.z),
				glm::vec3(WorldCenter.x, WorldCenter.y, WorldBoundMin.z),
				glm::vec3(0, 0, 1));

			CameraGlobals.Projection
				// = glm::ortho<glm::f32>(
				// 	-MaxExtent, MaxExtent, -MaxExtent, MaxExtent, 0.0f,
				// 	WorldBoundMax.z - WorldBoundMin.z);
				= glm::perspective<glm::f32>(
					glm::radians(72.0f),
					static_cast<float>(RenderSize.x) / RenderSize.y, 1.0f,
					1000.0f);
			// CameraGlobals.Projection[1][1] *= -1.0f;

			CameraGlobals.ViewProjection
				= CameraGlobals.Projection * CameraGlobals.View;

			CommandBuffer->pushConstants<VkBlam::CameraGlobals>(
				DebugDrawPipelineLayout.get(),
				vk::ShaderStageFlagBits::eAllGraphics, 0, {CameraGlobals});

			CommandBuffer->bindVertexBuffers(
				0, {VertexBuffer.get(), LightmapVertexBuffer.get()}, {0, 0});
			CommandBuffer->bindIndexBuffer(
				IndexBuffer.get(), 0, vk::IndexType::eUint16);

			for( std::size_t i = 0; i < LightmapMeshs.size(); ++i )
			{
				const auto& CurLightmapMesh = LightmapMeshs[i];
				Vulkan::InsertDebugLabel(
					CommandBuffer.get(), {0.5, 0.5, 0.5, 1.0}, "BSP Draw: %zu",
					i);
				if( CurLightmapMesh.LightmapTag.has_value()
					&& CurLightmapMesh.LightmapIndex.has_value() )
				{
					CommandBuffer->bindDescriptorSets(
						vk::PipelineBindPoint::eGraphics,
						DebugDrawPipelineLayout.get(), 0,
						{BitmapHeap.Sets.at(CurLightmapMesh.LightmapTag.value())
							 .at(CurLightmapMesh.LightmapIndex.value())},
						{});
				}
				if( CurLightmapMesh.BasemapTag.has_value() )
				{
					CommandBuffer->bindDescriptorSets(
						vk::PipelineBindPoint::eGraphics,
						DebugDrawPipelineLayout.get(), 1,
						{BitmapHeap.Sets.at(CurLightmapMesh.BasemapTag.value())
							 .at(0)},
						{});
				}
				CommandBuffer->drawIndexed(
					CurLightmapMesh.IndexCount, 1, CurLightmapMesh.IndexOffset,
					CurLightmapMesh.VertexIndexOffset, 0);
			}

			// CommandBuffer->bindPipeline(
			// 	vk::PipelineBindPoint::eGraphics, UnlitDrawPipeline.get());
			// CommandBuffer->pushConstants<glm::vec4>(
			// 	UnlitDrawPipelineLayout.get(),
			// 	vk::ShaderStageFlagBits::eAllGraphics, sizeof(glm::f32mat4),
			// 	{glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)});
			// for( std::size_t i = 0; i < VertexIndexOffsets.size(); ++i )
			// {
			// 	Vulkan::InsertDebugLabel(
			// 		CommandBuffer.get(), {0.5, 0.5, 0.5, 1.0}, "BSP Draw: %zu",
			// 		i);
			// 	CommandBuffer->drawIndexed(
			// 		IndexCounts[i], 1, IndexOffsets[i], VertexIndexOffsets[i],
			// 		0);
			// }

			CommandBuffer->endRenderPass();
		}

		// Wait for image data to be ready
		{
			Vulkan::DebugLabelScope DebugCopyScope(
				CommandBuffer.get(), {1.0, 1.0, 0.0, 1.0},
				"Upload framebuffer to staging buffer");
			CommandBuffer->pipelineBarrier(
				vk::PipelineStageFlagBits::eColorAttachmentOutput,
				vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlags(), {},
				{},
				{// Source Image
				 vk::ImageMemoryBarrier(
					 vk::AccessFlagBits::eColorAttachmentWrite,
					 vk::AccessFlagBits::eTransferRead,
					 vk::ImageLayout::eTransferSrcOptimal,
					 vk::ImageLayout::eTransferSrcOptimal,
					 VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
					 RenderImage.get(),
					 vk::ImageSubresourceRange(
						 vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1))});
			CommandBuffer->copyImageToBuffer(
				RenderImage.get(), vk::ImageLayout::eTransferSrcOptimal,
				StagingBuffer.get(),
				{vk::BufferImageCopy(
					0, RenderSize.x, RenderSize.y,
					vk::ImageSubresourceLayers(
						vk::ImageAspectFlagBits::eColor, 0, 0, 1),
					vk::Offset3D(),
					vk::Extent3D(RenderSize.x, RenderSize.y, 1))});
		}
	}

	if( auto EndResult = CommandBuffer->end();
		EndResult != vk::Result::eSuccess )
	{
		std::fprintf(
			stderr, "Error ending command buffer: %s\n",
			vk::to_string(EndResult).c_str());
		return EXIT_FAILURE;
	}

	const std::uint64_t UploadTick = StreamBuffer.Flush();

	// Submit work
	vk::UniqueFence Fence = {};
	if( auto CreateResult = Device->createFenceUnique({});
		CreateResult.result == vk::Result::eSuccess )
	{
		Fence = std::move(CreateResult.value);
	}
	else
	{
		std::fprintf(
			stderr, "Error creating fence: %s\n",
			vk::to_string(CreateResult.result).c_str());
		return EXIT_FAILURE;
	}

	vk::StructureChain<vk::SubmitInfo, vk::TimelineSemaphoreSubmitInfo>
		SubmitInfoChain;

	auto& SubmitInfo = SubmitInfoChain.get<vk::SubmitInfo>();

	SubmitInfo.commandBufferCount = 1;
	SubmitInfo.pCommandBuffers    = &CommandBuffer.get();

	SubmitInfo.waitSemaphoreCount = 1;
	SubmitInfo.pWaitSemaphores    = &StreamBuffer.GetSemaphore();

	static const vk::PipelineStageFlags WaitStage
		= vk::PipelineStageFlagBits::eTransfer;
	SubmitInfo.pWaitDstStageMask = &WaitStage;

	auto& SubmitTimelineInfo
		= SubmitInfoChain.get<vk::TimelineSemaphoreSubmitInfo>();

	SubmitTimelineInfo.waitSemaphoreValueCount = 1;
	SubmitTimelineInfo.pWaitSemaphoreValues    = &UploadTick;

	if( auto SubmitResult = RenderQueue.submit(SubmitInfo, Fence.get());
		SubmitResult != vk::Result::eSuccess )
	{
		std::fprintf(
			stderr, "Error submitting command buffer: %s\n",
			vk::to_string(SubmitResult).c_str());
		return EXIT_FAILURE;
	}

	// Wait for it
	if( auto WaitResult = Device->waitForFences(Fence.get(), true, ~0ULL);
		WaitResult != vk::Result::eSuccess )
	{
		std::fprintf(
			stderr, "Error waiting for fence: %s\n",
			vk::to_string(WaitResult).c_str());
		return EXIT_FAILURE;
	}

#ifdef CAPTURE
	if( rdoc_api )
		rdoc_api->EndFrameCapture(
			*(void**)(VkInstance)(*Instance.operator->()), NULL);
#endif

	stbi_write_png_compression_level = 0;
	stbi_write_png(
		("./" + CurPath.stem().string() + ".png").c_str(), RenderSize.x,
		RenderSize.y, 4, StagingBufferData.data(), 0);

	return EXIT_SUCCESS;
}

vk::UniqueRenderPass
	CreateMainRenderPass(vk::Device Device, vk::SampleCountFlagBits SampleCount)
{
	vk::RenderPassCreateInfo RenderPassInfo = {};

	const vk::AttachmentDescription Attachments[] = {
		// Color Attachment
		// We just care about it storing its color data
		vk::AttachmentDescription(
			vk::AttachmentDescriptionFlags(), vk::Format::eR8G8B8A8Srgb,
			vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined,
			vk::ImageLayout::eTransferSrcOptimal),
		// Depth Attachment
		// Dont care about reading or storing it
		vk::AttachmentDescription(
			vk::AttachmentDescriptionFlags(), vk::Format::eD32Sfloat,
			SampleCount, vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eDontCare, vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined,
			vk::ImageLayout::eDepthStencilAttachmentOptimal),
		// Color Attachment(MSAA)
		// We just care about it storing its color data
		vk::AttachmentDescription(
			vk::AttachmentDescriptionFlags(), vk::Format::eR8G8B8A8Srgb,
			SampleCount, vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eDontCare, vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined,
			vk::ImageLayout::eColorAttachmentOptimal)};

	const vk::AttachmentReference AttachmentRefs[] = {
		vk::AttachmentReference(0, vk::ImageLayout::eColorAttachmentOptimal),
		vk::AttachmentReference(
			1, vk::ImageLayout::eDepthStencilAttachmentOptimal),
		vk::AttachmentReference(2, vk::ImageLayout::eColorAttachmentOptimal),
	};

	RenderPassInfo.attachmentCount = std::size(Attachments);
	RenderPassInfo.pAttachments    = Attachments;

	vk::SubpassDescription Subpasses[1] = {{}};

	// First subpass
	Subpasses[0].colorAttachmentCount    = 1;
	Subpasses[0].pColorAttachments       = &AttachmentRefs[2];
	Subpasses[0].pDepthStencilAttachment = &AttachmentRefs[1];
	Subpasses[0].pResolveAttachments     = &AttachmentRefs[0];

	RenderPassInfo.subpassCount = std::size(Subpasses);
	RenderPassInfo.pSubpasses   = Subpasses;

	const vk::SubpassDependency SubpassDependencies[] = {vk::SubpassDependency(
		VK_SUBPASS_EXTERNAL, 0, vk::PipelineStageFlagBits::eTransfer,
		vk::PipelineStageFlagBits::eVertexInput,
		vk::AccessFlagBits::eTransferWrite,
		vk::AccessFlagBits::eVertexAttributeRead, vk::DependencyFlags())};

	RenderPassInfo.dependencyCount = std::size(SubpassDependencies);
	RenderPassInfo.pDependencies   = SubpassDependencies;

	if( auto CreateResult = Device.createRenderPassUnique(RenderPassInfo);
		CreateResult.result == vk::Result::eSuccess )
	{
		return std::move(CreateResult.value);
	}
	else
	{
		std::fprintf(
			stderr, "Error creating render pass: %s\n",
			vk::to_string(CreateResult.result).c_str());
		return {};
	}
}

vk::UniqueFramebuffer CreateMainFrameBuffer(
	vk::Device Device, vk::ImageView Color, vk::ImageView DepthAA,
	vk::ImageView ColorAA, glm::uvec2 ImageSize, vk::RenderPass RenderPass)
{
	vk::FramebufferCreateInfo FramebufferInfo = {};

	FramebufferInfo.width      = ImageSize.x;
	FramebufferInfo.height     = ImageSize.y;
	FramebufferInfo.layers     = 1;
	FramebufferInfo.renderPass = RenderPass;

	const vk::ImageView Attachments[] = {Color, DepthAA, ColorAA};
	FramebufferInfo.attachmentCount   = std::size(Attachments);
	FramebufferInfo.pAttachments      = Attachments;

	if( auto CreateResult = Device.createFramebufferUnique(FramebufferInfo);
		CreateResult.result == vk::Result::eSuccess )
	{
		return std::move(CreateResult.value);
	}
	else
	{
		std::fprintf(
			stderr, "Error creating framebuffer: %s\n",
			vk::to_string(CreateResult.result).c_str());
		return {};
	}
}

std::tuple<vk::UniquePipeline, vk::UniquePipelineLayout> CreateGraphicsPipeline(
	vk::Device Device, std::span<const vk::PushConstantRange> PushConstants,
	std::span<const vk::DescriptorSetLayout> SetLayouts,
	vk::ShaderModule VertModule, vk::ShaderModule FragModule,
	vk::RenderPass RenderPass, vk::PolygonMode PolygonMode)
{
	// Create Pipeline Layout
	vk::PipelineLayoutCreateInfo GraphicsPipelineLayoutInfo = {};

	GraphicsPipelineLayoutInfo.pSetLayouts            = SetLayouts.data();
	GraphicsPipelineLayoutInfo.setLayoutCount         = SetLayouts.size();
	GraphicsPipelineLayoutInfo.pPushConstantRanges    = PushConstants.data();
	GraphicsPipelineLayoutInfo.pushConstantRangeCount = PushConstants.size();

	vk::UniquePipelineLayout GraphicsPipelineLayout = {};
	if( auto CreateResult
		= Device.createPipelineLayoutUnique(GraphicsPipelineLayoutInfo);
		CreateResult.result == vk::Result::eSuccess )
	{
		GraphicsPipelineLayout = std::move(CreateResult.value);
	}
	else
	{
		std::fprintf(
			stderr, "Error creating pipeline layout: %s\n",
			vk::to_string(CreateResult.result).c_str());
		return {};
	}

	// Describe the stage and entry point of each shader
	const vk::PipelineShaderStageCreateInfo ShaderStagesInfo[2] = {
		vk::PipelineShaderStageCreateInfo(
			{},                               // Flags
			vk::ShaderStageFlagBits::eVertex, // Shader Stage
			VertModule,                       // Shader Module
			"main", // Shader entry point function name
			{}      // Shader specialization info
			),
		vk::PipelineShaderStageCreateInfo(
			{},                                 // Flags
			vk::ShaderStageFlagBits::eFragment, // Shader Stage
			FragModule,                         // Shader Module
			"main", // Shader entry point function name
			{}      // Shader specialization info
			),
	};

	vk::PipelineVertexInputStateCreateInfo VertexInputState = {};

	static std::array<vk::VertexInputBindingDescription, 2>
		VertexBindingDescriptions
		= {Vulkan::CreateVertexInputBinding<Blam::Vertex>(0),
		   Vulkan::CreateVertexInputBinding<Blam::LightmapVertex>(1)};

	VertexInputState.vertexBindingDescriptionCount
		= std::size(VertexBindingDescriptions);
	VertexInputState.pVertexBindingDescriptions
		= VertexBindingDescriptions.data();

	static std::array<vk::VertexInputAttributeDescription, 7>
		AttributeDescriptions = {};
	// Position
	AttributeDescriptions[0].binding  = 0;
	AttributeDescriptions[0].location = 0;
	AttributeDescriptions[0].format   = vk::Format::eR32G32B32Sfloat;
	AttributeDescriptions[0].offset   = offsetof(Blam::Vertex, Position);
	// Normal
	AttributeDescriptions[1].binding  = 0;
	AttributeDescriptions[1].location = 1;
	AttributeDescriptions[1].format   = vk::Format::eR32G32B32Sfloat;
	AttributeDescriptions[1].offset   = offsetof(Blam::Vertex, Normal);
	// Binormal
	AttributeDescriptions[2].binding  = 0;
	AttributeDescriptions[2].location = 2;
	AttributeDescriptions[2].format   = vk::Format::eR32G32B32Sfloat;
	AttributeDescriptions[2].offset   = offsetof(Blam::Vertex, Binormal);
	// Tangent
	AttributeDescriptions[3].binding  = 0;
	AttributeDescriptions[3].location = 3;
	AttributeDescriptions[3].format   = vk::Format::eR32G32B32Sfloat;
	AttributeDescriptions[3].offset   = offsetof(Blam::Vertex, Tangent);
	// UV
	AttributeDescriptions[4].binding  = 0;
	AttributeDescriptions[4].location = 4;
	AttributeDescriptions[4].format   = vk::Format::eR32G32Sfloat;
	AttributeDescriptions[4].offset   = offsetof(Blam::Vertex, UV);

	// Normal-Lightmap
	AttributeDescriptions[5].binding  = 1;
	AttributeDescriptions[5].location = 5;
	AttributeDescriptions[5].format   = vk::Format::eR32G32B32Sfloat;
	AttributeDescriptions[5].offset   = offsetof(Blam::LightmapVertex, Normal);
	// UV-Lightmap
	AttributeDescriptions[6].binding  = 1;
	AttributeDescriptions[6].location = 6;
	AttributeDescriptions[6].format   = vk::Format::eR32G32Sfloat;
	AttributeDescriptions[6].offset   = offsetof(Blam::LightmapVertex, UV);

	VertexInputState.vertexAttributeDescriptionCount
		= AttributeDescriptions.size();
	VertexInputState.pVertexAttributeDescriptions
		= AttributeDescriptions.data();

	vk::PipelineInputAssemblyStateCreateInfo InputAssemblyState = {};
	InputAssemblyState.topology = vk::PrimitiveTopology::eTriangleList;
	InputAssemblyState.primitiveRestartEnable = false;

	vk::PipelineViewportStateCreateInfo ViewportState = {};

	static const vk::Viewport DefaultViewport = {0, 0, 16, 16, 0.0f, 1.0f};
	static const vk::Rect2D   DefaultScissor  = {{0, 0}, {16, 16}};
	ViewportState.viewportCount               = 1;
	ViewportState.pViewports                  = &DefaultViewport;
	ViewportState.scissorCount                = 1;
	ViewportState.pScissors                   = &DefaultScissor;

	vk::PipelineRasterizationStateCreateInfo RasterizationState = {};

	RasterizationState.depthClampEnable        = false;
	RasterizationState.rasterizerDiscardEnable = false;
	RasterizationState.polygonMode             = PolygonMode;
	RasterizationState.cullMode                = vk::CullModeFlagBits::eBack;
	RasterizationState.frontFace               = vk::FrontFace::eClockwise;
	RasterizationState.depthBiasEnable         = false;
	RasterizationState.depthBiasConstantFactor = 0.0f;
	RasterizationState.depthBiasClamp          = 0.0f;
	RasterizationState.depthBiasSlopeFactor    = 0.0;
	RasterizationState.lineWidth               = 1.0f;

	vk::PipelineMultisampleStateCreateInfo MultisampleState = {};

	MultisampleState.rasterizationSamples  = RenderSamples;
	MultisampleState.sampleShadingEnable   = true;
	MultisampleState.minSampleShading      = 1.0f;
	MultisampleState.pSampleMask           = nullptr;
	MultisampleState.alphaToCoverageEnable = false;
	MultisampleState.alphaToOneEnable      = false;

	vk::PipelineDepthStencilStateCreateInfo DepthStencilState = {};

	DepthStencilState.depthTestEnable       = true;
	DepthStencilState.depthWriteEnable      = true;
	DepthStencilState.depthCompareOp        = vk::CompareOp::eLessOrEqual;
	DepthStencilState.depthBoundsTestEnable = false;
	DepthStencilState.stencilTestEnable     = false;
	DepthStencilState.front                 = vk::StencilOp::eKeep;
	DepthStencilState.back                  = vk::StencilOp::eKeep;
	DepthStencilState.minDepthBounds        = 0.0f;
	DepthStencilState.maxDepthBounds        = 1.0f;

	vk::PipelineColorBlendStateCreateInfo ColorBlendState = {};

	ColorBlendState.logicOpEnable   = false;
	ColorBlendState.logicOp         = vk::LogicOp::eClear;
	ColorBlendState.attachmentCount = 1;

	vk::PipelineColorBlendAttachmentState BlendAttachmentState = {};

	BlendAttachmentState.blendEnable         = false;
	BlendAttachmentState.srcColorBlendFactor = vk::BlendFactor::eZero;
	BlendAttachmentState.dstColorBlendFactor = vk::BlendFactor::eZero;
	BlendAttachmentState.colorBlendOp        = vk::BlendOp::eAdd;
	BlendAttachmentState.srcAlphaBlendFactor = vk::BlendFactor::eZero;
	BlendAttachmentState.dstAlphaBlendFactor = vk::BlendFactor::eZero;
	BlendAttachmentState.alphaBlendOp        = vk::BlendOp::eAdd;
	BlendAttachmentState.colorWriteMask
		= vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
		| vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

	ColorBlendState.pAttachments = &BlendAttachmentState;

	vk::PipelineDynamicStateCreateInfo DynamicState = {};
	vk::DynamicState                   DynamicStates[]
		= {// The viewport and scissor of the framebuffer will be dynamic at
		   // run-time
		   // so we definately add these
		   vk::DynamicState::eViewport, vk::DynamicState::eScissor};
	DynamicState.dynamicStateCount = std::size(DynamicStates);
	DynamicState.pDynamicStates    = DynamicStates;

	vk::GraphicsPipelineCreateInfo RenderPipelineInfo = {};

	RenderPipelineInfo.stageCount          = 2; // Vert + Frag stages
	RenderPipelineInfo.pStages             = ShaderStagesInfo;
	RenderPipelineInfo.pVertexInputState   = &VertexInputState;
	RenderPipelineInfo.pInputAssemblyState = &InputAssemblyState;
	RenderPipelineInfo.pViewportState      = &ViewportState;
	RenderPipelineInfo.pRasterizationState = &RasterizationState;
	RenderPipelineInfo.pMultisampleState   = &MultisampleState;
	RenderPipelineInfo.pDepthStencilState  = &DepthStencilState;
	RenderPipelineInfo.pColorBlendState    = &ColorBlendState;
	RenderPipelineInfo.pDynamicState       = &DynamicState;
	RenderPipelineInfo.subpass             = 0;
	RenderPipelineInfo.renderPass          = RenderPass;
	RenderPipelineInfo.layout              = GraphicsPipelineLayout.get();

	// Create Pipeline
	vk::UniquePipeline Pipeline
		= Device.createGraphicsPipelineUnique({}, RenderPipelineInfo).value;
	return std::make_tuple(
		std::move(Pipeline), std::move(GraphicsPipelineLayout));
}

std::string FormatDeviceCaps(vk::PhysicalDevice PhysicalDevice)
{
	std::string Result;

	const vk::PhysicalDeviceProperties Properties
		= PhysicalDevice.getProperties();

	Result += Common::Format(
		"Device Name: %.256s\n", Properties.deviceName.data());
	Result += Common::Format(
		"Device Type: %s\n", vk::to_string(Properties.deviceType).c_str());
	Result += Common::Format(
		"DeviceID/VendorID: %8x:%8x\n", Properties.deviceID,
		Properties.vendorID);

	const vk::PhysicalDeviceMemoryProperties MemoryProperties
		= PhysicalDevice.getMemoryProperties();
	for( std::uint8_t HeapIdx = 0; HeapIdx < MemoryProperties.memoryHeapCount;
		 ++HeapIdx )
	{
		const auto& CurHeap = MemoryProperties.memoryHeaps[HeapIdx];
		Result += Common::Format(
			"Heap %2u: %12s %s\n", HeapIdx,
			Common::FormatByteCount(CurHeap.size).c_str(),
			vk::to_string(CurHeap.flags).c_str());
	}

	return Result;
}