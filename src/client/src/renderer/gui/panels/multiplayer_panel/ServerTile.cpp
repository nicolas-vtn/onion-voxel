#include "ServerTile.hpp"

#include <renderer/gui/colored_background/ColoredBackground.hpp>

namespace onion::voxel
{
	ServerTile::ServerTile(const std::string& name, const ServerInfos& serverInfos)
		: GuiElement(name), m_ServerInfos(serverInfos), m_LabelName(name + "_LabelTitle"),
		  m_LabelDescription(name + "_LabelDescription"), m_LabelPlayerCount(name + "_LabelPlayerCount"),
		  m_UnreachableSprite(name + "_UnreachableSprite", s_UnreachableSpritePath, Sprite::eOrigin::ResourcePack),
		  m_PingSprites{Sprite(name + "_PingSprite1", s_PingSpritesPaths[0], Sprite::eOrigin::ResourcePack),
						Sprite(name + "_PingSprite2", s_PingSpritesPaths[1], Sprite::eOrigin::ResourcePack),
						Sprite(name + "_PingSprite3", s_PingSpritesPaths[2], Sprite::eOrigin::ResourcePack),
						Sprite(name + "_PingSprite4", s_PingSpritesPaths[3], Sprite::eOrigin::ResourcePack),
						Sprite(name + "_PingSprite5", s_PingSpritesPaths[4], Sprite::eOrigin::ResourcePack)},
		  m_PingingSprites{Sprite(name + "_PingingSprite1", s_PingingSpritesPaths[0], Sprite::eOrigin::ResourcePack),
						   Sprite(name + "_PingingSprite2", s_PingingSpritesPaths[1], Sprite::eOrigin::ResourcePack),
						   Sprite(name + "_PingingSprite3", s_PingingSpritesPaths[2], Sprite::eOrigin::ResourcePack),
						   Sprite(name + "_PingingSprite4", s_PingingSpritesPaths[3], Sprite::eOrigin::ResourcePack),
						   Sprite(name + "_PingingSprite5", s_PingingSpritesPaths[4], Sprite::eOrigin::ResourcePack)},
		  m_ThumbnailSprite(name + "_ThumbnailSprite", Texture())
	{
		m_LabelName.SetTextAlignment(Font::eTextAlignment::Left);
		m_LabelDescription.SetTextAlignment(Font::eTextAlignment::Left);
		m_LabelPlayerCount.SetTextAlignment(Font::eTextAlignment::Right);

		// Load texture if provided
		if (m_ServerInfos.IconPngData.size() > 0)
		{
			Texture thumbnailTexture("ThumbnailServer_" + m_ServerInfos.Name, m_ServerInfos.IconPngData);
			m_ThumbnailSprite.SwapTexture(std::move(thumbnailTexture));
		}
		else // Load default texture if none provided
		{
			std::filesystem::path defaultThumbnailPath =
				EngineContext::Get().Assets->GetTexturesDirectory() / "ServerThumbnail.png";
			m_ThumbnailSprite.SwapTexture(Texture(defaultThumbnailPath));
		}
	}

	ServerTile::~ServerTile()
	{
		m_EventHandles.clear();
	}

	void ServerTile::Render()
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
		const int posBorderLeft = static_cast<int>(round(m_Position.x - (m_Size.x / 2)));
		const glm::ivec2 thumbnailPos{
			posBorderLeft + (2 * borderThickness) + static_cast<int>(round(thumbnailSize.x / 2)), m_Position.y};

		const int topPosY = static_cast<int>(round(m_Position.y - (m_Size.y / 2)));

		// ---- Render Border ----
		if (m_IsSelected)
		{
			ColoredBackground::CenterOptions bgOptions;
			bgOptions.Position = m_Position;
			bgOptions.Size = m_Size;
			bgOptions.Color = glm::vec4(1.f, 1.f, 1.f, 1.f);
			bgOptions.ZOffset = -0.5f;
			ColoredBackground::Render(bgOptions);
		}

		// ---- Render Background ----
		if (m_IsSelected)
		{
			ColoredBackground::CenterOptions bgOptions;
			bgOptions.Position = m_Position;
			glm::vec2 sizeFloat = m_Size - glm::vec2(static_cast<float>(2 * borderThickness));
			bgOptions.Size = {static_cast<int>(round(sizeFloat.x)), static_cast<int>(round(sizeFloat.y))};
			bgOptions.Color = glm::vec4(0.f, 0.f, 0.f, 1.f);
			bgOptions.ZOffset = -0.4f;
			ColoredBackground::Render(bgOptions);
		}

		// ---- Render Thumbnail ----
		m_ThumbnailSprite.SetPosition(thumbnailPos);
		m_ThumbnailSprite.SetSize(thumbnailSize);
		m_ThumbnailSprite.Render();

		// ---- Render Server Name ----
		float textNamePosYratio = 28.f / 134.f;
		const int textNamePosY = static_cast<int>(round(topPosY + (textNamePosYratio * m_Size.y)));
		float textPosXratio = 143.f / 1211.f;
		const int textPosX = static_cast<int>(round(posBorderLeft + (textPosXratio * m_Size.x)));
		m_LabelName.SetPosition({textPosX, textNamePosY});
		m_LabelName.SetTextHeight(s_TextHeight);
		m_LabelName.SetText(m_ServerInfos.Name);
		m_LabelName.Render();

