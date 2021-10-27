//1.0 release on 27/10/2021 by Carloschi7

//This document implements a simple text editor built using the pixelGameEngine.h header
//Link: https://github.com/OneLoneCoder/olcPixelGameEngine/blob/master/olcPixelGameEngine.h
//This program allows the user to:
//1) Perform basic text editing(many advanced modern IDE features may be missing)
//2) Load files using their filepath
//3) Saving files

#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

class TextEditor : public olc::PixelGameEngine
{
public:
	using tuple_type = std::tuple<olc::vi2d, olc::vi2d, int32_t>;

	TextEditor()
	{
		// Name your application
		sAppName = "Text Editor";
	}
private:
	std::vector<std::string> m_Lines;
	std::string m_FileLines;
	int32_t m_Offset;
	int32_t m_LineSpacing;
	int32_t m_LetterSpacing;
	olc::vi2d m_Cursor;
	olc::vi2d m_FileSelectionCursor;

	//std::tuple pointer used to store the information used to draw the selection
	//every frame
	tuple_type* m_SelectionData = nullptr;
	std::string m_Selection;

	float fKeyUpdateRate;
	float fElapsedTimeBetweenIterations;

	int32_t m_ButtonWidth;
	int32_t m_HeadlineHeight;

	int32_t m_SideBarWidth;
	int32_t m_SideBarHeight;
	int32_t m_PrevMouseY;
	float fSideBarY;

	std::string m_CurrentFilePath;

	enum class Mode
	{
		NORMAL_WRITING = 0,
		FILE_SELECTION,
		SAVE_CONFIRMATION
	} m_ProgramMode;

	static constexpr uint32_t m_ValidityStages = 10;
	//Storing key states to obtain the behaviour of regular typing.
	//98 total keys with a grand total of m_ValidityStages stages to surpass
	//in order to be able to give hold input
	bool m_LocalKeyStates[m_ValidityStages][98];

private://IDE functionalities sorted by type

	//----------------------- ARROW KEYS --------------------------------
	void ArrowKeysForCursor(int32_t& line_selected)
	{
		//Cursor movement with arrow keys
		if (IsKeyValid(olc::Key::RIGHT) && line_selected < m_Lines.size())
		{
			if (line_selected != m_Lines.size() - 1 || m_Lines.back().size() != m_Cursor.x / m_LetterSpacing)
				m_Cursor.x += m_LetterSpacing;

			//End of line
			if (m_Cursor.x / m_LetterSpacing > m_Lines[line_selected].size()
				&& line_selected != m_Lines.size() - 1)
			{
				m_Cursor.y += m_LineSpacing;
				m_Cursor.x = 0;
				line_selected++;
			}
		}
		if (IsKeyValid(olc::Key::LEFT))
		{
			if (line_selected != 0 || m_Cursor.x != 0)
				m_Cursor.x -= m_LetterSpacing;

			//Going back to the last character of the prev line
			if (m_Cursor.x < 0 && line_selected > 0)
			{
				m_Cursor.y -= m_LineSpacing;
				m_Cursor.x = m_Lines[line_selected - 1].size() * m_LetterSpacing;
				line_selected--;
			}
		}
		if (IsKeyValid(olc::Key::UP) && line_selected != 0)
		{
			m_Cursor.y -= m_LineSpacing;

			if (line_selected > 0 && m_Cursor.x / m_LetterSpacing > m_Lines[line_selected - 1].size())
			{
				m_Cursor.x = m_Lines[line_selected - 1].size() * m_LetterSpacing;
			}
			line_selected--;
		}
		if (IsKeyValid(olc::Key::DOWN) && line_selected < m_Lines.size() - 1)
		{
			m_Cursor.y += m_LineSpacing;

			if (line_selected < m_Lines.size() - 1 && m_Cursor.x / m_LetterSpacing > m_Lines[line_selected + 1].size())
			{
				m_Cursor.x = m_Lines[line_selected + 1].size() * m_LetterSpacing;
			}

			line_selected++;
		}
	}



