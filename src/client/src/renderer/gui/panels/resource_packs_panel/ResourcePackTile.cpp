#include "ResourcePackTile.hpp"

#include <renderer/gui/colored_background/ColoredBackground.hpp>

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

	ResourcePackTile::~ResourcePackTile()
	{
		m_EventHandles.clear();
	}

	void ResourcePackTile::Render()
	{
		if (!HasBeenInit())
		{
			Initialize();
		}

		bool isHovered = IsMouseHovering();

		// ---- Hover Handling ----
		if (isHovered && !m_WasMouseHovering)
		{
			auto inputsManager = EngineContext::Get().Inputs;
			inputsManager->SetCursorStyle(CursorStyle::Hand);
		}
		else if (!isHovered && m_WasMouseHovering)
		{
			auto inputsManager = EngineContext::Get().Inputs;
			inputsManager->SetCursorStyle(CursorStyle::Arrow);
		}

		// ---- Click Handling ----
		bool isMouseDown = s_InputsSnapshot->Mouse.LeftButtonPressed;
		if (isHovered && isMouseDown && !m_WasMouseDown)
		{
			if (!IsSelected() && !m_Checkbox.IsHovered())
			{
				SetSelected(true);
				EvtCheckedChanged.Trigger(*this);
			}
		}

		bool isSelected = IsSelected();

		// Render Border and Background if Selected
		const int borderThickness = static_cast<int>(round(4.f / 1009.f * s_ScreenHeight));
		const int posBorderLeft = static_cast<int>(round(m_Position.x - (m_Size.x / 2) - (2 * borderThickness)));
		const int posBorderRight = static_cast<int>(round(m_Position.x + (m_Size.x / 2) + (2 * borderThickness)));
		const int posBorderTop = static_cast<int>(round(m_Position.y - (m_Size.y / 2) - (2 * borderThickness)));
		const int posBorderBottom = static_cast<int>(round(m_Position.y + (m_Size.y / 2) + (2 * borderThickness)));

		// ---- Render Border and Background if Selected ----
		if (isSelected)
		{
			// ---- Render Border ----
			ColoredBackground::CornerOptions bgOptionsBorder;
			bgOptionsBorder.TopLeftCorner = {posBorderLeft, posBorderTop};
			bgOptionsBorder.BottomRightCorner = {posBorderRight, posBorderBottom};
			bgOptionsBorder.Color = glm::vec4(1.f, 1.f, 1.f, 1.f);
			bgOptionsBorder.ZOffset = -0.5f;
			ColoredBackground::Render(bgOptionsBorder);

			// ---- Render Background ----
			ColoredBackground::CornerOptions bgOptions;
			bgOptions.TopLeftCorner = {posBorderLeft + borderThickness, posBorderTop + borderThickness};
			bgOptions.BottomRightCorner = {posBorderRight - borderThickness, posBorderBottom - borderThickness};
			bgOptions.Color = glm::vec4(0.f, 0.f, 0.f, 1.f);
			bgOptions.ZOffset = -0.4f;
			ColoredBackground::Render(bgOptions);
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

		// ----- Render Name -----
		int topY = static_cast<int>(round(m_Position.y - (m_Size.y / 2)));
		int namePosY = topY + static_cast<int>(round(m_Size.y * 0.20f));
		glm::ivec2 nameLabelPos{thumbnailPos.x + thumbnailSize.x / 2 + 10, namePosY};
		m_NameLabel.SetPosition(nameLabelPos);
		m_NameLabel.SetTextHeight(s_TextHeight);
		m_NameLabel.Render();

		// ----- Render Description -----
		int descriptionPosY = static_cast<int>(round(m_Position.y + m_Size.y * 0.15f));
		glm::ivec2 descriptionLabelPos{nameLabelPos.x, descriptionPosY};
		m_DescriptionLabel.SetPosition(descriptionLabelPos);
		m_DescriptionLabel.SetTextHeight(s_TextHeight);
		m_DescriptionLabel.Render();

		// ---- Update previous state ----
		m_WasMouseHovering = isHovered;
		m_WasMouseDown = isMouseDown;
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

	void ResourcePackTile::SetVisibility(const Visibility& visibility)
	{
		Visibility cbVisibility = Visibility::Compose(visibility, m_Checkbox.GetPosition(), m_Checkbox.GetSize());
		m_Checkbox.SetVisibility(cbVisibility);
		GuiElement::SetVisibility(visibility);
	}

	void ResourcePackTile::SetSelected(bool selected)
	{
		m_Checkbox.SetChecked(selected);
	}

	bool ResourcePackTile::IsSelected() const
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
		m_EventHandles.push_back(m_Checkbox.EvtCheckedChanged.Subscribe([this](const Checkbox& sender)
																	   { Handle_CheckboxCheckedChanged(sender); }));
	}

	void ResourcePackTile::Handle_CheckboxCheckedChanged(const Checkbox& sender)
	{
		(void) sender; // Unused

		EvtCheckedChanged.Trigger(*this);
	}

	bool ResourcePackTile::IsMouseHovering() const
	{
		glm::vec2 mousePos = EngineContext::Get().Inputs->GetMousePosition();

		Visibility visibility = GetVisibility();
		if (!visibility.IsVisible)
		{
			return false;
		}

		glm::vec2 topLeft;
		glm::vec2 bottomRight;

		if (visibility.IsFullyVisible)
		{
			topLeft = glm::vec2(m_Position) - glm::vec2(m_Size) * 0.5f;
			bottomRight = glm::vec2(m_Position) + glm::vec2(m_Size) * 0.5f;
		}
		else
		{
			topLeft = glm::vec2(visibility.VisibleAreaTopLeftCorner);
			bottomRight = glm::vec2(visibility.VisibleAreaBottomRightCorner);
		}

		bool hovered = mousePos.x >= topLeft.x && mousePos.x <= bottomRight.x && mousePos.y >= topLeft.y &&
			mousePos.y <= bottomRight.y;

		return hovered;
	}

} // namespace onion::voxel
