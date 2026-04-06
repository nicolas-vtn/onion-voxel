#include "DemoTextsPanel.hpp"

#include <renderer/gui/LayoutHelper.hpp>

namespace onion::voxel
{
	DemoTextsPanel::DemoTextsPanel(const std::string& name)
		: GuiElement(name), m_BackButton("Back Button"), m_LabelAllStyles_1("All Styles Label 1"),
		  m_LabelAllStyles_2("All Styles Label 2")
	{
		SubscribeToControlEvents();

		for (int i = 0; i < static_cast<int>(LabelType::Count); i++)
		{
			LabelType labelType = static_cast<LabelType>(i);
			std::string labelText;
			switch (labelType)
			{
				case LabelType::Regular:
					labelText = "Regular";
					break;
				case LabelType::Color0:
					labelText = Font::ColorToString(Font::eColor::Black);
					break;
				case LabelType::Color1:
					labelText = Font::ColorToString(Font::eColor::DarkBlue);
					break;
				case LabelType::Color2:
					labelText = Font::ColorToString(Font::eColor::DarkGreen);
					break;
				case LabelType::Color3:
					labelText = Font::ColorToString(Font::eColor::DarkAqua);
					break;
				case LabelType::Color4:
					labelText = Font::ColorToString(Font::eColor::DarkRed);
					break;
				case LabelType::Color5:
					labelText = Font::ColorToString(Font::eColor::DarkPurple);
					break;
				case LabelType::Color6:
					labelText = Font::ColorToString(Font::eColor::Gold);
					break;
				case LabelType::Color7:
					labelText = Font::ColorToString(Font::eColor::Gray);
					break;
				case LabelType::Color8:
					labelText = Font::ColorToString(Font::eColor::DarkGray);
					break;
				case LabelType::Color9:
					labelText = Font::ColorToString(Font::eColor::Blue);
					break;
				case LabelType::ColorA:
					labelText = Font::ColorToString(Font::eColor::Green);
					break;
				case LabelType::ColorB:
					labelText = Font::ColorToString(Font::eColor::Aqua);
					break;
				case LabelType::ColorC:
					labelText = Font::ColorToString(Font::eColor::Red);
					break;
				case LabelType::ColorD:
					labelText = Font::ColorToString(Font::eColor::LightPurple);
					break;
				case LabelType::ColorE:
					labelText = Font::ColorToString(Font::eColor::Yellow);
					break;
				case LabelType::ColorF:
					labelText = Font::ColorToString(Font::eColor::White);
					break;
				case LabelType::Bold:
					labelText = "bold";
					break;
				case LabelType::Strikethrough:
					labelText = "strikethrough";
					break;
				case LabelType::Underline:
					labelText = "underline";
					break;
				case LabelType::Italic:
					labelText = "italic";
					break;
				default:
					labelText = "Minecraft";
					break;
			}

			std::unique_ptr<Label> label = std::make_unique<Label>(labelText);
			label->SetText(labelText);
			label->SetTextAlignment(Font::eTextAlignment::Center);
			m_Labels.push_back(std::move(label));
		}

		m_LabelAllStyles_1.SetText(m_AllTextStylesLabelText_1);
		m_LabelAllStyles_1.SetTextAlignment(Font::eTextAlignment::Center);

		m_LabelAllStyles_2.SetText(m_AllTextStylesLabelText_2);
		m_LabelAllStyles_2.SetTextAlignment(Font::eTextAlignment::Center);

		m_BackButton.SetText("Done");
	}

	DemoTextsPanel::~DemoTextsPanel()
	{
		m_EventHandles.clear();
	}

