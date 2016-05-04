#include "main.h"
#include "Drawer.h"

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void inputHanlder( char* command );

rManager*		d3			= new rManager();
d3Console*		console		= new d3Console( 10,10,800,700 );
Memory*			m			= new Memory();

Logger			logger;
FileLogger		fileLogger;
driverManager	dMGR;
LPD3DXFONT		font;
D3Menu			d3d9;


bool				displayTeam			= false;
bool				displayEmptyCars	= false;
bool				displayPlayers		= true;
bool				displayCars			= true;
bool				displayItems		= false;
bool				isRunning			= true;


int					screenX = GetSystemMetrics(SM_CXSCREEN);
int					screenY = GetSystemMetrics(SM_CYSCREEN);
const				MARGINS margins = { -1, -1, -1, -1 };


VOID renderOverlay() {
	d3->clear();
	
	d3d9.render();
	console->render();

	d3->present();
}

INT WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	while (!m->isAttached())
		m->Attach(m->getProcessIdFromName("arma3.exe")), Sleep(1000);

	WNDCLASSEX		cx;
	HWND			hWnd		= NULL;

	cx.cbClsExtra = NULL;
	cx.cbSize = sizeof(WNDCLASSEX);
	cx.cbWndExtra = NULL;
	cx.hbrBackground = (HBRUSH)0;
	cx.hCursor = LoadCursor(NULL, IDC_ARROW);
	cx.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	cx.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	cx.hInstance = hInstance;
	cx.lpfnWndProc = WndProc;
	cx.lpszClassName = L"Cross";
	cx.lpszMenuName = NULL;
	cx.style = CS_HREDRAW | CS_VREDRAW;

	if (!RegisterClassEx(&cx))
		MessageBox(NULL, L"Couldn't register class", NULL, NULL), exit(0);

	hWnd = CreateWindowEx(WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW, L"Cross", L"Cross", WS_POPUP, 0, 0, screenX, screenY, NULL, NULL, hInstance, NULL);
	SetLayeredWindowAttributes(hWnd, RGB(0, 0, 0), 255, ULW_COLORKEY | LWA_ALPHA);

	ShowWindow(hWnd, nShowCmd);

	if ( !d3->Initilize(hWnd, screenX, screenY) )
		MessageBox(NULL, L"Couldn't initialize device", NULL, NULL), exit(0);

	if (!console->initilize(d3, hInstance))
		MessageBox(NULL, L"Couldn't initialize console", NULL, NULL), exit(0);

	if ( !d3d9.Initilize(d3,VK_OEM_3) )
		MessageBox(NULL, L"Couldn't initialize device", NULL, NULL), exit(0);

	console->sendInput( "Loading virtual driver" );

	WCHAR currentPath[MAX_PATH] = { 0 };
	GetCurrentDirectory(MAX_PATH, currentPath);
	wstring curPath = wstring( currentPath ) + L"\\Loader\\";

	if ( !dMGR.loadDriverless( curPath, L"FLDR.exe" , L"FMGR.sys" ) ) {
		//MessageBox(NULL, L"Couldn't load DRIVER", NULL, NULL), exit(0);
		console->sendInput( "Couldn't load driver! Operations will not work." );
	} else {
		console->sendInput( "Driver created with name: FMGR! Unloading virtual driver." );
	}

	d3d9.createMenu( "Arma III menu", NULL );

	// VISION
	PMENUENTRY vision = d3d9.createMenu( "Vision", NULL );
	d3d9.createItem( vision,"XRAY", new FLOAT(1000), NULL, 100, VAR_TYPE::_FLOAT, render, nullptr, true );
	d3d9.createToggle( vision, "Display players", &displayPlayers, NULL, nullptr, false );
	d3d9.createToggle( vision, "Cars", &displayCars, NULL, nullptr, false );
	d3d9.createToggle( vision, "Empty cars", &displayEmptyCars, NULL, nullptr, false );
	d3d9.createToggle( vision, "Other teams", &displayTeam, NULL, nullptr, false );

	//POSITION
	d3d9.createItem("Teleport", new INT(1), NULL, 1, ::VAR_TYPE::_INT, teleport, nullptr, true ); 
	d3d9.createItem("Unsafe Teleport",new INT(1), NULL, 1, ::VAR_TYPE::_INT, teleportUnsafe, nullptr, true ); 

	//FRAMES
	//d3d9.createItem("Frame Player", new string[250], NULL, NULL, VAR_TYPE::T_STRING, framePlayer, setPlayer, true);
	//d3d9.createItem("Kill Player", new string[250], NULL, NULL, VAR_TYPE::T_STRING, killPlayer, setPlayer, true);

	//OTHER
	d3d9.createItem( "Open Vehicle", nullptr, NULL, 0, ::VAR_TYPE::_INT, unlockCar, nullptr, false );

	logger.registerInput(inputHanlder);
	logger.handleInput();

	console->sendInput( "Process ready!" );

	MSG msg;
	SIZE_T	delay	= clock();
	while ( isRunning )
	{
		if( clock() - delay > 5000 )
			isRunning = m->isRunning();

		renderOverlay();

		console->receveInput();
		d3d9.handleInput();

		while ( PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) )
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	
	d3->~rManager();

	return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_PAINT:
		renderOverlay();
		break;
	case WM_CREATE:
		DwmExtendFrameIntoClientArea(hWnd, &margins);
		break;
	case WM_DESTROY:

		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void inputHanlder(char* command)
{
	if (!command || strlen(command) < 1)
		return;

	char* context = NULL;
	char* token = NULL;

	vector<string> splitString;

	token = strtok_s(command, " ", &context);
	if (!token)
		return;

	while (token != NULL)
	{
		splitString.push_back(token);
		token = strtok_s(NULL, " ", &context);
	}

	if (!strcmp(splitString.at(0).c_str(), "list"))
	{
		if (splitString.size() > 2)
		{
			if (splitString.at(1) == "weapon")
				listWeapon(splitString.at(2));
			else
				listConsumable(splitString.at(2));
		} else {
			cout << "list - [args] - 1. weapon, consumable 2. TYPE (rifle, mgun, pistol) (magazine, handgranade) (*)ALL" << endl;
		}
	} else if (!strcmp(splitString.at(0).c_str(), "spawn"))
	{
		if (splitString.size() > 3)
		{
			int ID = 0, amount = 0;

			try {
				ID = stoi(splitString.at(2));
				amount = stoi(splitString.at(3));
			} catch (...)
			{
				return;
			}

			if (splitString.at(1) == "weapon")
			{
				spawnWeaponConsole(ID, amount);
			} else {
				spawnConsumableConsole(ID, amount);
			}

		} else {
			cout << "spawn - [args] - 1.TYPE 2. ITEM ID, 3. AMOUNT" << endl;
		}
	} else if (!strcmp(splitString.at(0).c_str(), "find"))
	{
		if (splitString.size() > 2)
		{
			if (splitString.at(1) == "weapon")
			{
				findWeapon(splitString.at(2));
			} else {
				findConsumable(splitString.at(2));
			}

		} else {
			cout << "find - [args] - 1. TYPE (weapon, consumable) 2. ITEM NAME" << endl;
		}
	}
}