#include "WorldTile.hpp"

namespace onion::voxel
{
	WorldTile::WorldTile(const std::string& name, const WorldInfos& worldInfos, Texture texture)
		: GuiElement(name), m_WorldInfos(worldInfos), m_LabelTitle(name + "_LabelTitle"),
		  m_LabelDescription(name + "_LabelDescription"), m_LabelDetails(name + "_LabelLastPlayed"),
		  m_SpriteThumbnail(name + "_Sprite", std::move(texture)),
		  m_SpriteJoin(name + "_SpriteJoin", s_SpriteJoinPathFromRessourcePack, Sprite::eOrigin::ResourcePack),
		  m_SpriteJoinHighlighted(name + "_SpriteJoinHighlighted",
								  s_SpriteJoinHighlightedPathFromRessourcePack,
								  Sprite::eOrigin::ResourcePack)
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
		m_SpriteJoin.Initialize();
		m_SpriteJoinHighlighted.Initialize();

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
			}
			else
			{
				m_IsSelected = true;
				EvtTileSelected.Trigger(*this);
			}

			m_LastClickTime = currentTime;
		}

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
			ColoredBackground::Options bgOptions;
			bgOptions.Position = m_Position;
			bgOptions.Size = m_Size;
			bgOptions.Color = glm::vec4(1.f, 1.f, 1.f, 1.f);
			bgOptions.ZOffset = -0.5f;
			ColoredBackground::Render(bgOptions);
		}

		// ---- Render Background ----
		if (m_IsSelected)
		{
			ColoredBackground::Options bgOptions;
			bgOptions.Position = m_Position;
			glm::vec2 sizeFloat = m_Size - glm::vec2(2 * borderThickness);
			bgOptions.Size = {static_cast<int>(round(sizeFloat.x)), static_cast<int>(round(sizeFloat.y))};
			bgOptions.Color = glm::vec4(0.f, 0.f, 0.f, 1.f);
			bgOptions.ZOffset = -0.4f;
			ColoredBackground::Render(bgOptions);
		}

		// ---- Render Thumbnail ----
		float zOffset = 0.1f;
		m_SpriteThumbnail.SetPosition(thumbnailPos);
		m_SpriteThumbnail.SetSize(thumbnailSize);
		m_SpriteThumbnail.SetZOffset(zOffset);
		m_SpriteThumbnail.Render();

		// ---- Render Join Sprite if Hovered ----
		if (isHovered)
		{
			// ---- Render Background ----
			glm::vec4 bgColor = glm::vec4(0.5f, 0.5f, 0.5f, 0.5f);
			zOffset += 0.05f;
			ColoredBackground::Options bgOptions;
			bgOptions.Position = thumbnailPos;
			bgOptions.Size = thumbnailSize;
			bgOptions.Color = bgColor;
			bgOptions.ZOffset = zOffset;
			ColoredBackground::Render(bgOptions);

			// ---- Render Join Sprite ----
			bool isThumbnailHovered = m_SpriteThumbnail.IsHovered();
			zOffset += 0.05f;
			Sprite& spriteToRender = isThumbnailHovered ? m_SpriteJoinHighlighted : m_SpriteJoin;
			spriteToRender.SetPosition(thumbnailPos);
			spriteToRender.SetSize(thumbnailSize);
			spriteToRender.SetZOffset(zOffset);
			spriteToRender.Render();

			// Detect Click on Join Sprite

			if (isThumbnailHovered && isMouseDown && !m_WasMouseDown)
			{
				EvtTileDoubleClicked.Trigger(*this);
			}
		}

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

		// ---- Update previous state ----
		m_WasMouseDown = isMouseDown;
		m_WasMouseHovering = isHovered;
	}

	void WorldTile::Delete()
	{
		m_LabelTitle.Delete();
		m_LabelDescription.Delete();
		m_LabelDetails.Delete();
		m_SpriteThumbnail.Delete();
		m_SpriteJoin.Delete();
		m_SpriteJoinHighlighted.Delete();

		SetInitState(false);
	}

	void WorldTile::ReloadTextures()
	{
		m_LabelTitle.ReloadTextures();
		m_LabelDescription.ReloadTextures();
		m_LabelDetails.ReloadTextures();
		m_SpriteJoin.ReloadTextures();
		m_SpriteJoinHighlighted.ReloadTextures();
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
