#include "ResourcePackTile.hpp"

namespace onion::voxel
{
	ResourcePackTile::ResourcePackTile(const std::string& name, Texture texture)
		: GuiElement(name), m_Thumbnail(name + "_Thumbnail", std::move(texture)), m_Checkbox(name + "_Checkbox"),
		  m_NameLabel(name + "_NameLabel"), m_DescriptionLabel(name + "_DescriptionLabel")
	{
		SubscribeToControlEvents();

		m_NameLabel.SetTextAlignment(Font::eTextAlignment::Left);
		m_NameLabel.SetTextColor(s_ColorMainText);

		m_DescriptionLabel.SetTextAlignment(Font::eTextAlignment::Left);
		m_DescriptionLabel.SetTextColor(s_ColorSecondaryText);
	}

	ResourcePackTile::~ResourcePackTile() {}

	void ResourcePackTile::Render()
	{
		if (!HasBeenInit())
		{
			Initialize();
		}

		// ----- Render Checkbox -----
		glm::ivec2 checkboxSize{m_Size.y / 2, m_Size.y / 2};
		glm::ivec2 checkboxPos{m_Position.x - (m_Size.x / 2) + checkboxSize.x, m_Position.y};
		m_Checkbox.SetSize(checkboxSize);
		m_Checkbox.SetPosition(checkboxPos);
		m_Checkbox.Render();

		// ----- Render Thumbnail -----
		glm::ivec2 thumbnailSize{m_Size.y, m_Size.y};
		glm::ivec2 thumbnailPos{m_Position.x - (m_Size.x / 2) + checkboxSize.x * 2 + thumbnailSize.x / 2, m_Position.y};
		m_Thumbnail.SetPosition(thumbnailPos);
		m_Thumbnail.SetSize(thumbnailSize);
		m_Thumbnail.Render();

		// ----- Render Name and Description -----
		glm::ivec2 nameLabelPos{thumbnailPos.x + thumbnailSize.x / 2 + 10, m_Position.y - m_Size.y * 0.2f};
		float textHeight = m_Size.y * 0.25f;
		m_NameLabel.SetPosition(nameLabelPos);
		m_NameLabel.SetTextHeight(textHeight);
		m_NameLabel.Render();

		glm::ivec2 descriptionLabelPos{nameLabelPos.x, m_Position.y + m_Size.y * 0.2f};
		m_DescriptionLabel.SetPosition(descriptionLabelPos);
		m_DescriptionLabel.SetTextHeight(textHeight * 0.8f);
		m_DescriptionLabel.Render();
	}

	void ResourcePackTile::Initialize()
	{
		if (m_Thumbnail.GetTextureHeight() == -1)
		{
			// Failed to load the thumbnail texture, use the default one instead
			std::vector<unsigned char> thumbnailData =
				EngineContext::Get().Assets->GetFileBinary(s_DefaultResourcePackThumbnailPath);
			m_Thumbnail.SwapTexture(Texture(s_DefaultResourcePackThumbnailPath.string(), thumbnailData));
		}

		m_Thumbnail.Initialize();
		m_Checkbox.Initialize();
		m_NameLabel.Initialize();
		m_DescriptionLabel.Initialize();
		SetInitState(true);
	}

	void ResourcePackTile::Delete()
	{
		m_Thumbnail.Delete();
		m_Checkbox.Delete();
		m_NameLabel.Delete();
		m_DescriptionLabel.Delete();
		SetDeletedState(true);
	}

	void ResourcePackTile::ReloadTextures()
	{
		m_Checkbox.ReloadTextures();
	}

	void ResourcePackTile::SetSize(const glm::vec2& size)
	{
		m_Size = size;
	}

	glm::vec2 ResourcePackTile::GetSize() const
	{
		return m_Size;
	}

	void ResourcePackTile::SetPosition(const glm::vec2& pos)
	{
		m_Position = pos;
	}

	glm::vec2 ResourcePackTile::GetPosition() const
	{
		return m_Position;
	}

	void ResourcePackTile::SetChecked(bool checked)
	{
		m_Checkbox.SetChecked(checked);
	}

	bool ResourcePackTile::IsChecked() const
	{
		return m_Checkbox.IsChecked();
	}

	void ResourcePackTile::SetResourcePackName(const std::string& name)
	{
		m_NameLabel.SetText(name);
	}

	std::string ResourcePackTile::GetResourcePackName() const
	{
		return m_NameLabel.GetText();
	}

	void ResourcePackTile::SetResourcePackDescription(const std::string& description)
	{
		m_DescriptionLabel.SetText(description);
	}

	std::string ResourcePackTile::GetResourcePackDescription() const
	{
		return m_DescriptionLabel.GetText();
	}

	void ResourcePackTile::SubscribeToControlEvents()
	{
		m_EventHandles.push_back(m_Checkbox.OnCheckedChanged.Subscribe([this](const Checkbox& sender)
																	   { Handle_CheckboxCheckedChanged(sender); }));
	}

	void ResourcePackTile::Handle_CheckboxCheckedChanged(const Checkbox& sender)
	{
		(void) sender; // Unused
		OnCheckedChanged.Trigger(*this);
	}

} // namespace onion::voxel