	//--------------------- SPECIAL KEYS -------------------------
	void SpecialKeys(int32_t& line_selected)
	{
		if (IsKeyValid(olc::Key::BACK))
		{
			//Input detected when there is a selection will delete everything in
			//the selection and then break the loop
			if (ParseSelection()) return;
			if (m_Cursor.x == 0 && line_selected == 0)
			{
				//DO NOTHING
			}
			else if (m_Cursor.x == 0 && line_selected > 0)
			{
				m_Cursor.x = m_Lines[line_selected - 1].size() * m_LetterSpacing;
				m_Cursor.y -= m_LineSpacing;

				m_Lines[line_selected - 1] += m_Lines[line_selected];
				m_Lines.erase(m_Lines.begin() + line_selected);
				line_selected--;
			}
			else
			{
				m_Lines[line_selected].erase(m_Cursor.x / m_LetterSpacing - 1, 1);
				m_Cursor.x -= m_LetterSpacing;
			}
		}

		if (IsKeyValid(olc::Key::DEL))
		{
			if (ParseSelection()) return;
			if (line_selected == m_Lines.size() - 1 &&
				m_Cursor.x / m_LetterSpacing == m_Lines[line_selected].size())
			{
				//DO NOTHING
			}
			else if (m_Cursor.x / m_LetterSpacing == m_Lines[line_selected].size())
			{
				m_Lines[line_selected] += m_Lines[line_selected + 1];
				m_Lines.erase(m_Lines.begin() + line_selected + 1);
			}
			else
			{
				m_Lines[line_selected].erase(m_Cursor.x / m_LetterSpacing, 1);
			}
		}

		if (IsKeyValid(olc::Key::ENTER))
		{
			if (ParseSelection()) return;
			m_Lines.insert(m_Lines.begin() + line_selected + 1, "");
			m_Lines[line_selected + 1] = m_Lines[line_selected].substr(m_Cursor.x / m_LetterSpacing);

			m_Lines[line_selected].erase(m_Cursor.x / m_LetterSpacing);

			line_selected++;
			m_Cursor.x = 0;
			m_Cursor.y += m_LineSpacing;
		}

		if (IsKeyValid(olc::Key::SPACE))
		{
			if (ParseSelection()) return;
			m_Lines[line_selected].insert(m_Cursor.x / m_LetterSpacing, " ");
			m_Cursor.x += m_LetterSpacing;
		}

		if (IsKeyValid(olc::Key::TAB))
		{
			if (ParseSelection()) return;
			m_Lines[line_selected].insert(m_Cursor.x / m_LetterSpacing, "\x9");
			m_Cursor.x += m_LetterSpacing;
		}

		if (IsKeyValid(olc::Key::EQUALS))
		{
			if (ParseSelection()) return;

			if (Shift() && GetKey(olc::Key::NONE).bHeld) //Used for ALT-GR key
				m_Lines[line_selected].insert(m_Cursor.x / m_LetterSpacing, "}");
			else if (GetKey(olc::Key::NONE).bHeld)
				m_Lines[line_selected].insert(m_Cursor.x / m_LetterSpacing, "]");
			else if (Shift())
				m_Lines[line_selected].insert(m_Cursor.x / m_LetterSpacing, "*");
			else
				m_Lines[line_selected].insert(m_Cursor.x / m_LetterSpacing, "+");

			m_Cursor.x += m_LetterSpacing;
		}

		for (uint32_t key = olc::Key::OEM_1; key <= olc::Key::OEM_7; key++)
		{
			if (IsKeyValid((olc::Key)key))
			{
				if (ParseSelection()) return;
				std::string ret;
				ret += SpecialSymbolMapping((olc::Key)key);
				m_Lines[line_selected].insert(m_Cursor.x / m_LetterSpacing, ret);
				m_Cursor.x += m_LetterSpacing;
			}
		}

	}


