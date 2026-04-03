#include "TextField.hpp"

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <renderer/debug_draws/DebugDraws.hpp>

namespace onion::voxel
{
	TextField::TextField(const std::string& name)
		: GuiElement(name), m_Label(name + "_TextFieldLabel"),
		  m_NineSliceSprite_TextField(name + "_9Slice_TextField", s_SpritePathFromGui),
		  m_NineSliceSprite_TextFieldHighlighted(name + "_9Slice_TextField_Highlighted",
												 s_SpritePathFromGui_Highlighted)
	{
		SubscribeToSpriteEvents();

		m_Label.SetTextAlignment(Font::eTextAlignment::Left);
		m_Label.SetZOffset(0.8f);
	}

	TextField::~TextField()
	{
		m_EventHandles.clear();
	}

	void TextField::Initialize()
	{
		SubscribeToInputsManagerEvents();
		RegisterInputsToInputsManager();

		m_NineSliceSprite_TextField.Initialize();
		m_NineSliceSprite_TextFieldHighlighted.Initialize();
		m_Label.Initialize();

		SetInitState(true);
	}

	void TextField::Render()
	{
		if (!s_InputsSnapshot)
		{
			std::cerr << "Error: TextField::Render() called without a valid InputsSnapshot." << std::endl;
			return;
		}

		// Pulls events
		m_NineSliceSprite_TextField.PullEvents();

		// Handle inputs
		Handle_CharInputs();
		Handle_KeyInputs();

		// DEBUG
		//if (EngineContext::Get().ShowDebugMenus)
		//	RenderImGuiDebug();

		// Calculate text position
		glm::ivec2 size = GetSize();
		glm::ivec2 pos = GetPosition();
		glm::ivec2 textPos = pos - glm::ivec2(size.x / m_TextStartXratio, 0);

		// Add trailing '_' or Render Cursor
		std::string drawnText = m_Text;
		bool visible = fmod(glfwGetTime(), 0.8) < 0.5;
		if (visible && m_IsActive && !m_ReadOnly)
		{
			if (m_CursorPosition == m_Text.size())
			{
				drawnText.push_back('_');
			}
			else
			{
				std::string textBeforeCursor = m_Text.substr(0, m_CursorPosition);
				glm::vec2 textBeforeCursorSize =
					s_TextFont.MeasureText(textBeforeCursor, GetSize().y * m_TextScaleFactor);
				glm::ivec2 cursorPos = textPos + glm::ivec2(textBeforeCursorSize.x, 0);

				float cursorHeight = size.y * m_CursorHeightRatio;
				glm::ivec2 bottomCursor = cursorPos + glm::ivec2(0, cursorHeight / 2.f);
				glm::ivec2 topCursor = cursorPos - glm::ivec2(0, cursorHeight / 2.f);

				DebugDraws::DrawScreenLine_Pixels(bottomCursor, topCursor, glm::vec4(s_TextColor, 1.f), m_CursorWidth);
			}
		}

		if (m_IsActive)
			m_NineSliceSprite_TextFieldHighlighted.Render();
		else
			m_NineSliceSprite_TextField.Render();

		// ----- Render Label -----
		float textHeight = size.y * m_TextScaleFactor;
		m_Label.SetTextHeight(textHeight);
		if (m_Text.empty() && !m_IsActive)
		{
			// PLACEHOLDER TEXT

			m_Label.SetText(m_PlaceholderText);
			m_Label.SetTextColor(s_PlaceholderTextColor);
			m_Label.SetPosition(textPos);
			m_Label.Render();
		}
		else
		{
			// NORMAL TEXT

			if (m_SelectionStart != SIZE_MAX)
			{
				// SELECTION

				// Calculates real selection start and end positions
				size_t selectionStart = std::min(m_SelectionStart, m_CursorPosition);
				size_t selectionEnd = std::max(m_SelectionStart, m_CursorPosition);

				// Render 3 parts: text before selection, selected text, text after selection
				std::string textBeforeSelection = drawnText.substr(0, selectionStart);
				std::string selectedText = drawnText.substr(selectionStart, selectionEnd - selectionStart);
				std::string textAfterSelection = drawnText.substr(selectionEnd);

				// Measure text sizes for positioning
				glm::vec2 textBeforeSelectionSize = s_TextFont.MeasureText(textBeforeSelection, textHeight);
				glm::vec2 selectedTextSize = s_TextFont.MeasureText(selectedText, textHeight);
				glm::vec2 textAfterSelectionSize = s_TextFont.MeasureText(textAfterSelection, textHeight);

				// Calculate positions
				glm::ivec2 selectedTextPos = textPos + glm::ivec2(textBeforeSelectionSize.x, 0);
				glm::ivec2 textAfterSelectionPos = selectedTextPos + glm::ivec2(selectedTextSize.x, 0);

				// Render text before selection
				m_Label.SetText(textBeforeSelection);
				m_Label.SetTextColor(s_TextColor);
				m_Label.SetPosition(textPos);
				m_Label.Render();

				// Render selected text with highlight
				m_Label.SetText(selectedText);
				m_Label.SetTextColor(s_SelectedTextColor);
				m_Label.SetShadowColor(s_SelectedTextShadowColor);
				m_Label.SetBackgroundColor(glm::vec4(1.f));
				m_Label.SetPosition(selectedTextPos);
				m_Label.Render();

				// Render text after selection
				m_Label.SetText(textAfterSelection);
				m_Label.SetTextColor(s_TextColor);
				m_Label.ResetShadowColor(); // Reset shadow color for non-selected text
				m_Label.SetPosition(textAfterSelectionPos);
				m_Label.SetBackgroundColor(glm::vec4(0.f)); // Disable background for non-selected text
				m_Label.Render();
			}
			else
			{
				// NO SELECTION
				m_Label.SetText(drawnText);
				m_Label.SetTextColor(s_TextColor);
				m_Label.SetPosition(textPos);
				m_Label.Render();
			}
		}
	}

