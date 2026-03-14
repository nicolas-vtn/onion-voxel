#include "AssetsManager.hpp"

#include <Windows.h>

#include <fstream>
#include <iostream>
#include <stdexcept>

namespace
{
	std::filesystem::path GetExecutableDirectory()
	{
		char buffer[MAX_PATH];
		GetModuleFileNameA(nullptr, buffer, MAX_PATH);
		return std::filesystem::path(buffer).parent_path();
	}
} // namespace

namespace onion::voxel
{
	AssetsManager::AssetsManager()
	{
		m_AssetsDirectory = GetExecutableDirectory() / "assets";
		if (!std::filesystem::exists(m_AssetsDirectory))
		{
			std::cerr << "WARNING: Assets directory not found at: " << m_AssetsDirectory << std::endl;
			throw std::runtime_error("Assets directory not found at: " + m_AssetsDirectory.string());
		}

		std::filesystem::path resourcePacksDirectory = m_AssetsDirectory / "resourcepacks";
		if (!std::filesystem::exists(resourcePacksDirectory))
		{
			std::cerr << "WARNING: Resource packs directory not found at: " << resourcePacksDirectory << std::endl;
			throw std::runtime_error("Resource packs directory not found at: " + resourcePacksDirectory.string());
		}
		m_ResourcePackManager.SetResourcePackDirectory(resourcePacksDirectory);
	}

	AssetsManager::~AssetsManager() {}

	std::string AssetsManager::GetFileText(const std::filesystem::path& path) const
	{
		std::filesystem::path fullPath = m_AssetsDirectory / path;
		if (!std::filesystem::exists(fullPath))
		{
			throw std::runtime_error("AssetsManager::GetFileText: File not found: " + fullPath.string());
		}

		std::ifstream file(fullPath);
		if (!file.is_open())
		{
			throw std::runtime_error("AssetsManager::GetFileText: Failed to open file: " + fullPath.string());
		}

		std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

		return content;
	}

	std::vector<unsigned char> AssetsManager::GetFileBinary(const std::filesystem::path& path) const
	{
		std::filesystem::path fullPath = m_AssetsDirectory / path;

		if (!std::filesystem::exists(fullPath))
		{
			throw std::runtime_error("AssetsManager::GetFileBinary: File not found: " + fullPath.string());
		}

		std::ifstream file(fullPath, std::ios::binary);
		if (!file.is_open())
		{
			throw std::runtime_error("AssetsManager::GetFileBinary: Failed to open file: " + fullPath.string());
		}

		std::vector<unsigned char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

		return buffer;
	}

	std::string AssetsManager::GetResourcePackFileText(const std::filesystem::path& path) const
	{
		return m_ResourcePackManager.GetFileText(path);
	}

	std::vector<unsigned char> AssetsManager::GetResourcePackFileBinary(const std::filesystem::path& path) const
	{
		return m_ResourcePackManager.GetFileBinary(path);
	}

	void AssetsManager::SetCurrentResourcePack(const std::string& resourcePack)
	{
		m_ResourcePackManager.SetCurrentResourcePack(resourcePack);
	}

	std::string AssetsManager::GetCurrentResourcePack() const
	{
		return m_ResourcePackManager.GetCurrentResourcePack();
	}

	std::filesystem::path AssetsManager::GetAssetsDirectory() const
	{
		return m_AssetsDirectory;
	}

	std::filesystem::path AssetsManager::GetResourcePacksDirectory() const
	{
		return m_AssetsDirectory / "resourcepacks";
	}

} // namespace onion::voxel