	void DemoTextsPanel::Render()
	{
		if (s_IsBackPressed)
		{
			Handle_BackButtonClick(m_BackButton);
			return;
		}

		// ----- Create a Layout -----
		int numColumns = 4;
		int numRows = 6;
		int tableWidth = static_cast<int>(s_ScreenWidth * 0.8f);
		int tableHeight = static_cast<int>(s_ScreenHeight * 0.5f);
		TableLayout layout = LayoutHelper::CreateTableLayout(numRows, numColumns, {tableWidth, tableHeight}, 0, 0);
		int topY = (s_ScreenHeight - tableHeight) / 4;
		int leftX = (s_ScreenWidth - tableWidth) / 2;
		glm::ivec2 tableTopLeftCorner = {leftX, topY};

		// ----- Render Labels -----
		for (int i = 0; i < static_cast<int>(m_Labels.size()); i++)
		{
			//LabelType labelType = static_cast<LabelType>(i);
			std::unique_ptr<Label>& label = m_Labels[i];
			glm::ivec2 cellCenter = layout.GetElementPosition(i / numColumns, i % numColumns);
			label->SetPosition(tableTopLeftCorner + cellCenter);
			label->SetTextHeight(s_TextHeight);
			label->Render();
		}

		// ----- Render "All Styles" Label -----
		glm::vec2 allStylesLabelPos = {
			s_ScreenWidth * 0.5f,										// Centered horizontally
			tableTopLeftCorner.y + tableHeight + s_ScreenHeight * 0.05f // Below the table with some margin
		};
		//m_LabelAllStyles.SetPosition(allStylesLabelPos);
		//m_LabelAllStyles.SetTextHeight(s_TextHeight / 2);
		//m_LabelAllStyles.Render();

		const std::string& txt1 = m_AllTextStylesLabelText_1;
		const std::string& txt2 = m_AllTextStylesLabelText_2;
		Font::eTextAlignment alignment = Font::eTextAlignment::Center;

		s_TextFont.RenderText(txt1, alignment, allStylesLabelPos, s_TextHeight, 0.8f, 0.f);
		s_TextFont.RenderText(
			txt2, alignment, allStylesLabelPos + glm::vec2(0, s_TextHeight * 1.2f), s_TextHeight, 0.8f, 0.f);

		//void RenderText(const std::string& text,
		//		eTextAlignment alignment,
		//		const glm::vec2& position,
		//		float textHeightPx,
		//		float zOffset = 0.0f,
		//		float rotationDegrees = 0.0f,
		//		bool renderShadow = true,
		//		const glm::vec4& backgroundColor = glm::vec4(0.0f));

		// ----- Render Back Button -----
		float doneButtonYPosRatio = 948.f / 1009.f;
		float doneButtonWidth = (800.f / 1920.f) * s_ScreenWidth;
		glm::vec2 buttonPos = {s_ScreenWidth * 0.5f, s_ScreenHeight * doneButtonYPosRatio};
		m_BackButton.SetPosition(buttonPos);
		m_BackButton.SetSize({doneButtonWidth, s_ControlHeight});
		m_BackButton.Render();
	}

	void DemoTextsPanel::Initialize()
	{
		m_BackButton.Initialize();

		for (auto& label : m_Labels)
		{
			label->Initialize();
		}

		SetInitState(true);
	}

	void DemoTextsPanel::Delete()
	{
		m_BackButton.Delete();

		for (auto& label : m_Labels)
		{
			label->Delete();
		}

		SetDeletedState(true);
	}

	void DemoTextsPanel::ReloadTextures()
	{
		m_BackButton.ReloadTextures();

		for (auto& label : m_Labels)
		{
			label->ReloadTextures();
		}
	}

	void DemoTextsPanel::SubscribeToControlEvents()
	{
		m_EventHandles.push_back(
			m_BackButton.OnClick.Subscribe([this](const Button& button) { Handle_BackButtonClick(button); }));
	}

	void DemoTextsPanel::Handle_BackButtonClick(const Button& button)
	{
		(void) button; // Unused parameter
		EvtRequestBackNavigation.Trigger(this);
	}
} // namespace onion::voxel