	void TextField::Delete()
	{
		m_NineSliceSprite_TextField.Delete();
		m_NineSliceSprite_TextFieldHighlighted.Delete();
		m_Label.Delete();

		SetDeletedState(true);
	}

	void TextField::ReloadTextures()
	{
		m_NineSliceSprite_TextField.ReloadTextures();
		m_NineSliceSprite_TextFieldHighlighted.ReloadTextures();
		m_Label.ReloadTextures();
	}

	void TextField::SetText(const std::string& text)
	{
		m_Text = text;

		// Reset States
		m_CursorPosition = std::min(m_CursorPosition, m_Text.size());
		if (m_SelectionStart != SIZE_MAX)
		{
			m_SelectionStart = std::min(m_SelectionStart, m_Text.size());
			if (m_SelectionStart == m_CursorPosition)
				m_SelectionStart = SIZE_MAX; // No selection if selection start and cursor position are the same
		}
	}

	std::string TextField::GetText() const
	{
		return m_Text;
	}

	void TextField::SetPlaceholderText(const std::string& placeholderText)
	{
		m_PlaceholderText = placeholderText;
	}

	std::string TextField::GetPlaceholderText() const
	{
		return m_PlaceholderText;
	}

	void TextField::SetSize(const glm::ivec2& size)
	{
		if (size == m_Size)
			return;

		m_Size = size;
		m_NineSliceSprite_TextField.SetSize(size);
		m_NineSliceSprite_TextFieldHighlighted.SetSize(size);
	}

	glm::ivec2 TextField::GetSize() const
	{
		return m_Size;
	}

	void TextField::SetPosition(const glm::ivec2& pos)
	{
		if (pos == m_Position)
			return;

		m_Position = pos;
		m_NineSliceSprite_TextField.SetPosition(pos);
		m_NineSliceSprite_TextFieldHighlighted.SetPosition(pos);
	}

