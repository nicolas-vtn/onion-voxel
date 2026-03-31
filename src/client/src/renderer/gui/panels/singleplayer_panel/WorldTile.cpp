#include "WorldTile.hpp"

namespace onion::voxel
{
	WorldTile::WorldTile(const std::string& name, Texture texture)
		: GuiElement(name), m_LabelTitle(name + "_LabelTitle"), m_LabelDescription(name + "_LabelDescription"),
		  m_LabelLastPlayed(name + "_LabelLastPlayed"), m_SpriteThumbnail(name + "_Sprite", std::move(texture))
	{
		SubscribeToControlEvents();

		m_LabelTitle.SetTextAlignment(Font::eTextAlignment::Left);
		m_LabelDescription.SetTextAlignment(Font::eTextAlignment::Left);
		m_LabelLastPlayed.SetTextAlignment(Font::eTextAlignment::Left);
	}

	WorldTile::~WorldTile()
	{
		m_EventHandles.clear();
	}

	void WorldTile::Initialize()
	{
		m_LabelTitle.Initialize();
		m_LabelDescription.Initialize();
		m_LabelLastPlayed.Initialize();
		m_SpriteThumbnail.Initialize();

		SetInitState(true);
	}

	void WorldTile::Render()
	{
		bool isHovered = IsMouseHovering();
	}

	void WorldTile::Delete()
	{
		m_LabelTitle.Delete();
		m_LabelDescription.Delete();
		m_LabelLastPlayed.Delete();
		m_SpriteThumbnail.Delete();

		SetInitState(false);
	}

	void WorldTile::ReloadTextures()
	{
		m_LabelTitle.ReloadTextures();
		m_LabelDescription.ReloadTextures();
		m_LabelLastPlayed.ReloadTextures();
		m_SpriteThumbnail.ReloadTextures();
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

	void WorldTile::SetSelected(bool selected)
	{
		m_IsSelected = selected;
	}

	bool WorldTile::IsSelected() const
	{
		return m_IsSelected;
	}

	void WorldTile::SetWorldName(const std::string& name)
	{
		m_WorldName = name;
	}

	std::string WorldTile::GetWorldName() const
	{
		return m_WorldName;
	}

	void WorldTile::SetWorldDescription(const std::string& description)
	{
		m_WorldDescription = description;
	}

	std::string WorldTile::GetWorldDescription() const
	{
		return m_WorldDescription;
	}

	void WorldTile::SetWorldLastPlayed(const DateTime& lastPlayed)
	{
		m_WorldLastPlayed = lastPlayed;
	}

	DateTime WorldTile::GetWorldLastPlayed() const
	{
		return m_WorldLastPlayed;
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

	void WorldTile::SubscribeToControlEvents() {}
} // namespace onion::voxel
