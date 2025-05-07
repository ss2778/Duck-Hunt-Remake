#include "baseTypes.h"
#include "input.h"
#include "application.h"
#include "framework.h"


#include "levelmgr.h"
#include "objmgr.h"


static void _gameInit();
static void _gameShutdown();
static void _gameDraw();
static void _gameUpdate(uint32_t milliseconds);

static LevelDef _levelDefs[] = {
	{
		{{0, 0}, {960, 1024}},	// fieldBounds
		0x00ff0000,					// fieldColor
		2							// numDucks
	}
};
static Level* _curLevel = NULL;
static bool _wiiInput = true;

/// @brief Program Entry Point (WinMain)
/// @param hInstance 
/// @param hPrevInstance 
/// @param lpCmdLine 
/// @param nCmdShow 
/// @return 
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
	const char GAME_NAME[] = "Duck Hunt";

	Application* app = appNew(hInstance, GAME_NAME, _gameDraw, _gameUpdate);
	appSetWidth(app, 960);
	appSetHeight(app, 1024);
	if (app != NULL)
	{
		GLWindow* window = fwInitWindow(app);
		if (window != NULL)
		{
			_gameInit();

			bool running = true;
			while (running)
			{
				running = fwUpdateWindow(window);
			}

			_gameShutdown();
			fwShutdownWindow(window);
		}

		appDelete(app);
	}
}

/// @brief Initialize code to run at application startup
static void _gameInit()
{
	const uint32_t MAX_OBJECTS = 500;
	objMgrInit(MAX_OBJECTS);
	levelMgrInit();
	_curLevel = levelMgrLoad(&_levelDefs[0]);
}

/// @brief Cleanup the game and free up any allocated resources
static void _gameShutdown()
{
	levelMgrUnload(_curLevel);

	levelMgrShutdown();
	objMgrShutdown();
}

/// @brief Draw everything to the screen for current frame
static void _gameDraw() 
{
	objMgrDraw();
}

/// @brief Perform updates for all game objects, for the elapsed duration
/// @param milliseconds 
static void _gameUpdate(uint32_t milliseconds)
{
	static bool latch = false;
	if (levelMgrGetState() == menuScreen)
	{
		if (inputMousePressed(INPUT_BUTTON_LEFT))
		{
			levelMgrStartGame();
			_wiiInput = true;
		}
		else if (inputMousePressed(INPUT_BUTTON_RIGHT))
		{
			_wiiInput = false;
			levelMgrStartGame();
		}
	}
	// Register user click as a gun shot
	else if (inputMousePressed(INPUT_BUTTON_LEFT))
	{
		if (!latch && levelMgrGetState() == gameScreen)
			processClick(inputMousePosition());
		latch = true;
	}
	else if (!inputMousePressed(INPUT_BUTTON_LEFT) && latch)
	{
		latch = false;

		// if the wii remote is being used, perform virtual alt+tab input for mouse y-axis consistency
		if (_wiiInput)
		{

			INPUT alt;


			
			alt.type = INPUT_KEYBOARD;
			alt.ki.wVk = VK_MENU; // Virtual key code for alt
			alt.ki.wScan = 0; // Hardware scan code (not used here)
			alt.ki.dwFlags = 0; // No special flags
			alt.ki.time = 0; // Let system provide timestamp
			alt.ki.dwExtraInfo = 0;

			// Press the key
			SendInput(1, &alt, sizeof(INPUT));

			INPUT tab;
			tab.type = INPUT_KEYBOARD;
			tab.ki.wVk = VK_TAB; // Virtual key code for tab
			tab.ki.wScan = 0; // Hardware scan code (not used here)
			tab.ki.dwFlags = 0; // No special flags
			tab.ki.time = 0; // Let system provide timestamp
			tab.ki.dwExtraInfo = 0;

			// Press the key
			SendInput(1, &tab, sizeof(INPUT));

			// Release the key
			tab.ki.dwFlags = KEYEVENTF_KEYUP;
			SendInput(1, &tab, sizeof(INPUT));
			// Release the key
			alt.ki.dwFlags = KEYEVENTF_KEYUP;
			SendInput(1, &alt, sizeof(INPUT));
		}
	}
	
	objMgrUpdate(milliseconds);
}