	glm::ivec2 TextField::GetPosition() const
	{
		return m_Position;
	}

	void TextField::SetVisibility(const Visibility& visibility)
	{
		GuiElement::SetVisibility(visibility);

		m_NineSliceSprite_TextField.SetVisibility(visibility);
		m_NineSliceSprite_TextFieldHighlighted.SetVisibility(visibility);
	}

	bool TextField::IsReadOnly() const
	{
		return m_ReadOnly;
	}

	void TextField::SetReadOnly(bool readOnly)
	{
		m_ReadOnly = readOnly;
	}

	void TextField::SubscribeToSpriteEvents()
	{
		m_EventHandles.push_back(m_NineSliceSprite_TextField.OnMouseDown.Subscribe([this](const NineSliceSprite& sprite)
																				   { Handle_MouseDown(sprite); }));

		m_EventHandles.push_back(m_NineSliceSprite_TextField.OnMouseUp.Subscribe([this](const NineSliceSprite& sprite)
																				 { Handle_MouseUp(sprite); }));

		m_EventHandles.push_back(m_NineSliceSprite_TextField.OnClick.Subscribe([this](const NineSliceSprite& sprite)
																			   { Handle_SpriteClick(sprite); }));

		m_EventHandles.push_back(m_NineSliceSprite_TextField.OnHoverEnter.Subscribe(
			[this](const NineSliceSprite& sprite) { Handle_SpriteHoverEnter(sprite); }));

		m_EventHandles.push_back(m_NineSliceSprite_TextField.OnHoverLeave.Subscribe(
			[this](const NineSliceSprite& sprite) { Handle_SpriteHoverLeave(sprite); }));
	}

	void TextField::SubscribeToInputsManagerEvents()
	{
		const auto& InputsManager = EngineContext::Get().Inputs;
		m_HandleCharInput = InputsManager->EventCharInput.Subscribe([this](const unsigned int& codepoint)
																	{ Handle_CharInput(codepoint); });
	}

	void TextField::RegisterInputsToInputsManager()
	{
		const auto& InputsManager = EngineContext::Get().Inputs;

		InputConfig keyRepeatConfig = InputConfig(true, 0.5f, 0.03f, 0.5f);
		m_InputId_Backspace = InputsManager->RegisterInput(Key::Backspace, keyRepeatConfig);
		m_InputId_Enter = InputsManager->RegisterInput(Key::Enter, keyRepeatConfig);
		m_InputId_KpEnter = InputsManager->RegisterInput(Key::KPEnter, keyRepeatConfig);
		m_InputId_Escape = InputsManager->RegisterInput(Key::Escape, keyRepeatConfig);
		m_InputId_Left = InputsManager->RegisterInput(Key::Left, keyRepeatConfig);
		m_InputId_Right = InputsManager->RegisterInput(Key::Right, keyRepeatConfig);
		m_InputId_Delete = InputsManager->RegisterInput(Key::Delete, keyRepeatConfig);
		m_InputId_C = InputsManager->RegisterInput(Key::C, keyRepeatConfig);
		m_InputId_V = InputsManager->RegisterInput(Key::V, keyRepeatConfig);
		m_InputId_X = InputsManager->RegisterInput(Key::X, keyRepeatConfig);
		m_InputId_A = InputsManager->RegisterInput(Key::Q, keyRepeatConfig);

		InputConfig noKeyRepeatConfig = InputConfig(false);
		m_InputId_Ctrl = InputsManager->RegisterInput(Key::LeftControl, noKeyRepeatConfig);
		m_InputId_Click = InputsManager->RegisterInput(Key::MouseButtonLeft, noKeyRepeatConfig);
		m_InputId_Shift = InputsManager->RegisterInput(Key::LeftShift, noKeyRepeatConfig);

		InputConfig doubleClickConfig = InputConfig(true, 0.5f, 0.5f, 0.2f);
		m_InputId_DoubleClick = InputsManager->RegisterInput(Key::MouseButtonLeft, doubleClickConfig);
	}

