#include "DemoTextsPanel.hpp"

#include <renderer/gui/LayoutHelper.hpp>

namespace onion::voxel
{
	DemoTextsPanel::DemoTextsPanel(const std::string& name)
		: GuiElement(name), m_BackButton("Back Button"), m_LabelAllStyles_1("All Styles Label 1"),
		  m_LabelAllStyles_2("All Styles Label 2"), m_LabelUnicodes("Unicodes Label")
	{
		SubscribeToControlEvents();

		for (int i = 0; i < static_cast<int>(LabelType::Count); i++)
		{
			LabelType labelType = static_cast<LabelType>(i);
			Font::eColor color = Font::eColor::White;
			Font::TextFormat format{};
			std::string labelText;
			switch (labelType)
			{
				case LabelType::Regular:
					labelText = "Regular";
					break;
				case LabelType::Color0:
					color = Font::eColor::Black;
					labelText = Font::ColorToString(color);
					break;
				case LabelType::Color1:
					color = Font::eColor::DarkBlue;
					labelText = Font::ColorToString(color);
					break;
				case LabelType::Color2:
					color = Font::eColor::DarkGreen;
					labelText = Font::ColorToString(color);
					break;
				case LabelType::Color3:
					color = Font::eColor::DarkAqua;
					labelText = Font::ColorToString(color);
					break;
				case LabelType::Color4:
					color = Font::eColor::DarkRed;
					labelText = Font::ColorToString(color);
					break;
				case LabelType::Color5:
					color = Font::eColor::DarkPurple;
					labelText = Font::ColorToString(color);
					break;
				case LabelType::Color6:
					color = Font::eColor::Gold;
					labelText = Font::ColorToString(color);
					break;
				case LabelType::Color7:
					color = Font::eColor::Gray;
					labelText = Font::ColorToString(color);
					break;
				case LabelType::Color8:
					color = Font::eColor::DarkGray;
					labelText = Font::ColorToString(color);
					break;
				case LabelType::Color9:
					color = Font::eColor::Blue;
					labelText = Font::ColorToString(color);
					break;
				case LabelType::ColorA:
					color = Font::eColor::Green;
					labelText = Font::ColorToString(color);
					break;
				case LabelType::ColorB:
					color = Font::eColor::Aqua;
					labelText = Font::ColorToString(color);
					break;
				case LabelType::ColorC:
					color = Font::eColor::Red;
					labelText = Font::ColorToString(color);
					break;
				case LabelType::ColorD:
					color = Font::eColor::LightPurple;
					labelText = Font::ColorToString(color);
					break;
				case LabelType::ColorE:
					color = Font::eColor::Yellow;
					labelText = Font::ColorToString(color);
					break;
				case LabelType::ColorF:
					color = Font::eColor::White;
					labelText = Font::ColorToString(color);
					break;
				case LabelType::Bold:
					format.bold = true;
					labelText = "bold";
					break;
				case LabelType::Strikethrough:
					format.strikethrough = true;
					labelText = "strikethrough";
					break;
				case LabelType::Underline:
					format.underline = true;
					labelText = "underline";
					break;
				case LabelType::Italic:
					format.italic = true;
					labelText = "italic";
					break;
				default:
					labelText = "Minecraft";
					break;
			}

			std::unique_ptr<Label> label = std::make_unique<Label>(labelText);
			label->SetText(labelText);
			label->SetTextColor(color);
			label->SetTextFormat(format);
			//label->SetRotationDegrees(-15.f); // Rotate all labels by 15 degrees for demonstration
			label->SetTextAlignment(Font::eTextAlignment::Center);
			m_Labels.push_back(std::move(label));
		}

		m_LabelAllStyles_1.SetText(m_AllTextStylesLabelText_1);
		m_LabelAllStyles_1.SetTextAlignment(Font::eTextAlignment::Center);

		m_LabelAllStyles_2.SetText(m_AllTextStylesLabelText_2);
		m_LabelAllStyles_2.SetTextAlignment(Font::eTextAlignment::Center);

		std::string unicodeTextExample = "Unicodes : àâäçèéêëîïôöùûü";
		m_LabelUnicodes.SetText(unicodeTextExample);
		m_LabelUnicodes.SetTextAlignment(Font::eTextAlignment::Center);

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

		const std::u32string& txt1 = m_AllTextStylesLabelText_1;
		const std::u32string& txt2 = m_AllTextStylesLabelText_2;
		Font::eTextAlignment alignment = Font::eTextAlignment::Center;

		glm::vec2 demoTxtPos = allStylesLabelPos + glm::vec2(0, s_TextHeight * 1.2f);
		s_TextFont.RenderText(txt1, alignment, allStylesLabelPos, s_TextHeight, 0.8f, 0.f);
		s_TextFont.RenderText(txt2, alignment, demoTxtPos, s_TextHeight, 0.8f, 0.f);

		// ----- Render Unicodes Label -----
		glm::ivec2 unicodesLabelPos = demoTxtPos + glm::vec2(0, s_TextHeight * 1.2f);
		m_LabelUnicodes.SetPosition(unicodesLabelPos);
		m_LabelUnicodes.SetTextHeight(s_TextHeight);
		m_LabelUnicodes.Render();

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
		m_LabelUnicodes.Initialize();

		for (auto& label : m_Labels)
		{
			label->Initialize();
		}

		SetInitState(true);
	}

	void DemoTextsPanel::Delete()
	{
		m_BackButton.Delete();
		m_LabelUnicodes.Delete();

		for (auto& label : m_Labels)
		{
			label->Delete();
		}

		SetDeletedState(true);
	}

	void DemoTextsPanel::ReloadTextures()
	{
		m_BackButton.ReloadTextures();
		m_LabelUnicodes.ReloadTextures();

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