		// ---- Render Server Description ----
		float textDescrPosYratio = 80.f / 134.f;
		const int descriptionTextPosY = static_cast<int>(round(topPosY + (textDescrPosYratio * m_Size.y)));
		m_LabelDescription.SetPosition({textPosX, descriptionTextPosY});
		m_LabelDescription.SetTextHeight(s_TextHeight);
		m_LabelDescription.SetText(m_ServerInfos.Description);
		m_LabelDescription.Render();

		// ---- Render Ping ----
		// Select ping sprite based on ping value (0-1000ms mapped to 5 sprites)
		Sprite* pingSprite = nullptr;

		if (m_ServerInfos.Ping < 0)
		{
			pingSprite = &m_UnreachableSprite;
		}
		else
		{
			int pingIndex = 4 - std::clamp(m_ServerInfos.Ping / 200, 0, 4);
			pingSprite = &m_PingSprites[pingIndex];
		}
		float pingSpritePosXratio = (1172.f) / 1211.f;
		int pingSpritePosX = static_cast<int>(round(posBorderLeft + (pingSpritePosXratio * m_Size.x)));
		float pingSizeRatio = 32.f / 1009.f;
		int pingSize = static_cast<int>(round(pingSizeRatio * s_ScreenHeight));
		glm::ivec2 pingSpritePos{pingSpritePosX, textNamePosY};
		pingSprite->SetPosition(pingSpritePos);
		pingSprite->SetSize({pingSize, pingSize});
		pingSprite->Render();

		// ---- Render Player Count ----
		textPosXratio = 1040.f / 1211.f;
		const int playerCountTextPosX = pingSpritePosX - pingSize;
		m_LabelPlayerCount.SetPosition({playerCountTextPosX, textNamePosY});
		m_LabelPlayerCount.SetTextHeight(s_TextHeight);
		std::u32string playerCountText = U"\u00A77" + Utf8ToUtf32(std::to_string(m_ServerInfos.PlayerCount));
		playerCountText += U"\u00A78/";
		playerCountText += U"\u00A77" + Utf8ToUtf32(std::to_string(m_ServerInfos.MaxPlayerCount));
		m_LabelPlayerCount.SetText(playerCountText);
		m_LabelPlayerCount.Render();

		// ---- Update previous state ----
		m_WasMouseDown = isMouseDown;
		m_WasMouseHovering = isHovered;
	}

	void ServerTile::Initialize()
	{
		m_LabelName.Initialize();
		m_LabelDescription.Initialize();
		m_LabelPlayerCount.Initialize();
		m_ThumbnailSprite.Initialize();
		m_UnreachableSprite.Initialize();

		for (auto& pingSprite : m_PingSprites)
		{
			pingSprite.Initialize();
		}

		for (auto& pingingSprite : m_PingingSprites)
		{
			pingingSprite.Initialize();
		}

		SetInitState(true);
	}

	void ServerTile::Delete()
	{
		m_LabelName.Delete();
		m_LabelDescription.Delete();
		m_LabelPlayerCount.Delete();
		m_ThumbnailSprite.Delete();
		m_UnreachableSprite.Delete();

		for (auto& pingSprite : m_PingSprites)
		{
			pingSprite.Delete();
		}

		for (auto& pingingSprite : m_PingingSprites)
		{
			pingingSprite.Delete();
		}

		SetDeletedState(true);
	}

	void ServerTile::ReloadTextures()
	{
		m_LabelName.ReloadTextures();
		m_LabelDescription.ReloadTextures();
		m_LabelPlayerCount.ReloadTextures();
		m_UnreachableSprite.ReloadTextures();

		for (auto& pingSprite : m_PingSprites)
		{
			pingSprite.ReloadTextures();
		}

		for (auto& pingingSprite : m_PingingSprites)
		{
			pingingSprite.ReloadTextures();
		}
	}

	void ServerTile::SetSize(const glm::vec2& size)
	{
		m_Size = size;
	}

	glm::vec2 ServerTile::GetSize() const
	{
		return m_Size;
	}

	void ServerTile::SetPosition(const glm::vec2& pos)
	{
		m_Position = pos;
	}

	glm::vec2 ServerTile::GetPosition() const
	{
		return m_Position;
	}

	void ServerTile::SetServerInfos(const ServerInfos& serverInfos)
	{
		m_ServerInfos = serverInfos;

		Texture thumbnailTexture("ThumbnailServer_" + m_ServerInfos.Name, m_ServerInfos.IconPngData);
		m_ThumbnailSprite.SwapTexture(std::move(thumbnailTexture));
	}

	const ServerInfos ServerTile::GetServerInfos() const
	{
		return m_ServerInfos;
	}

	void ServerTile::SetSelected(bool selected)
	{
		m_IsSelected = selected;
	}

	bool ServerTile::IsSelected() const
	{
		return m_IsSelected;
	}

	void ServerTile::SetThumbnailTexture(Texture& texture) {}

	bool ServerTile::IsMouseHovering() const
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

	void ServerTile::SubscribeToControlEvents() {}

} // namespace onion::voxel