	void TextField::Handle_MouseDown(const NineSliceSprite& sprite)
	{
		(void) sprite;

		m_IsActive = true;
		m_IsSelecting = true;

		m_TextAtActivation = m_Text; // Store the text at the moment of activation for potential cancellation

		// Sets the selection start position based on the mouse click position
		bool shiftPressed = s_InputsSnapshot->GetKeyState(m_InputId_Shift).IsPressed;
		if (!shiftPressed)
		{
			int mouseX = static_cast<int>(std::round(s_InputsSnapshot->Mouse.Xpos));
			m_SelectionStart = GetCursorPositionFromMouseX(mouseX);
		}
	}

	void TextField::Handle_MouseUp(const NineSliceSprite& sprite)
	{
		(void) sprite;

		if (!HasSelection())
		{
			ResetSelection();
		}

		m_IsSelecting = false;
	}

	void TextField::Handle_SpriteClick(const NineSliceSprite& sprite)
	{
		(void) sprite;
	}

	void TextField::Handle_SpriteHoverEnter(const NineSliceSprite& sprite)
	{
		(void) sprite;
		GuiElement::RequestCursorStyleChange.Trigger(CursorStyle::IBeam);
	}

	void TextField::Handle_SpriteHoverLeave(const NineSliceSprite& sprite)
	{
		(void) sprite;
		GuiElement::RequestCursorStyleChange.Trigger(CursorStyle::Arrow);
	}

	void TextField::Handle_CharInput(const unsigned int& codepoint)
	{
		m_LastCharInput = codepoint;
	}