	//----------------------- PUNCTUATION KEYS ---------------------------
	void PunctuationKeys(std::string& str, olc::vi2d& cursor)
	{
		if (IsKeyValid(olc::Key::COMMA))
		{
			if (ParseSelection()) return;
			str.insert(cursor.x / m_LetterSpacing,
				Shift() ? ";" : ",");
			cursor.x += m_LetterSpacing;
		}

		if (IsKeyValid(olc::Key::MINUS))
		{
			if (ParseSelection()) return;
			str.insert(cursor.x / m_LetterSpacing,
				Shift() ? "_" : "-");
			cursor.x += m_LetterSpacing;
		}

		if (IsKeyValid(olc::Key::PERIOD))
		{
			if (ParseSelection()) return;
			str.insert(cursor.x / m_LetterSpacing,
				Shift() ? ":" : ".");
			cursor.x += m_LetterSpacing;
		}
	}



	//----------------------- NUMBER KEYS --------------------------------
	void NumberKeys(std::string& str, olc::vi2d& cursor)
	{
		for (uint32_t number = olc::Key::K0; number <= olc::Key::K9; number++)
		{
			if (IsKeyValid((olc::Key)number))
			{
				if (ParseSelection()) return;

				std::string ins, capins;
				ins += '0' + (number - (uint8_t)olc::K0);
				capins += NumberSymbolMapping((olc::Key)number);

				str.insert(cursor.x / m_LetterSpacing,
					Shift() ? capins : ins);
				cursor.x += m_LetterSpacing;
			}
		}
	}


	//----------------------- A TO Z KEYS --------------------------------
	void AZKeys(std::string& str, olc::vi2d& cursor)
	{
		if (GetKey(olc::Key::CTRL).bHeld) return;

		for (uint32_t letter = olc::A; letter <= olc::Z; letter++)
		{
			if (IsKeyValid((olc::Key)letter))
			{
				if (ParseSelection()) return;

				std::string ins, capins;
				ins += 'a' + (letter - (uint8_t)olc::A);
				capins += 'A' + (letter - (uint8_t)olc::A);

				//The GetKeyState function is used only to retrieve
				//the caps lock key state
				str.insert(cursor.x / m_LetterSpacing,
					Shift() ^ ((GetKeyState(VK_CAPITAL) & 0x0001) != 0) ? capins : ins);

				cursor.x += m_LetterSpacing;
			}
		}
	}



	//-------------------- INPUT FOR FILE SELECTION ----------------------
	void FileSelectionInput()
	{
		AZKeys(m_FileLines, m_FileSelectionCursor);
		NumberKeys(m_FileLines, m_FileSelectionCursor);
		PunctuationKeys(m_FileLines, m_FileSelectionCursor);

		if (IsKeyValid(olc::Key::BACK) && m_FileSelectionCursor.x > 0)
		{
			m_FileLines.erase(m_FileSelectionCursor.x / m_LetterSpacing - 1, 1);
			m_FileSelectionCursor.x -= m_LetterSpacing;
		}
		if (IsKeyValid(olc::Key::SPACE))
		{
			m_FileLines.insert(m_FileSelectionCursor.x / m_LetterSpacing, " ");
			m_FileSelectionCursor.x += m_LetterSpacing;
		}
		if (IsKeyValid(olc::Key::RIGHT) &&
			m_FileSelectionCursor.x / m_LetterSpacing < m_FileLines.size())
		{
			m_FileSelectionCursor.x += m_LetterSpacing;
		}
		if (IsKeyValid(olc::Key::LEFT) &&
			m_FileSelectionCursor.x > 0)
		{
			m_FileSelectionCursor.x -= m_LetterSpacing;
		}
	}

public:

	//-------------------------------- APPLICATION SETUP -----------------------------------
	bool OnUserCreate() override
	{
		// Called once at the start, so create things here
		m_LineSpacing = 10;
		m_LetterSpacing = 8;

		m_Cursor = { 0, m_HeadlineHeight };
		m_FileSelectionCursor = { 0, m_LineSpacing };

		fElapsedTimeBetweenIterations = 0.0f;
		fKeyUpdateRate = 0.03f;
		m_ProgramMode = Mode::NORMAL_WRITING;
		m_Offset = m_HeadlineHeight;

		m_ButtonWidth = 82;
		m_HeadlineHeight = 20;

		m_SideBarWidth = 10;
		m_SideBarHeight = 60;
		m_PrevMouseY = 0;
		fSideBarY = m_HeadlineHeight;
		m_CurrentFilePath = "src/main.cpp";

		LoadFile(m_CurrentFilePath);

		return true;
	}






