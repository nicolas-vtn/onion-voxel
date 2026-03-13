#include "ResourcePackManager.hpp"

namespace onion::voxel
{
	ResourcePackManager::ResourcePackManager() {}

	ResourcePackManager::~ResourcePackManager() {}

	std::string ResourcePackManager::GetFileText(const std::filesystem::path& path) const
	{
		std::lock_guard lock(m_Mutex);
		if (m_ResourcePackArchive && m_ResourcePackArchive->FileExists(path))
		{
			return m_ResourcePackArchive->GetFileText(path);
		}
		else if (m_DefaultResourcePackArchive && m_DefaultResourcePackArchive->FileExists(path))
		{
			return m_DefaultResourcePackArchive->GetFileText(path);
		}
		else
		{
			throw std::runtime_error("ResourcePackManager::GetFileText: File not found: " + path.string());
		}
	}

	std::vector<unsigned char> ResourcePackManager::GetFileBinary(const std::filesystem::path& path) const
	{
		std::lock_guard lock(m_Mutex);
		if (m_ResourcePackArchive && m_ResourcePackArchive->FileExists(path))
		{
			return m_ResourcePackArchive->GetFileBinary(path);
		}
		else if (m_DefaultResourcePackArchive && m_DefaultResourcePackArchive->FileExists(path))
		{
			return m_DefaultResourcePackArchive->GetFileBinary(path);
		}
		else
		{
			throw std::runtime_error("ResourcePackManager::GetFileBinary: File not found: " + path.string());
		}
	}

	void ResourcePackManager::SetResourcePackDirectory(const std::filesystem::path& directory)
	{
		std::lock_guard lock(m_Mutex);
		m_ResourcePackDirectory = directory;

		auto packPath = m_ResourcePackDirectory / m_CurrentResourcePack;
		packPath += ".zip";

		m_ResourcePackArchive = std::make_unique<ZipArchive>(packPath);

		auto defaultPackPath = m_ResourcePackDirectory / DEFAULT_RESOURCE_PACK;
		defaultPackPath += ".zip";

		m_DefaultResourcePackArchive = std::make_unique<ZipArchive>(defaultPackPath);
	}

	std::string ResourcePackManager::GetCurrentResourcePack() const
	{
		std::lock_guard lock(m_Mutex);
		return m_CurrentResourcePack;
	}

	void ResourcePackManager::SetCurrentResourcePack(const std::string& resourcePack)
	{
		bool changed = false;
		{
			std::lock_guard lock(m_Mutex);
			if (m_CurrentResourcePack != resourcePack)
			{
				m_CurrentResourcePack = resourcePack;

				auto packPath = m_ResourcePackDirectory / m_CurrentResourcePack;
				packPath += ".zip";
				m_ResourcePackArchive = std::make_unique<ZipArchive>(packPath);

				changed = true;
			}
		}

		if (changed)
		{
			OnResourcePackChanged.Trigger(m_CurrentResourcePack);
		}
	}

} // namespace onion::voxel