	void TextField::Handle_KeyInputs()
	{
		if (!m_IsActive || m_ReadOnly)
			return;

		const auto& snapshot = s_InputsSnapshot;
		const auto& inputsManager = EngineContext::Get().Inputs;

		bool backspacePressed = snapshot->GetKeyState(m_InputId_Backspace).IsPressed;
		bool enterPressed =
			snapshot->GetKeyState(m_InputId_Enter).IsPressed || snapshot->GetKeyState(m_InputId_KpEnter).IsPressed;
		bool escapePressed = snapshot->GetKeyState(m_InputId_Escape).IsPressed;
		bool leftPressed = snapshot->GetKeyState(m_InputId_Left).IsPressed;
		bool rightPressed = snapshot->GetKeyState(m_InputId_Right).IsPressed;
		bool ctrlPressed = snapshot->GetKeyState(m_InputId_Ctrl).IsPressed;
		bool deletePressed = snapshot->GetKeyState(m_InputId_Delete).IsPressed;
		bool aPressed = snapshot->GetKeyState(m_InputId_A).IsPressed;
		bool cPressed = snapshot->GetKeyState(m_InputId_C).IsPressed;
		bool vPressed = snapshot->GetKeyState(m_InputId_V).IsPressed;
		bool xPressed = snapshot->GetKeyState(m_InputId_X).IsPressed;
		bool clickPressed = snapshot->GetKeyState(m_InputId_Click).IsPressed;
		bool doubleClick = snapshot->GetKeyState(m_InputId_DoubleClick).IsDoublePressed;
		bool shiftPressed = snapshot->GetKeyState(m_InputId_Shift).IsPressed;

		glm::ivec2 mousePos{snapshot->Mouse.Xpos, snapshot->Mouse.Ypos};

		if (enterPressed)
		{
			ValidateText();
			return;
		}

		if (escapePressed)
		{
			// Revert to the text at activation if Escape is pressed
			m_Text = m_TextAtActivation;
			ValidateText();
			return;
		}

		if (!m_WasClickDown && clickPressed && !m_NineSliceSprite_TextField.IsHovered())
		{
			// Validate text and deactivate if there's a mouse click outside the text field while it's active
			m_IsActive = false;
			ValidateText();
			return;
		}
		m_WasClickDown = clickPressed;

		if (aPressed && ctrlPressed)
		{
			// Ctrl + A: Select all text
			m_SelectionStart = 0;
			m_CursorPosition = m_Text.size();
		}

		if (cPressed && ctrlPressed)
		{
			// Ctrl + C: Copy selected text to clipboard
			if (HasSelection())
			{
				std::string selectedText = GetSelectedText();
				inputsManager->SetClipboardText(selectedText);
			}
		}

		if (vPressed && ctrlPressed)
		{
			// Ctrl + V: Paste from clipboard
			std::string clipboardText = inputsManager->GetClipboardText();

			// If there's a selection, remove it before pasting the clipboard text
			if (HasSelection())
			{
				auto [selectionStart, selectionEnd] = GetSelectionRange();
				m_Text.erase(m_Text.begin() + selectionStart, m_Text.begin() + selectionEnd);
				m_CursorPosition = selectionStart;
				ResetSelection();
			}

			// Insert the clipboard text at the current cursor position
			m_Text.insert(m_Text.begin() + m_CursorPosition, clipboardText.begin(), clipboardText.end());
			m_CursorPosition += clipboardText.size();
		}

		if (xPressed && ctrlPressed)
		{
			// Ctrl + X: Cut selected text to clipboard
			if (HasSelection())
			{
				std::string selectedText = GetSelectedText();

				inputsManager->SetClipboardText(selectedText);

				auto [selectionStart, selectionEnd] = GetSelectionRange();
				m_Text.erase(m_Text.begin() + selectionStart, m_Text.begin() + selectionEnd);
				m_CursorPosition = selectionStart;
				ResetSelection();
			}
		}

		// Reset selection if Shift is not pressed and there's no mouse click
		if (!shiftPressed && !clickPressed && !HasSelection())
		{
			ResetSelection();
		}

		if (m_Text.empty())
		{
			return;
		}

		// Initiate selection if Shift is pressed and there's no active selection.
		if (shiftPressed && !clickPressed)
		{
			if (!HasSelection())
			{
				m_SelectionStart = m_CursorPosition;
			}
		}

		// Update cursor position based on mouse click (For selection )
		if (clickPressed && m_IsSelecting && mousePos != m_DoubleClickPosition)
		{
			m_CursorPosition = GetCursorPositionFromMouseX(mousePos.x);
		}

		// Select the current word on double click
		if (doubleClick)
		{
			size_t wordStart = GetNextWordBoundary(m_Text, m_CursorPosition, eDirection::Left);
			size_t wordEnd = GetNextWordBoundary(m_Text, m_CursorPosition, eDirection::Right);
			m_SelectionStart = wordStart;
			m_CursorPosition = wordEnd;
			m_DoubleClickPosition = mousePos;
		}

		bool hasSelection = HasSelection();
		std::pair<size_t, size_t> selectionRange = GetSelectionRange();

		if (m_Text.empty())
		{
			ResetSelection();
			return;
		}
		else if (backspacePressed)
		{
			// If there's a selection, delete the selected text
			if (hasSelection)
			{
				m_Text.erase(m_Text.begin() + selectionRange.first, m_Text.begin() + selectionRange.second);
				m_CursorPosition = selectionRange.first;
				ResetSelection();
				return;
			}

			// Bugfix : (Shift + Backspace was selecting a char)
			if (shiftPressed)
			{
				ResetSelection();
			}

			// Ctrl + Backspace: delete the previous word
			if (ctrlPressed)
			{
				size_t lastWordBoundary = GetNextWordBoundary(m_Text, m_CursorPosition, eDirection::Left);
				m_Text.erase(m_Text.begin() + lastWordBoundary, m_Text.begin() + m_CursorPosition);
				m_CursorPosition = lastWordBoundary;
			}
			else if (m_CursorPosition > 0)
			{
				// Backspace: delete the character before the cursor position
				m_Text.erase(m_Text.begin() + m_CursorPosition - 1);
				m_CursorPosition--;
			}
		}
		else if (leftPressed)
		{
			// Reset selection if Shift is not pressed
			if (!shiftPressed)
			{
				ResetSelection();
			}

			// Ctrl + Left: move cursor to the left of the previous word
			if (ctrlPressed)
			{
				size_t lastWordBoundary = GetNextWordBoundary(m_Text, m_CursorPosition, eDirection::Left);
				m_CursorPosition = lastWordBoundary;
			}
			else if (m_CursorPosition > 0)
			{
				// Left: move cursor one position to the left
				m_CursorPosition--;
			}
		}
		else if (rightPressed)
		{
			// Reset selection if Shift is not pressed
			if (!shiftPressed)
			{
				ResetSelection();
			}

			// Ctrl + Right: move cursor to the right of the next word
			if (ctrlPressed)
			{
				size_t nextWordBoundary = GetNextWordBoundary(m_Text, m_CursorPosition, eDirection::Right);
				m_CursorPosition = nextWordBoundary;
			}
			else if (m_CursorPosition < m_Text.size())
			{
				// Right: move cursor one position to the right
				m_CursorPosition++;
			}
		}
		else if (deletePressed)
		{
			// If there's a selection, delete the selected text
			if (hasSelection)
			{
				m_Text.erase(m_Text.begin() + selectionRange.first, m_Text.begin() + selectionRange.second);
				m_CursorPosition = selectionRange.first;
				ResetSelection();
				return;
			}

			if (m_CursorPosition < m_Text.size())
			{
				m_Text.erase(m_Text.begin() + m_CursorPosition);
			}
		}
	}