	//------------------------------------ EVERY FRAME --------------------------------------
	bool OnUserUpdate(float fElapsedTime) override
	{
		Clear(olc::BLACK);

		if (m_ProgramMode == Mode::NORMAL_WRITING)
		{
			//Screen Scrolling
			int32_t ScrollingSpeed = 3;

			float fAvailableSpace = (float)ScreenHeight() - (float)m_SideBarHeight;
			if (GetMouseWheel() > 0 && m_Offset < m_HeadlineHeight)
			{
				m_Offset += m_LineSpacing * ScrollingSpeed;
				m_Cursor.y += m_LineSpacing * ScrollingSpeed;

				//this is the result of the process of reversing the mathematical function found
				//in the OffsetFromSidebar function. It is not perfect but accurate enough to work fine
				fSideBarY = (int32_t)((1.0f + 1.0f / ((float)m_Lines.size() / 40)) * (float)m_HeadlineHeight -
					m_Offset / ((float)m_Lines.size() / 40));
				ClampSideBar();

			}
			//Not scolling further down from the document
			if (GetMouseWheel() < 0 && std::abs(m_Offset) < m_Lines.size() * m_LineSpacing)
			{
				m_Offset -= m_LineSpacing * ScrollingSpeed;
				m_Cursor.y -= m_LineSpacing * ScrollingSpeed;
				fSideBarY = (int32_t)((1.0f + 1.0f / ((float)m_Lines.size() / 40)) * (float)m_HeadlineHeight -
					m_Offset / ((float)m_Lines.size() / 40));
				ClampSideBar();
			}

			//Cursor handling
			int32_t mouse_selected = (GetMouseY() - m_Offset) / m_LineSpacing;
			int32_t line_selected = (m_Cursor.y - m_Offset) / m_LineSpacing;
			static int32_t second_line_selected = 0;

			if (GetKey(olc::Key::ESCAPE).bPressed) return false;

			//Mouse selection
			if (GetMouse(0).bPressed && mouse_selected < m_Lines.size()
				&& GetMouseY() > m_HeadlineHeight && GetMouseX() < ScreenWidth() - m_SideBarWidth)
			{
				SetupCursor(m_Cursor, line_selected);
				delete_if(m_SelectionData);
			}
			//Multiple Selection
			if (GetMouse(0).bHeld && mouse_selected < m_Lines.size() && GetMouseY() > m_HeadlineHeight
				&& GetMouseX() < ScreenWidth() - m_SideBarWidth && m_PrevMouseY == 0)
			{
				olc::vi2d tempCursor;
				SetupCursor(tempCursor, second_line_selected);

				//If the two cursor are in the same spot, there is obviously
				//no point to create a selection
				if (m_Cursor != tempCursor)
				{
					if (!m_SelectionData) m_SelectionData = new tuple_type;
					if (line_selected == second_line_selected)
					{
						if (tempCursor.x < m_Cursor.x)
						{
							*m_SelectionData = std::make_tuple(tempCursor, m_Cursor, m_Offset);
						}
						else
						{
							*m_SelectionData = std::make_tuple(m_Cursor, tempCursor, m_Offset);
						}
					}
					else
					{
						olc::vi2d upper, lower;
						if (tempCursor.y < m_Cursor.y)
						{
							upper = tempCursor;
							lower = m_Cursor;
						}
						else
						{
							upper = m_Cursor;
							lower = tempCursor;
						}
						*m_SelectionData = std::make_tuple(upper, lower, m_Offset);
					}
				}
			}
			//Copy & paste commands
			if (m_SelectionData && GetKey(olc::Key::CTRL).bHeld &&
				GetKey(olc::Key::C).bPressed)
			{
				m_Selection.clear();

#if (_CRT_HAS_CXX17 == 1)
				//CPP17 required for structure binding
				auto [first, second, old_offset] = *m_SelectionData;
#else
				//Old method
				olc::vi2d first = std::get<0>(*m_SelectionData);
				olc::vi2d second = std::get<1>(*m_SelectionData);
				int32_t old_offset = std::get<2>(*m_SelectionData);
#endif

				StoreSelection(first, second);
			}
			if (GetKey(olc::Key::CTRL).bHeld && GetKey(olc::Key::V).bPressed)
			{
				WriteSelection(line_selected);
			}

			//Handling arrow keys(see function)
			ArrowKeysForCursor(line_selected);

			//User Input + refreshing the current line
			SpecialKeys(line_selected);
			line_selected = (m_Cursor.y - m_Offset) / m_LineSpacing;
			PunctuationKeys(m_Lines[line_selected], m_Cursor);
			line_selected = (m_Cursor.y - m_Offset) / m_LineSpacing;
			NumberKeys(m_Lines[line_selected], m_Cursor);
			line_selected = (m_Cursor.y - m_Offset) / m_LineSpacing;
			AZKeys(m_Lines[line_selected], m_Cursor);
			line_selected = (m_Cursor.y - m_Offset) / m_LineSpacing;

			//Bar selection
			//Load file button
			if (GetMouse(0).bPressed && GetMouseX() > 0 && GetMouseX() < m_ButtonWidth
				&& GetMouseY() > 0 && GetMouseY() < m_HeadlineHeight - 2)
			{
				delete_if(m_SelectionData);
				m_ProgramMode = Mode::FILE_SELECTION;
			}
			//Save file button
			if (GetMouse(0).bPressed && GetMouseX() > m_ButtonWidth && GetMouseX() < m_ButtonWidth * 2
				&& GetMouseY() > 0 && GetMouseY() < m_HeadlineHeight - 2)
			{
				delete_if(m_SelectionData);
				m_ProgramMode = Mode::SAVE_CONFIRMATION;
			}

			//Sidebar input
			int32_t nSideBarY = (int32_t)fSideBarY;
			if (GetMouse(0).bHeld && ((GetMouseX() > ScreenWidth() - m_SideBarWidth &&
				GetMouseY() > nSideBarY && GetMouseY() < nSideBarY + m_SideBarHeight)
				|| m_PrevMouseY != 0))
			{
				if (m_PrevMouseY == 0) m_PrevMouseY = GetMouseY();

				//Colliding with screen boundaries
				if (GetMouseY() < m_PrevMouseY && nSideBarY > m_HeadlineHeight)
				{
					fSideBarY += (float)GetMouseY() - (float)m_PrevMouseY;
					m_PrevMouseY = GetMouseY();
				}
				if (GetMouseY() > m_PrevMouseY && nSideBarY < ScreenHeight() - m_SideBarHeight)
				{
					fSideBarY += (float)GetMouseY() - (float)m_PrevMouseY;
					m_PrevMouseY = GetMouseY();
				}

				OffsetFromSidebar();
			}
			else
			{
				m_PrevMouseY = 0;
			}

			//Drawing text
			for (int32_t line = 0; line < m_Lines.size(); line++)
			{
				olc::vi2d DrawOffset = { 0, line * m_LineSpacing + m_Offset };
				if (DrawOffset.y >= 0 && DrawOffset.y < ScreenHeight())
					DrawString(DrawOffset, m_Lines[line]);
			}

			//Draw Cursor
			DrawRect(m_Cursor, { 1, m_LineSpacing });
			//Draw utility bar and writings
			FillRect({ 0,0 }, { ScreenWidth(), m_HeadlineHeight - 2 }, olc::GREY);
			DrawString({ 5,5 }, "Load file", olc::BLACK);
			DrawString({ m_ButtonWidth + 5, 5 }, "Save file", olc::BLACK);
			//Draw sidebar
			FillRect({ ScreenWidth() - m_SideBarWidth, nSideBarY },
				{ m_SideBarWidth, m_SideBarHeight }, olc::GREY);

			if (m_SelectionData)
			{
#if (_CRT_HAS_CXX17 == 1)
				auto [first, second, old_offset] = *m_SelectionData;
#else
				olc::vi2d first = std::get<0>(*m_SelectionData);
				olc::vi2d second = std::get<1>(*m_SelectionData);
				int32_t old_offset = std::get<2>(*m_SelectionData);
#endif
				first.y += m_Offset - old_offset;
				second.y += m_Offset - old_offset;

				if (first.y == second.y)
				{
					DrawRect(
						{ first.x, first.y },
						{ second.x - first.x, m_LineSpacing },
						olc::YELLOW
					);
				}
				else
				{
					DrawSelection(first, second);
				}
			}
		}
		else if (m_ProgramMode == Mode::FILE_SELECTION)
		{
			//Input
			FileSelectionInput();

			DrawString({ 0,0 }, "Insert File Path:");
			//Draw cursor
			DrawRect(m_FileSelectionCursor, { 1, m_LineSpacing });
			//Draw filepath
			DrawString({ 0, m_LineSpacing }, m_FileLines);

			if (GetKey(olc::Key::ESCAPE).bPressed)
			{
				m_ProgramMode = Mode::NORMAL_WRITING;
				m_FileLines.clear();
				m_FileSelectionCursor = { 0, m_LineSpacing };
			}
			if (GetKey(olc::Key::ENTER).bPressed)
			{
				m_ProgramMode = Mode::NORMAL_WRITING;
				LoadFile(m_FileLines);
				m_FileLines.clear();
				m_FileSelectionCursor = { 0, m_LineSpacing };
			}
		}
		else if (m_ProgramMode == Mode::SAVE_CONFIRMATION)
		{
			DrawString({ 0,0 }, "Do you really want to save the current file? [Y/N]");
			if (GetKey(olc::Key::Y).bPressed)
			{
				SaveFile();
				m_ProgramMode = Mode::NORMAL_WRITING;
			}
			if (GetKey(olc::Key::N).bPressed)
			{
				m_ProgramMode = Mode::NORMAL_WRITING;
			}
		}

		//Updating key states
		if (fElapsedTimeBetweenIterations > fKeyUpdateRate)
		{
			fElapsedTimeBetweenIterations = 0.0f;
			UpdateKeyStates();
		}

		fElapsedTimeBetweenIterations += fElapsedTime;

		return true;
	}

private: //Utilities
	bool LoadFile(const std::string& path)
	{
		std::ifstream file(path);
		if (!file.is_open())
			return false;

		m_Cursor = { 0, m_HeadlineHeight };
		m_Offset = m_HeadlineHeight;
		m_CurrentFilePath = path;
		m_Lines.clear();

		std::string line;
		while (std::getline(file, line))
		{
			m_Lines.push_back(line);
		}

		file.close();
		return true;
	}

