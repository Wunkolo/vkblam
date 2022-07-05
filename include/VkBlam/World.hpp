#pragma once

#include <VkBlam/VkBlam.hpp>

#include <filesystem>
#include <memory>

#include <optional>

#include <mio/mmap.hpp>

namespace VkBlam
{

// A simulated representation of an instantiated Halo map
class World
{
private:
	std::filesystem::path BitmapPath;

	mio::mmap_source BitmapFileData;

	const Blam::MapFile& MapFile;

	glm::f32vec3 WorldBoundMax;
	glm::f32vec3 WorldBoundMin;

	World(const Blam::MapFile& MapFile);

public:
	~World();

	World(World&&) = default;

	const Blam::MapFile& GetMapFile() const
	{
		return MapFile;
	}

	const Blam::MapHeader& GetMapHeader() const
	{
		return MapFile.MapHeader;
	}

	const glm::f32mat2x3 GetWorldBounds() const
	{
		return glm::f32mat2x3(WorldBoundMin, WorldBoundMax);
	}

	static std::optional<World>
		Create(const Blam::MapFile& MapFile, std::filesystem::path BitmapPath);
};
} // namespace VkBlam