	void TextField::Handle_CharInputs()
	{
		unsigned int codepoint = m_LastCharInput;
		m_LastCharInput = 0; // Reset after handling

		if (!m_IsActive || m_ReadOnly || codepoint == 0)
			return;

		if (HasSelection())
		{
			// If there's a selection, replace it with the new character
			auto [selectionStart, selectionEnd] = GetSelectionRange();
			m_Text.erase(m_Text.begin() + selectionStart, m_Text.begin() + selectionEnd);
			m_CursorPosition = selectionStart;
			ResetSelection();
		}

		bool shiftPressed = s_InputsSnapshot->GetKeyState(m_InputId_Shift).IsPressed;
		if (shiftPressed)
		{
			ResetSelection();
		}

		// Append the new character to the text
		m_Text.insert(m_Text.begin() + m_CursorPosition, static_cast<char>(codepoint));
		m_CursorPosition++;
	}

	void TextField::ValidateText()
	{
		m_IsActive = false;
		ResetSelection();
		OnTextChanged.Trigger(*this);
	}

	bool TextField::HasSelection() const
	{
		return m_SelectionStart != SIZE_MAX && m_SelectionStart != m_CursorPosition;
	}

	std::pair<size_t, size_t> TextField::GetSelectionRange() const
	{
		if (!HasSelection())
			return {0, 0};

		size_t selectionStart = std::min(m_SelectionStart, m_CursorPosition);
		size_t selectionEnd = std::max(m_SelectionStart, m_CursorPosition);

		return {selectionStart, selectionEnd};
	}

	std::string TextField::GetSelectedText() const
	{
		if (m_SelectionStart == SIZE_MAX || m_SelectionStart == m_CursorPosition)
			return "";

		size_t selectionStart = std::min(m_SelectionStart, m_CursorPosition);
		size_t selectionEnd = std::max(m_SelectionStart, m_CursorPosition);

		return m_Text.substr(selectionStart, selectionEnd - selectionStart);
	}