	void SaveFile()
	{
		std::ofstream file(m_CurrentFilePath);
		file.clear();
		file.seekp(std::ios::beg);

		for (std::string str : m_Lines)
		{
			//Need to use \n when writing files
			str += "\n";
			file.write(str.c_str(), str.size());
		}

		file.close();
	}

	void SetupCursor(olc::vi2d& cursor, int32_t& line_selected)
	{
		cursor = { GetMouseX(), GetMouseY() };
		while (cursor.x % m_LetterSpacing != 0) cursor.x--;
		while (cursor.y % m_LineSpacing != 0) cursor.y--;

		//Handling selection over the utility bar
		line_selected = (cursor.y - m_Offset) / m_LineSpacing;

		//Lining up with the selected line
		if (cursor.x / m_LetterSpacing > m_Lines[line_selected].size())
		{
			cursor.x = m_Lines[line_selected].size() * m_LetterSpacing;
		}
	}

	void DrawSelection(const olc::vi2d& upper, const olc::vi2d& lower)
	{
		for (int32_t i = upper.y; i <= lower.y; i += m_LineSpacing)
		{
			int32_t cur_line_selected = (i - m_Offset) / m_LineSpacing;
			if (i == upper.y)
			{
				DrawRect(
					{ upper.x, upper.y },
					{ std::abs(upper.x - (int32_t)m_Lines[cur_line_selected].size() * m_LetterSpacing), m_LineSpacing },
					olc::YELLOW
				);
			}
			else if (i == lower.y)
			{
				DrawRect(
					{ 0, lower.y },
					{ lower.x, m_LineSpacing },
					olc::YELLOW
				);
			}
			else
			{
				DrawRect({ 0, i },
					{ (int32_t)m_Lines[cur_line_selected].size() * m_LetterSpacing, m_LineSpacing },
					olc::YELLOW);
			}
		}
	}

