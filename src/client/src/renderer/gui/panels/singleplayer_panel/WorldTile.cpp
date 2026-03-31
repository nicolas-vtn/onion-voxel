#include "WorldTile.hpp"

namespace onion::voxel
{
	WorldTile::WorldTile(const std::string& name, const WorldInfos& worldInfos, Texture texture)
		: GuiElement(name), m_WorldInfos(worldInfos), m_LabelTitle(name + "_LabelTitle"),
		  m_LabelDescription(name + "_LabelDescription"), m_LabelDetails(name + "_LabelLastPlayed"),
		  m_SpriteThumbnail(name + "_Sprite", std::move(texture))
	{
		SubscribeToControlEvents();

		m_LabelTitle.SetTextAlignment(Font::eTextAlignment::Left);
		m_LabelTitle.SetTextColor(s_ColorMainText);

		m_LabelDescription.SetTextAlignment(Font::eTextAlignment::Left);
		m_LabelDescription.SetTextColor(s_ColorSecondaryText);

		m_LabelDetails.SetTextAlignment(Font::eTextAlignment::Left);
		m_LabelDetails.SetTextColor(s_ColorTertiaryText);
	}

	WorldTile::~WorldTile()
	{
		m_EventHandles.clear();
	}

	void WorldTile::Initialize()
	{
		m_LabelTitle.Initialize();
		m_LabelDescription.Initialize();
		m_LabelDetails.Initialize();
		m_SpriteThumbnail.Initialize();

		SetInitState(true);
	}

	void WorldTile::Render()
	{
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
			float currentTime = static_cast<float>(glfwGetTime());
			if (currentTime - m_LastClickTime < 0.25f)
			{
				EvtTileDoubleClicked.Trigger(*this);
				std::cout << "Tile double-clicked: " << m_WorldInfos.Name << std::endl;
			}
			else
			{
				m_IsSelected = true;
				EvtTileSelected.Trigger(*this);
				std::cout << "Tile selected: " << m_WorldInfos.Name << std::endl;
			}

			m_LastClickTime = currentTime;
		}

		// Update previous state
		m_WasMouseDown = isMouseDown;
		m_WasMouseHovering = isHovered;

		const int borderThickness = static_cast<int>(round(4.f / 1009.f * s_ScreenHeight));
		const glm::ivec2 thumbnailSize{static_cast<int>(round(m_Size.y - (4 * borderThickness)))};
		const int posBorderLeft = m_Position.x - (m_Size.x / 2);
		const int posBorderTop = m_Position.y - (m_Size.y / 2);
		const glm::ivec2 thumbnailPos{
			posBorderLeft + (2 * borderThickness) + static_cast<int>(round(thumbnailSize.x / 2)), m_Position.y};

		const int startTextPosX = thumbnailPos.x + static_cast<int>(round(thumbnailSize.x / 2)) + (2 * borderThickness);
		const float textSpacingY = m_Size.y / 4.f;

		// ---- Render Border ----
		if (m_IsSelected)
		{
			// TODO : Render a white rectangle
		}

		// ---- Render Background ----
		if (m_IsSelected)
		{
			// TODO : Render a black rectangle
		}

		// ---- Render Thumbnail ----
		m_SpriteThumbnail.SetPosition(thumbnailPos);
		m_SpriteThumbnail.SetSize(thumbnailSize);
		m_SpriteThumbnail.Render();

		// ---- Render Title ----
		float textHeight = s_ScreenHeight * (31.f / 1009.f);
		int textPosY = static_cast<int>(round(posBorderTop + textSpacingY));
		m_LabelTitle.SetPosition({startTextPosX, textPosY});
		m_LabelTitle.SetTextHeight(textHeight);
		m_LabelTitle.SetText(m_WorldInfos.Name);
		m_LabelTitle.Render();

		// ---- Render Description ----
		textPosY = static_cast<int>(round(posBorderTop + (2 * textSpacingY)));
		m_LabelDescription.SetPosition({startTextPosX, textPosY});
		m_LabelDescription.SetTextHeight(textHeight);
		m_LabelDescription.SetText(FormatDescription());
		m_LabelDescription.Render();

		// ---- Render Last Played ----
		textPosY = static_cast<int>(round(posBorderTop + (3 * textSpacingY)));
		m_LabelDetails.SetPosition({startTextPosX, textPosY});
		m_LabelDetails.SetTextHeight(textHeight);
		m_LabelDetails.SetText(FormatDetails());
		m_LabelDetails.Render();
	}

	void WorldTile::Delete()
	{
		m_LabelTitle.Delete();
		m_LabelDescription.Delete();
		m_LabelDetails.Delete();
		m_SpriteThumbnail.Delete();

		SetInitState(false);
	}

	void WorldTile::ReloadTextures()
	{
		m_LabelTitle.ReloadTextures();
		m_LabelDescription.ReloadTextures();
		m_LabelDetails.ReloadTextures();
	}

	void WorldTile::SetSize(const glm::vec2& size)
	{
		m_Size = size;
	}

	glm::vec2 WorldTile::GetSize() const
	{
		return m_Size;
	}

	void WorldTile::SetPosition(const glm::vec2& pos)
	{
		m_Position = pos;
	}

	glm::vec2 WorldTile::GetPosition() const
	{
		return m_Position;
	}

	void WorldTile::SetWorldInfos(const WorldInfos& worldInfos)
	{
		m_WorldInfos = worldInfos;
	}

	const WorldInfos WorldTile::GetWorldInfos() const
	{
		return m_WorldInfos;
	}

	void WorldTile::SetSelected(bool selected)
	{
		m_IsSelected = selected;
	}

	bool WorldTile::IsSelected() const
	{
		return m_IsSelected;
	}

	void WorldTile::SetThumbnailTexture(Texture& texture)
	{
		m_SpriteThumbnail.SwapTexture(std::move(texture));
	}

	bool WorldTile::IsMouseHovering() const
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

	std::string WorldTile::FormatDescription() const
	{
		std::string description = m_WorldInfos.SaveDirectory.filename().string();
		const std::string lastPlayedStr = m_WorldInfos.LastPlayedDate.toString("%d/%m/%Y %H:%M");
		description += " (" + lastPlayedStr + ")";
		return description;
	}

	std::string WorldTile::FormatDetails() const
	{
		return (WorldGenerator::WorldGenerationTypeToString(m_WorldInfos.WorldGenerationType) +
				", Version: " + m_WorldInfos.Version);
	}

	void WorldTile::SubscribeToControlEvents() {}
} // namespace onion::voxel
