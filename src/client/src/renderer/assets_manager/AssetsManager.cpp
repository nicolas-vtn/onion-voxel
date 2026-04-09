#include "AssetsManager.hpp"

#include <shared/utils/Utils.hpp>

#include <fstream>
#include <iostream>
#include <stdexcept>

namespace onion::voxel
{
	std::filesystem::path AssetsManager::s_ExecutableDirectory = Utils::GetExecutableDirectory();

	AssetsManager::AssetsManager()
	{
		const auto assetsDir = GetAssetsDirectory();
		if (!std::filesystem::exists(assetsDir))
		{
			std::cerr << "ERROR: Assets directory not found at: " << assetsDir << std::endl;
			throw std::runtime_error("Assets directory not found at: " + assetsDir.string());
		}

		const auto texturesDir = GetTexturesDirectory();
		if (!std::filesystem::exists(texturesDir))
		{
			std::cerr << "ERROR: Textures directory not found at: " << texturesDir << std::endl;
			throw std::runtime_error("Textures directory not found at: " + texturesDir.string());
		}

		const auto shadersDir = GetShadersDirectory();
		if (!std::filesystem::exists(shadersDir))
		{
			std::cerr << "ERROR: Shaders directory not found at: " << shadersDir << std::endl;
			throw std::runtime_error("Shaders directory not found at: " + shadersDir.string());
		}

		const auto resourcePacksDirectory = GetResourcePacksDirectory();
		if (!std::filesystem::exists(resourcePacksDirectory))
		{
			// If the resource packs directory doesn't exist, we create it.
			std::cout << "WARNING: Resource packs directory not found at: " << resourcePacksDirectory
					  << ". Creating it now." << std::endl;
			std::filesystem::create_directories(resourcePacksDirectory);
		}

		m_ResourcePackManager.SetResourcePackDirectory(resourcePacksDirectory);

		const auto textsDir = GetTextsDirectory();
		if (!std::filesystem::exists(textsDir))
		{
			std::cerr << "ERROR: Texts directory not found at: " << textsDir << std::endl;
			throw std::runtime_error("Texts directory not found at: " + textsDir.string());
		}

		const auto appIconsDir = GetAppIconsDirectory();
		if (!std::filesystem::exists(appIconsDir))
		{
			std::cerr << "ERROR: App icons directory not found at: " << appIconsDir << std::endl;
			throw std::runtime_error("App icons directory not found at: " + appIconsDir.string());
		}
	}

	AssetsManager::~AssetsManager() {}

	std::string AssetsManager::GetFileText(const std::filesystem::path& path) const
	{
		std::filesystem::path fullPath = GetAssetsDirectory() / path;
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
		std::filesystem::path fullPath = GetAssetsDirectory() / path;

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

	std::filesystem::path AssetsManager::GetAssetsDirectory()
	{
		return s_ExecutableDirectory / ASSETS_FOLDER_NAME;
	}

	std::filesystem::path AssetsManager::GetTexturesDirectory()
	{
		return s_ExecutableDirectory / ASSETS_FOLDER_NAME / TEXTURES_FOLDER_NAME;
	}

	std::filesystem::path AssetsManager::GetShadersDirectory()
	{
		return s_ExecutableDirectory / ASSETS_FOLDER_NAME / SHADERS_FOLDER_NAME;
	}

	std::filesystem::path AssetsManager::GetResourcePacksDirectory()
	{
		return s_ExecutableDirectory / ASSETS_FOLDER_NAME / RESOURCE_PACKS_FOLDER_NAME;
	}

	std::filesystem::path AssetsManager::GetTextsDirectory()
	{
		return s_ExecutableDirectory / ASSETS_FOLDER_NAME / TEXTS_FOLDER_NAME;
	}

	std::filesystem::path AssetsManager::GetAppIconsDirectory()
	{
		return s_ExecutableDirectory / ASSETS_FOLDER_NAME / APPICONS_FOLDER_NAME;
	}

} // namespace onion::voxel