	void StoreSelection(const olc::vi2d& upper, const olc::vi2d& lower)
	{
		int32_t line_selected = (upper.y - m_Offset) / m_LineSpacing;

		if (upper.y == lower.y)
		{
			m_Selection = m_Lines[line_selected].substr(
				upper.x / m_LetterSpacing,
				lower.x / m_LetterSpacing - upper.x / m_LetterSpacing
			);
		}
		else
		{
			for (int32_t i = upper.y, j = 0; i <= lower.y; i += m_LineSpacing, j++)
			{
				if (i == upper.y)
				{
					m_Selection += m_Lines[line_selected].substr(upper.x / m_LetterSpacing);
					m_Selection += "\n";
				}
				else if (i == lower.y)
				{
					m_Selection += m_Lines[line_selected + j].substr(0, lower.x / m_LetterSpacing);
					m_Selection += "\n";
				}
				else
				{
					m_Selection += m_Lines[line_selected + j];
					m_Selection += "\n";
				}
			}
		}
	}

	void WriteSelection(int32_t& line_selected)
	{
		if (m_Selection.empty()) return;

		for (auto x : m_Selection)
		{
			if (x == '\n')
			{
				m_Lines.insert(m_Lines.begin() + line_selected + 1,
					m_Lines[line_selected].substr(m_Cursor.x / m_LetterSpacing));

				m_Lines[line_selected].erase(m_Cursor.x / m_LetterSpacing,
					m_Lines[line_selected].size() - m_Cursor.x / m_LetterSpacing);

				m_Cursor.x = 0;
				m_Cursor.y += m_LineSpacing;
				line_selected++;
			}
			else
			{
				m_Lines[line_selected].insert(
					m_Lines[line_selected].begin() + m_Cursor.x / m_LetterSpacing, x);

				m_Cursor.x += m_LetterSpacing;
			}
		}
	}