	void TextField::ResetSelection()
	{
		m_SelectionStart = SIZE_MAX;
		m_IsSelecting = false;
		m_DoubleClickPosition = glm::ivec2{0, 0};
	}

	size_t TextField::GetCursorPositionFromMouseX(int mouseX)
	{
		int startTextX = GetPosition().x - static_cast<int>(GetSize().x / m_TextStartXratio);

		float textHeight = GetSize().y * m_TextScaleFactor;
		std::string textToMeasure;

		// Iterate through the text character by character to find where the mouse click occurred
		for (size_t i = 0; i < m_Text.size(); i++)
		{
			textToMeasure += m_Text[i];
			float textWidth = s_TextFont.MeasureText(textToMeasure, textHeight).x;

			textWidth -= 5; // Magic number

			int textEndX = startTextX + static_cast<int>(textWidth);

			// If the mouse click is before the end of the current character, place the cursor here
			if (textEndX >= mouseX)
			{
				return i;
				break;
			}

			// If we reached the end of the text and the mouse is still to the right, move cursor to the end
			if (i == m_Text.size() - 1)
			{
				return m_Text.size();
			}
		}

		return 0; // Default to start if something goes wrong
	}

	size_t TextField::GetNextWordBoundary(const std::string& text, size_t cursorPosition, eDirection direction) const
	{
		if (direction == eDirection::Left)
		{
			if (cursorPosition == 0)
				return 0;

			size_t startWordPos = cursorPosition - 1;
			while (startWordPos > 0 && text[startWordPos] == ' ')
			{
				startWordPos--;
			}

			size_t lastSpacePos = text.find_last_of(' ', startWordPos);
			return lastSpacePos == std::string::npos ? 0 : lastSpacePos + 1;
		}
		else // direction == eDirection::Right
		{
			if (cursorPosition >= text.size())
				return text.size();

			size_t endWordPos = cursorPosition;
			while (endWordPos < text.size() && text[endWordPos] == ' ')
			{
				endWordPos++;
			}

			endWordPos = text.find_first_of(' ', endWordPos);

			// If there are spaces after the next word, skip them
			while (endWordPos < text.size() && text[endWordPos] == ' ')
			{
				endWordPos++;
			}

			return endWordPos == std::string::npos ? text.size() : endWordPos;
		}
	}

	void TextField::RenderImGuiDebug()
	{
		// Debug ImGui pannel
		ImGui::Begin(("TextField: " + GetName()).c_str());

		ImGui::Text("Hovered: %s", m_NineSliceSprite_TextField.IsHovered() ? "Yes" : "No");

		std::string text = GetText();
		if (ImGui::InputText("Text", &text))
		{
			SetText(text);
		}

		std::string placeholderText = GetPlaceholderText();
		if (ImGui::InputText("Placeholder Text", &placeholderText))
		{
			SetPlaceholderText(placeholderText);
		}

		ImGui::SliderFloat("Text Scale Factor", &m_TextScaleFactor, 0.1f, 1.f);

		bool isReadOnly = IsReadOnly();
		if (ImGui::Checkbox("Read Only", &isReadOnly))
		{
			SetReadOnly(isReadOnly);
		}

		glm::ivec2 position = GetPosition();
		if (ImGui::SliderInt2("Position", &position.x, 0, s_ScreenWidth))
		{
			SetPosition(position);
		}

		ImGui::Separator();

		ImGui::SliderInt("Cursor Width", &m_CursorWidth, 1, 10);
		ImGui::SliderFloat("Cursor Height Ratio", &m_CursorHeightRatio, 0.1f, 1.f);

		ImGui::Separator();

		ImGui::SliderInt("Selection Start", (int*) &m_SelectionStart, 0, static_cast<int>(text.size()));

		ImGui::Separator();

		glm::ivec2 size = GetSize();
		if (ImGui::SliderInt2("Size", &size.x, 20, 1500))
		{
			SetSize(size);
		}

		ImGui::End();
	}

} // namespace onion::voxel