	void DeleteSelection(const olc::vi2d& upper, const olc::vi2d& lower)
	{
		int32_t line_selected = (upper.y - m_Offset) / m_LineSpacing;

		if (upper.y == lower.y)
		{
			m_Lines[line_selected].erase(
				upper.x / m_LetterSpacing,
				(lower.x - upper.x) / m_LetterSpacing);

			if (m_Cursor == lower) m_Cursor.x -= (lower.x - upper.x);
		}
		else
		{
			int32_t j = (lower.y - upper.y) / m_LineSpacing;
			for (int32_t i = lower.y; i >= upper.y; i -= m_LineSpacing, j--)
			{
				if (i == upper.y)
				{
					m_Lines[line_selected].erase(upper.x / m_LetterSpacing);
				}
				else if (i == lower.y)
				{
					m_Lines[line_selected + j].erase(0, lower.x / m_LetterSpacing);
				}
				else
				{
					m_Lines.erase(m_Lines.begin() + line_selected + j);
				}
			}

			m_Cursor = upper;
		}
	}

	bool ParseSelection()
	{
		if (m_SelectionData)
		{
			DeleteSelection(std::get<0>(*m_SelectionData),
				std::get<1>(*m_SelectionData));

			delete_if(m_SelectionData);
			return true;
		}
		return false;
	}

	void OffsetFromSidebar()
	{
		//Clamping bar according to borders
		int32_t nSideBarY = (int32_t)fSideBarY;
		ClampSideBar();

		//Calculating approximate bar movement according to document length
		int32_t diff = ((nSideBarY - m_HeadlineHeight) * (int32_t)m_Lines.size() / 40 - m_HeadlineHeight);
		int32_t old_offset = m_Offset;
		m_Offset = -diff;
		while (std::abs(m_Offset) % 10 != 0) m_Offset--;

		m_Cursor.y += m_Offset - old_offset;
	}

	void ClampSideBar()
	{
		if (fSideBarY < m_HeadlineHeight)
			fSideBarY = (float)m_HeadlineHeight;
		if (fSideBarY > (float)(ScreenHeight() - m_SideBarHeight))
			fSideBarY = (float)(ScreenHeight() - m_SideBarHeight);
	}

	void UpdateKeyStates()
	{
		for (uint32_t i = 1; i < 98; i++)
		{
			if (GetKey((olc::Key)i).bHeld)
			{
				for (uint32_t k = 0; k < m_ValidityStages; k++)
				{
					if (!m_LocalKeyStates[k][i])
					{
						m_LocalKeyStates[k][i] = true;
						break;
					}
				}
			}
			else
			{
				for (uint32_t k = 0; k < m_ValidityStages; k++)
				{
					m_LocalKeyStates[k][i] = false;
				}
			}
		}
	}

	bool IsKeyValid(olc::Key key)
	{
		//When the key is pressed for the first time, it returns true,
		//then it returns false while still being pressed because the m_ValidityStages
		//stages need to activate.
		//After that it returns alternatively true and false so that the consecutive letter
		//input does not get too fast

		if (GetKey(key).bPressed) return true;

		for (uint32_t i = 0; i < m_ValidityStages; i++)
			if (!m_LocalKeyStates[i][key])
				return false;

		m_LocalKeyStates[m_ValidityStages - 1][key] = false;
		return true;
	}

	char NumberSymbolMapping(olc::Key num_key)
	{
		//Italian keyboard mapping
		switch (num_key)
		{
		case olc::Key::K1: return '!';
		case olc::Key::K2: return '"';
		case olc::Key::K3: return '£'; //NOT WORKING
		case olc::Key::K4: return '$';
		case olc::Key::K5: return '%';
		case olc::Key::K6: return '&';
		case olc::Key::K7: return '/';
		case olc::Key::K8: return '(';
		case olc::Key::K9: return ')';
		case olc::Key::K0: return '=';
		}

		return '\0';
	}

	char SpecialSymbolMapping(olc::Key special_key)
	{
		//Italian keyboard mapping
		if (Shift() && GetKey(olc::NONE).bHeld)
		{
			switch (special_key)
			{
			case olc::Key::OEM_1: return '{';
			}
		}
		if (GetKey(olc::NONE).bHeld)
		{
			switch (special_key)
			{
			case olc::Key::OEM_1: return '[';
			case olc::Key::OEM_3: return '@';
			case olc::Key::OEM_7: return '#';
			}
		}
		if (Shift())
		{
			switch (special_key)
			{
			case olc::Key::OEM_1: return 'é';
			case olc::Key::OEM_2: return '§';
			case olc::Key::OEM_3: return 'ç';
			case olc::Key::OEM_4: return '?';
			case olc::Key::OEM_5: return '|';
			case olc::Key::OEM_6: return '^';
			case olc::Key::OEM_7: return '°';
			}
		}

		switch (special_key)
		{
		case olc::Key::OEM_1: return 'è';
		case olc::Key::OEM_2: return 'ù';
		case olc::Key::OEM_3: return 'ò';
		case olc::Key::OEM_4: return '\'';
		case olc::Key::OEM_5: return '\\';
		case olc::Key::OEM_6: return 'ì';
		case olc::Key::OEM_7: return 'à';
		}

		return '\0';
	}

	void delete_if(tuple_type*& ptr)
	{
		if (ptr)
		{
			delete ptr;
			ptr = nullptr;
		}
	}

	bool Shift() const
	{
		return GetKey(olc::Key::SHIFT).bHeld;
	}

};


int main()
{
	TextEditor demo;
	if (demo.Construct(800, 500, 2, 2))
		demo.Start();

	return 0;
}
