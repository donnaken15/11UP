#include <d3d9.h>
#include "d3dx9.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#pragma comment (lib, "d3d9.lib")
#pragma comment (lib, "d3dx9.lib")
#pragma comment (lib, "winmm.lib")

#include <windows.h>

LPDIRECT3DTEXTURE9 textures[4];
LPD3DXSPRITE drawing;
RECT rect, card_rect, serect, card_rect_num[20], card_rect_suit[4], card_rect_next[2], click_areas[24], card_score_rect[5], timer_rect_lights[3], flash_11s_rect;
LPRECT lprect;
D3DXVECTOR2 card_pos[23], card_suit_pos[23], card_next_pos[2], card_score_pos[23], timer_pos[36], flash_11s_pos;
LPD3DXFONT fonts[16];
POINT mousepos;
HFONT hfonts[16];

DWORD tick = 16, score, hiscore, bnsgoal, elevens;

byte cards[23], deck[0x7ffffff], deckleft, timer, timeleft = 36, curnum, timerInterval, level = 0, maxlvl, bonuslvl, curdeckcard, speedbns;

bool cardexists[23], selcards[23], matchedcards[23], LMB, matched, won, flash11s = false, bnsround;

unsigned int seed, seedampmin, seedampmax, seedamp, decksize;

unsigned long long frame, pause, lastTick;

std::stringstream stringstream;

LPCSTR title = "11UP", ini, flipcardSnd = "FLIPCARD.WAV", errorSnd = "ERROR.WAV";

std::string customCards = "";

/*
#define CARD_ACE			0b000000
#define CARD_TWO			0b000001
#define CARD_THREE			0b000010
#define CARD_FOUR			0b000011
#define CARD_FIVE			0b000100
#define CARD_SIX			0b000101
#define CARD_SEVEN			0b000110
#define CARD_EIGHT			0b000111
#define CARD_NINE			0b001000
#define CARD_TEN			0b001001

#define CARD_SUIT_CLUBS		0b000000
#define CARD_SUIT_SPADES	0b010000
#define CARD_SUIT_HEARTS	0b100000
#define CARD_SUIT_DIAMONDS	0b110000
*/

LPDIRECT3D9 d3d;
LPDIRECT3DDEVICE9 d3ddev;

void initD3D(HWND hWnd);
void render_frame(void);
void cleanD3D(void);
void playSnd(LPCSTR fname);
void NewTexture(LPCSTR fname, D3DFORMAT fmt, LPDIRECT3DTEXTURE9 *tex);
void drawText(int i, LPSTR text, int left, int top, int right, int bottom, D3DCOLOR COLOR);

int random(int min, int max)
{
	return rand() % max + min;
}

void drawText(int i, LPSTR text, int left, int top, int right, int bottom, D3DCOLOR COLOR)
{
	rect.left = left;
	rect.right = right;
	rect.top = top;
	rect.bottom = bottom;
	fonts[i]->DrawTextA(text,-1,&rect,NULL,COLOR);
}

void playSnd(LPCSTR fname)
{
	PlaySound(fname,
		GetModuleHandle(NULL),
		SND_ASYNC | SND_FILENAME | SND_NOWAIT);
}

void NewTexture(LPCSTR fname, D3DFORMAT fmt, LPDIRECT3DTEXTURE9 *tex)
{
	D3DXCreateTextureFromFileEx(d3ddev, fname, D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2,
		D3DX_DEFAULT, 0, fmt, D3DPOOL_DEFAULT, D3DX_DEFAULT,
		D3DX_DEFAULT, D3DCOLOR_XRGB(255,0,255), NULL, NULL, tex);
}

std::string workingdir()
{
	char buf[256];
	GetCurrentDirectoryA(256, buf);
	return std::string(buf) + '\\';
}

std::string get_profile_string(LPCSTR name, LPCSTR key, LPCSTR def, LPCSTR filename)
{
	char temp[1024];
	int result = GetPrivateProfileString(name, key, def, temp, sizeof(temp), filename);
	return std::string(temp, result);
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow)
{
	HWND hWnd;
	WNDCLASSEX wc;

	ZeroMemory(&wc, sizeof(WNDCLASSEX));

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.lpszClassName = title;

	RegisterClassEx(&wc);

	hWnd = CreateWindowEx(NULL,
		title,
		title,
		WS_OVERLAPPED | WS_SYSMENU,
		GetSystemMetrics(SM_CXSCREEN) / 2 - 640 / 2,
		GetSystemMetrics(SM_CYSCREEN) / 2 - 480 / 2,
		640 + 6,
		480 + 34,
		NULL,
		NULL,
		hInstance,
		NULL);

	ShowWindow(hWnd, nCmdShow);

	initD3D(hWnd);

	stringstream << workingdir() << + "\\GAME.INI";

	ini = const_cast<char *>(stringstream.str().c_str());

	seed = GetPrivateProfileInt(title, "Seed", GetTickCount(), ini);
	seedampmin = GetPrivateProfileInt(title, "SeedAmpMin", 1, ini);
	seedampmax = GetPrivateProfileInt(title, "SeedAmpMax", 1, ini);
	seedamp = int(rand() * seedampmax + seedampmin);
	tick = GetPrivateProfileInt(title, "TickRate", 16, ini);
	hiscore = GetPrivateProfileInt(title, "Hiscore", 50000, ini);
	maxlvl = GetPrivateProfileInt(title, "Rounds", 2, ini);
	decksize = GetPrivateProfileIntA(title, "DeckSize", 30, ini);
	bnsgoal = GetPrivateProfileIntA(title, "BonusGoal", 80000, ini);
	customCards = get_profile_string(title, "Nonrand", "", ini);
	curdeckcard = decksize;

	timer = 36;

	timerInterval = 1000 / tick;

	srand(seed*seedamp);

	MSG msg;

	while (TRUE)
	{
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (msg.message == WM_QUIT)
			break;

		render_frame();
	}

	cleanD3D();

	exit(score);

	return msg.wParam;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
	{
		PostQuitMessage(score);
		return 0;
	} break;
	case WM_LBUTTONDOWN:
	{
		GetCursorPos(&mousepos);
		ScreenToClient(hWnd, &mousepos);
		LMB = true;
	}
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

void initD3D(HWND hWnd)
{
	d3d = Direct3DCreate9(D3D_SDK_VERSION);

	D3DPRESENT_PARAMETERS d3dpp;

	ZeroMemory(&d3dpp, sizeof(d3dpp));

	d3dpp.Windowed = true;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = hWnd;
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

	d3d->CreateDevice(D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		hWnd,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING,
		&d3dpp,
		&d3ddev);

	d3ddev->SetRenderState(D3DRS_LIGHTING, FALSE);

	d3ddev->SetRenderState(D3DRS_ZENABLE, FALSE);

	hfonts[0] = CreateFontA(16, 6, 0, 0, 0, false, false, false, 0, 0, 0, 0, 0, "Tahoma");
	hfonts[1] = CreateFontA(64, 24, 0, 0, FW_BOLD, false, false, false, 0, 0, 0, 0, 0, "Arial");
	hfonts[2] = CreateFontA(24, 8, 0, 0, 0, false, false, false, 0, 0, 0, 0, 0, "Times New Roman");
	hfonts[3] = CreateFontA(64 * 2, 26 * 2, 0, 0, FW_BOLD, false, false, false, 0, 0, 0, 0, 0, "Arial");
	hfonts[4] = CreateFontA(14 * 2, 6 * 2, 0, 0, 0, false, false, false, 0, 0, 0, 0, 0, "Arial");
	hfonts[5] = CreateFontA(60 * 2, 22 * 2, 0, 0, FW_BOLD, false, false, false, 0, 0, 0, 0, 0, "Arial");
	for (int i = 0; i <= 5; i++)
		D3DXCreateFont(d3ddev, hfonts[i], &fonts[i]);

	NewTexture("BACK.TEX", D3DFMT_R5G6B5, &textures[0]);
	NewTexture("CARD.TEX", D3DFMT_A1R5G5B5, &textures[1]);
	NewTexture("TIMER.TEX", D3DFMT_A1R5G5B5, &textures[2]);
	NewTexture("11SFLASH.TEX", D3DFMT_A1R5G5B5, &textures[3]);

	D3DXCreateSprite(d3ddev, &drawing);

	rect.top = 0;
	rect.left = 0;
	rect.right = 640;
	rect.bottom = 480;

	lastTick = GetTickCount() - 16;

	card_rect.left = 66;
	card_rect.top = 64;
	card_rect.right = 132;
	card_rect.bottom = 147;

	for (int i = 0; i < 10; i++)
		for (int j = 0; j < 2; j++)
		{
			card_rect_num[i * 2 + j].top = i * 27;
			card_rect_num[i * 2 + j].left = j * 33;
			card_rect_num[i * 2 + j].right = card_rect_num[i * 2 + j].left + 33;
			card_rect_num[i * 2 + j].bottom = card_rect_num[i * 2 + j].top + 27;
		}

	for (int i = 0; i < 2; i++)
		for (int j = 0; j < 2; j++)
		{
			card_rect_suit[i * 2 + j].top = i * 32;
			card_rect_suit[i * 2 + j].left = 66 + j * 33;
			card_rect_suit[i * 2 + j].right = card_rect_suit[i * 2 + j].left + 33;
			card_rect_suit[i * 2 + j].bottom = card_rect_suit[i * 2 + j].top + 32;
		}

	card_pos[0].x = 287;
	card_pos[0].y = 112;

	card_pos[1].x = card_pos[0].x - 161;
	card_pos[1].y = card_pos[0].y + 41;

	card_pos[2].x = card_pos[1].x + 115;
	card_pos[2].y = card_pos[1].y;

	card_pos[3].x = card_pos[2].x + 90;
	card_pos[3].y = card_pos[2].y;

	card_pos[4].x = card_pos[3].x + 116;
	card_pos[4].y = card_pos[3].y;

	card_pos[5].x = 82;
	card_pos[5].y = 190;

	card_pos[6].x = card_pos[5].x + 84;
	card_pos[6].y = card_pos[5].y;

	card_pos[7].x = card_pos[6].x + 118;
	card_pos[7].y = card_pos[6].y;

	card_pos[8].x = card_pos[7].x + 123;
	card_pos[8].y = card_pos[7].y;

	card_pos[9].x = card_pos[8].x + 84;
	card_pos[9].y = card_pos[8].y;

	card_pos[10].x = 42;
	card_pos[10].y = 228;

	card_pos[11].x = card_pos[10].x + 84;
	card_pos[11].y = card_pos[10].y;

	card_pos[12].x = card_pos[11].x + 83;
	card_pos[12].y = card_pos[11].y;

	card_pos[13].x = card_pos[12].x + 154;
	card_pos[13].y = card_pos[12].y;

	card_pos[14].x = card_pos[13].x + 84;
	card_pos[14].y = card_pos[13].y;

	card_pos[15].x = card_pos[14].x + 85;
	card_pos[15].y = card_pos[14].y;

	card_pos[16].x = 81;
	card_pos[16].y = 267;

	card_pos[17].x = card_pos[16].x + 84;
	card_pos[17].y = card_pos[16].y;

	card_pos[18].x = card_pos[17].x + 241;
	card_pos[18].y = card_pos[17].y;

	card_pos[19].x = card_pos[18].x + 85;
	card_pos[19].y = card_pos[18].y;

	card_pos[20].x = 126;
	card_pos[20].y = 299;

	card_pos[21].x = 446;
	card_pos[21].y = 299;

	card_pos[22].x = 331;
	card_pos[22].y = 359;

	serect.left = 84 - 9;
	serect.top = 226 - 29;
	serect.right = serect.left + 9 + 48;
	serect.bottom = serect.top + 29 + 44;

	for (int i = 0; i < 22; i++)
	{
		click_areas[i].left = card_pos[i].x;
		click_areas[i].top = card_pos[i].y;
		click_areas[i].right = card_pos[i].x + 66;
		click_areas[i].bottom = card_pos[i].y + 83;
	}

	card_rect_next[0].left = 0;
	card_rect_next[0].top = 270;
	card_rect_next[0].right = 66;
	card_rect_next[0].bottom = 312;

	card_rect_next[1].left = 66;
	card_rect_next[1].top = 271;
	card_rect_next[1].right = 132;
	card_rect_next[1].bottom = 312;

	card_next_pos[0].x = 244;
	card_next_pos[0].y = 359;

	card_next_pos[1].x = 244;
	card_next_pos[1].y = 359 + 42;

	for (int i = 0; i < 23; i++)
	{
		card_suit_pos[i] = card_pos[i];
		card_suit_pos[i].x += 33;
	}

	for (int i = 0; i < 23; i++)
		cardexists[i] = true;

	click_areas[22].left = 331;
	click_areas[22].top = 359;
	click_areas[22].right = click_areas[22].left + 66;
	click_areas[22].bottom = click_areas[22].top + 83;

	click_areas[23].left = 244;
	click_areas[23].top = 359;
	click_areas[23].right = click_areas[23].left + 66;
	click_areas[23].bottom = click_areas[23].top + 83;

	card_score_rect[0].left = 66;
	card_score_rect[0].top = 147;
	card_score_rect[0].right = card_score_rect[0].left + 25;
	card_score_rect[0].bottom = card_score_rect[0].top + 16;

	card_score_rect[1].left = 91;
	card_score_rect[1].top = 147;
	card_score_rect[1].right = card_score_rect[1].left + 28;
	card_score_rect[1].bottom = card_score_rect[1].top + 16;

	card_score_rect[2].left = 66;
	card_score_rect[2].top = 163;
	card_score_rect[2].right = card_score_rect[2].left + 28;
	card_score_rect[2].bottom = card_score_rect[2].top + 16;

	card_score_rect[3].left = 94;
	card_score_rect[3].top = 163;
	card_score_rect[3].right = card_score_rect[3].left + 29;
	card_score_rect[3].bottom = card_score_rect[3].top + 16;

	card_score_rect[4].left = 66;
	card_score_rect[4].top = 179;
	card_score_rect[4].right = card_score_rect[4].left + 28;
	card_score_rect[4].bottom = card_score_rect[4].top + 16;

	card_score_rect[5].left = 94;
	card_score_rect[5].top = 179;
	card_score_rect[5].right = card_score_rect[5].left + 28;
	card_score_rect[5].bottom = card_score_rect[5].top + 16;

	for (int i = 0; i < 23; i++)
	{
		card_score_pos[i].x = card_pos[i].x + 20;
		card_score_pos[i].y = card_pos[i].y + 44;
	}

	timer_rect_lights[0].left = 0;
	timer_rect_lights[0].top = 0;
	timer_rect_lights[0].right = 13;
	timer_rect_lights[0].bottom = 9;

	timer_rect_lights[1].left = 0;
	timer_rect_lights[1].top = 9;
	timer_rect_lights[1].right = 13;
	timer_rect_lights[1].bottom = 18;

	timer_rect_lights[2].left = 0;
	timer_rect_lights[2].top = 18;
	timer_rect_lights[2].right = 13;
	timer_rect_lights[2].bottom = 27;

	timer_pos[0].x = 504;
	timer_pos[0].y = 10;

	timer_pos[1].x = timer_pos[0].x + 17;
	timer_pos[1].y = timer_pos[0].y;
	
	timer_pos[2].x = timer_pos[1].x + 17;
	timer_pos[2].y = timer_pos[1].y;
	
	timer_pos[3].x = timer_pos[2].x + 17;
	timer_pos[3].y = timer_pos[2].y;
	
	timer_pos[4].x = timer_pos[3].x + 17;
	timer_pos[4].y = timer_pos[3].y;
	
	timer_pos[5].x = timer_pos[4].x + 17;
	timer_pos[5].y = timer_pos[4].y;
	
	timer_pos[6].x = 504;
	timer_pos[6].y = 23;

	timer_pos[7].x = timer_pos[6].x + 17;
	timer_pos[7].y = timer_pos[6].y;
	
	timer_pos[8].x = timer_pos[7].x + 17;
	timer_pos[8].y = timer_pos[7].y;
	
	timer_pos[9].x = timer_pos[8].x + 17;
	timer_pos[9].y = timer_pos[8].y;
	
	timer_pos[10].x = timer_pos[9].x + 17;
	timer_pos[10].y = timer_pos[9].y;
	
	timer_pos[11].x = timer_pos[10].x + 17;
	timer_pos[11].y = timer_pos[10].y;
	
	timer_pos[12].x = 504;
	timer_pos[12].y = 36;

	timer_pos[13].x = timer_pos[12].x + 17;
	timer_pos[13].y = timer_pos[12].y;
	
	timer_pos[14].x = timer_pos[13].x + 17;
	timer_pos[14].y = timer_pos[13].y;
	
	timer_pos[15].x = timer_pos[14].x + 17;
	timer_pos[15].y = timer_pos[14].y;
	
	timer_pos[16].x = timer_pos[15].x + 17;
	timer_pos[16].y = timer_pos[15].y;
	
	timer_pos[17].x = timer_pos[16].x + 17;
	timer_pos[17].y = timer_pos[16].y;
	
	timer_pos[18].x = 504;
	timer_pos[18].y = 49;

	timer_pos[19].x = timer_pos[18].x + 17;
	timer_pos[19].y = timer_pos[18].y;
	
	timer_pos[20].x = timer_pos[19].x + 17;
	timer_pos[20].y = timer_pos[19].y;
	
	timer_pos[21].x = timer_pos[20].x + 17;
	timer_pos[21].y = timer_pos[20].y;
	
	timer_pos[22].x = timer_pos[21].x + 17;
	timer_pos[22].y = timer_pos[21].y;
	
	timer_pos[23].x = timer_pos[22].x + 17;
	timer_pos[23].y = timer_pos[22].y;
	
	timer_pos[24].x = 504;
	timer_pos[24].y = 62;

	timer_pos[25].x = timer_pos[24].x + 17;
	timer_pos[25].y = timer_pos[24].y;
	
	timer_pos[26].x = timer_pos[25].x + 17;
	timer_pos[26].y = timer_pos[25].y;
	
	timer_pos[27].x = timer_pos[26].x + 17;
	timer_pos[27].y = timer_pos[26].y;
	
	timer_pos[28].x = timer_pos[27].x + 17;
	timer_pos[28].y = timer_pos[27].y;
	
	timer_pos[29].x = timer_pos[28].x + 17;
	timer_pos[29].y = timer_pos[28].y;
	
	timer_pos[30].x = 504;
	timer_pos[30].y = 75;

	timer_pos[31].x = timer_pos[30].x + 17;
	timer_pos[31].y = timer_pos[30].y;
	
	timer_pos[32].x = timer_pos[31].x + 17;
	timer_pos[32].y = timer_pos[31].y;
	
	timer_pos[33].x = timer_pos[32].x + 17;
	timer_pos[33].y = timer_pos[32].y;
	
	timer_pos[34].x = timer_pos[33].x + 17;
	timer_pos[34].y = timer_pos[33].y;
	
	timer_pos[35].x = timer_pos[34].x + 17;
	timer_pos[35].y = timer_pos[34].y;

	flash_11s_pos.x = 60;
	flash_11s_pos.y = 105;

	flash_11s_rect.left = 0;
	flash_11s_rect.top = 0;
	flash_11s_rect.right= 106;
	flash_11s_rect.bottom = 46;

	stringstream.imbue(std::locale(""));
}

int choose(int choice1, int choice2, int choice3, int choice4)
{
	switch (int(rand() * 3))
	{
	case 0:
		return choice1;
	case 1:
		return choice2;
	case 2:
		return choice3;
	case 3:
		return choice4;
	default:
		break;
	}
}

int getCardNum(byte card)
{
	return ((cards[card] >> 0) & 0x01) +
		(((cards[card] >> 1) & 0x01) * 2) +
		(((cards[card] >> 2) & 0x01) * 4) +
		(((cards[card] >> 3) & 0x01) * 8);
}

int getCardColor(byte card)
{
	return ((cards[card] >> 4) & 0x01) +
		(((cards[card] >> 5) & 0x01) * 2);
}

bool clickedArea(RECT rct)
{
	return (mousepos.x >= rct.left && mousepos.x <= rct.right
		&& mousepos.y >= rct.top && mousepos.y <= rct.bottom && LMB);
}

void drawCard(int i)
{
	drawing->Draw(textures[1], &card_rect, NULL, NULL, 0, &card_pos[i], 0xFFFFFFFF);
	drawing->Draw(textures[1], &card_rect_num[getCardNum(i) + (((cards[i] >> 5) & 0x01) * 10)], NULL, NULL, 0, &card_pos[i], 0xFFFFFFFF);
	drawing->Draw(textures[1], &card_rect_suit[getCardColor(i)], NULL, NULL, 0, &card_suit_pos[i], 0xFFFFFFFF);
}

void selectCard(int i)
{
	if (cardexists[i])
		if (!selcards[i] && curnum + getCardNum(i) < 11)
		{
			selcards[i] = true;
			playSnd("SELECT.WAV");
		}
		else if (selcards[i])
		{
			selcards[i] = false;
			playSnd("SELECT.WAV");
		}
		else
			playSnd(errorSnd);
	LMB = false;
}

void flipCard()
{
	if (curdeckcard > 0)
	{
		curdeckcard--;
		for (int i = 0; i < 23; i++)
			selcards[i] = 0;
		for (int i = 0; i < 22; i++)
			if (!cardexists[i])
			{
				cards[i] = cards[22];
				cardexists[i] = true;
				break;
			}
		playSnd(flipcardSnd);
		cards[22] = random(0, 9) + (16 * random(0, 4));
	}
}

bool ifSelectedCard(int i)
{
	return clickedArea(click_areas[i]) && cardexists[i];
}

void render_frame(void)
{
	stringstream.str("");

	curnum = 0;

	for (int i = 0; i < 23; i++)
		if (selcards[i])
			curnum += getCardNum(i) + 1;

	d3ddev->Clear(0, NULL, D3DCLEAR_TARGET, 0, 0.0f, 0);

	d3ddev->BeginScene();

	drawing->Begin();

	drawing->Draw(textures[0], NULL, NULL, 0, 0, NULL, 0xFFFFFFFF);

	for (int i = 0; i < 23; i++)
	{
		if (frame >= 130 + (4 * i))
			if (cardexists[i])
			{
				drawCard(i);
				if (selcards[i])
					drawing->Draw(textures[1], &serect, NULL, NULL, 0, &card_pos[i], 0xFFFFFFFF);
			}
		if (matchedcards[i])
		{
			{
				int j;
				if (i == 0)
					j = 5;
				else if (i > 0 && i <= 4)
					j = 4;
				else if (i >= 5 && i < 10)
					j = 3;
				else if (i >= 10 && i <= 15)
					j = 2;
				else if (i >= 16 && i <= 19)
					j = 1;
				else
					j = 0;
				drawing->Draw(textures[1], &card_score_rect[j], NULL, NULL, 0, &card_score_pos[i], 0xFFFFFFFF);
			}
		}
	}

	if (frame >= ULLONG_MAX - 4)
	{
		won = false;
		drawText(3, "WINNER", 110, 104, 640, 480, D3DCOLOR_XRGB(192, 0, 255));
		drawText(5, "PLAYER 1", 120, 220, 640, 480, D3DCOLOR_XRGB(0, 255, 0));
		if (frame == ULLONG_MAX - 1)
			PlaySoundA("WINNER.WAV", NULL, SND_FILENAME);
	}

	if (score > bnsgoal && !bnsround)
	{
		maxlvl++;
		bnsround = true;
	}

	if (frame == ULLONG_MAX - 0xfff0)
	{
		if (timer > 0)
		{
			timer -= 1;
			playSnd("SPEEDBONUS.WAV");
			speedbns += 1;
			score += 150;
			frame -= 2;
		}
		else
			frame = ULLONG_MAX - 0xfff;
	}

	if (frame >= ULLONG_MAX - 0x1002 && timeleft > 0 && timeleft != 127)
	{
		drawText(4, "SPEED\nBONUS", 503, 8, 640, 480, D3DCOLOR_XRGB(0, 255, 255));
		stringstream << int(speedbns * 150);
		drawText(4, const_cast<char *>(stringstream.str().c_str()), 507, 60, 640, 480, D3DCOLOR_XRGB(255, 255, 0));
		stringstream.str("");
	}

	if (timer == 0 && timeleft == 0)
	{
		timeleft = 127;
		frame = ULLONG_MAX - 0xffffff;
	}

	if (frame >= ULLONG_MAX - 0xffffff && frame <= ULLONG_MAX - 0xfffff8)
		drawText(4, "TIME'S\n  UP!", 503, 8, 640, 480, D3DCOLOR_XRGB(255, 0, 0));

	if (frame == ULLONG_MAX - 0xfffffa)
	{
		won = false;
		PlaySoundA("TIMESUP.WAV", NULL, SND_FILENAME);
		frame = ULLONG_MAX - 0xff;
	}

	if (frame == ULLONG_MAX - 0xfff && won)
	{
		if (curdeckcard > 0)
		{
			curdeckcard--;
			score += 1000;
			playSnd("DECKBONUS.WAV");
			frame -= 4;
		}
		else
			frame = ULLONG_MAX - 0xff;
	}

	if (flash11s)
		drawing->Draw(textures[3], &flash_11s_rect, NULL, NULL, 0, &flash_11s_pos, 0xFFFFFFFF);

	if (frame == ULLONG_MAX - 0xff)
	{
		if (elevens > 0)
		{
			score += 1000;
			PlaySoundA("COUNT11UP.WAV", NULL, SND_NOSTOP | SND_FILENAME | SND_ASYNC | SND_LOOP);
			flash11s = !flash11s;
			frame -= 15;
			elevens--;
		}
		else
		{
			flash11s = false;
			PlaySoundA("NULL.WAV", NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
			PlaySoundA("COUNT11UPSTOP.WAV", NULL, SND_FILENAME);
			if (level < maxlvl)
			{
				frame = 10;
				for (int i = 0; i < 23; i++)
					cardexists[i] = true;
				level++;
			}
			else
				frame = ULLONG_MAX - 4;
		}
	}

	if (frame >= ULLONG_MAX - 0xfffff && won)
	{
		drawText(1, "LEVEL COMPLETE", 110, 144, 640, 480, D3DCOLOR_XRGB(0, 255, 0));
		drawText(3, "BONUS!", 120, 190, 640, 480, D3DCOLOR_XRGB(255, 255, 0));
		drawText(1, "+10,000", 220, 290, 640, 480, D3DCOLOR_XRGB(255, 255, 0));
		switch (ULLONG_MAX - frame)
		{
		case 0xffff9:
		case 0xffff6:
		case 0xffff3:
		case 0xffff0:
		case 0xffff0 - (3 * 1) :
		case 0xffff0 - (3 * 2) :
		case 0xffff0 - (3 * 3) :
		case 0xffff0 - (3 * 4) :
		case 0xffff0 - (3 * 5) :
		case 0xffff0 - (3 * 6) :
			playSnd("BONUS.WAV");
			score += 1000;
			break;
		case 0xffff0 - (3 * 8) :
			frame = ULLONG_MAX - 0xfff1;
			break;
		}
		if (frame > ULLONG_MAX - 0xffff3 + (3 * 8))
			drawText(1, "AWARDED", 200, 260, 640, 480, D3DCOLOR_XRGB(255, 0, 0));
	}

	if (GetAsyncKeyState(VK_SPACE))
		frame = ULLONG_MAX - 4;
	
	if (frame == ULLONG_MAX - 0xffffa)
	{
		timeleft = timer;
		playSnd("WINROUND.WAV");
		Sleep(6922);
	}

	if (!cardexists[0] && !cardexists[1] &&
		!cardexists[2] && !cardexists[3] &&
		!cardexists[4] && !cardexists[5] &&
		!cardexists[6] && !cardexists[7] &&
		!cardexists[8] && !cardexists[9] &&
		!cardexists[10] && !cardexists[11] &&
		!cardexists[12] && !cardexists[13] &&
		!cardexists[14] && !cardexists[15] &&
		!cardexists[16] && !cardexists[17] &&
		!cardexists[18] && !cardexists[19] &&
		!cardexists[20] && !cardexists[21] && frame < ULLONG_MAX - 0xfffff)
	{
		won = true;
		frame = ULLONG_MAX - 0xfffff;
	}
	
	if (frame > 130 + (4 * 22))
	{
		stringstream << score;
		drawText(2, const_cast<char *>(stringstream.str().c_str()), 86, 6, 640, 480, D3DCOLOR_XRGB(255, 255, 0));
		stringstream.str("");
		if (score < hiscore)
			stringstream << hiscore;
		else
			stringstream << score;
		drawText(2, const_cast<char *>(stringstream.str().c_str()), 98, 63, 640, 480, 0xFFFFFFFF);
		stringstream.str("");
		if (elevens < 10)
			stringstream << " ";
		if (flash11s)
			drawing->Draw(textures[3], &rect, NULL, NULL, 0, &flash_11s_pos, 0xFFFFFFFF);
		stringstream << elevens;
		drawText(2, const_cast<char *>(stringstream.str().c_str()), 135, 119, 640, 480, 0xFFFFFFFF);
		stringstream.str("");
		if (level == maxlvl && bnsround)
			drawText(1, "B", 445, 25, 640, 480, D3DCOLOR_XRGB(0, 255, 255));
		else
		{
			stringstream << level + 1;
			drawText(1, const_cast<char *>(stringstream.str().c_str()), 445, 25, 640, 480, D3DCOLOR_XRGB(0, 255, 255));
			stringstream.str("");
		}
		stringstream << int(curnum);
		drawText(2, const_cast<char *>(stringstream.str().c_str()), 313, 312, 640, 480, D3DCOLOR_XRGB(255, 255, 0));
		stringstream.str("");
		stringstream << int(curdeckcard);
		drawText(2, const_cast<char *>(stringstream.str().c_str()), 206, 390, 640, 480, 0xFFFFFFFF);
		stringstream.str("");
		if (curdeckcard > 0)
		{
			drawing->Draw(textures[1], &card_rect_next[0], NULL, NULL, 0, &card_next_pos[0], 0xFFFFFFFF);
			drawing->Draw(textures[1], &card_rect_next[1], NULL, NULL, 0, &card_next_pos[1], 0xFFFFFFFF);
		}
		for (int i = 35; i > 35 - timer; i--)
		{
			int j;
			if (i > 29 && i <= 35)
				j = 2;
			else if (i > 23 && i <= 29)
				j = 1;
			else
				j = 0;
			drawing->Draw(textures[2], &timer_rect_lights[j], NULL, NULL, 0, &timer_pos[i], 0xFFFFFFFF);
		}
		if (frame < ULLONG_MAX - 0xfffff && timer > 0)
			if (timerInterval > 0)
				timerInterval--;
			else
			{
				timer--;
				timerInterval = 2500 / tick;
				if (timer == 6)
					playSnd("HURRYUP.WAV");
			}
	}

	fonts[0]->Begin();

	/*stringstream << "Framerate: " << 1000 / tick << "\nTickrate: " << tick << "\nFrame: " << frame << "\nSeed: " << seed << "\nSeed Amplified (" << seedampmin << "-" << seedampmax << "x):\n" << seed*seedamp << "\nLevel: " << int(level) << "\nMax Levels: " << int(maxlvl) << "\nBonus Round: " << bnsround << "\nDeck Size: " << int(curdeckcard) << " / " << decksize << "\nGame Timer: " << int(timer) << "\nElevens: " << int(elevens) << "\nMouse Position: " << mousepos.x << ", " << mousepos.y << "\nSelected cards: ";
	for (int i = 0; i < 23; i++)
		stringstream << selcards[i];
	stringstream << "\nPause Frames: " << int(pause);
	for (int i = 0; i < 23; i++)
		stringstream << " \nCard " << i << ": " << ((cards[i] >> 0) & 0x01) + getCardNum(i) << " / " << ((cards[i] >> 5) & 0x01) << ((cards[i] >> 4) & 0x01) << " " << ((cards[i] >> 3) & 0x01) << ((cards[i] >> 2) & 0x01) << ((cards[i] >> 1) & 0x01) << ((cards[i] >> 0) & 0x01);
	drawText(0, const_cast<char *>(stringstream.str().c_str()), 0, 0, 640, 480, D3DCOLOR_XRGB(0, 255, 255));
	stringstream.str("");*/

	fonts[0]->End();

	fonts[1]->Begin();

	if (frame == 10)
	{
		for (byte i = 0; i < 23; i++)
		{
			if (customCards == "")
				cards[i] = random(0, 10);
			else
				if (customCards[i] != 'X')
					cards[i] = customCards[i] - 0x30;
				else
					cards[i] = 10;
			cards[i] += 16 * random(0, 4);
			selcards[i] = false;
		}
		curdeckcard = decksize;
		timer = 36;
		timeleft = 36;
	}

	if (frame >= 10 && frame <= 130)
	{
		if (level == maxlvl && bnsround)
			drawText(1, "BONUS ROUND", 140, 200, 600, 400, D3DCOLOR_XRGB(255, 255, 0));
		else
		{
			stringstream << "ROUND " << level + 1;
			drawText(1, const_cast<char *>(stringstream.str().c_str()), 215, 200, 500, 400, D3DCOLOR_XRGB(255, 255, 0));
			stringstream.str("");
		}
		if (!bnsround && level == maxlvl)
		{
			stringstream << "BONUS ROUND AT " << bnsgoal;
			drawText(4, const_cast<char *>(stringstream.str().c_str()), 160, 270, 640, 480, D3DCOLOR_XRGB(0, 255, 255));
			stringstream.str("");
		}
	}

	fonts[1]->End();

	if (curnum == 11 && pause == 0)
	{
		for (int i = 0; i < 23; i++)
			if (selcards[i])
				if (i == 0)
					score += 600;
				else if (i > 0 && i <= 4)
					score += 500;
				else if (i >= 5 && i < 10)
					score += 400;
				else if (i >= 10 && i <= 15)
					score += 300;
				else if (i >= 16 && i <= 19)
					score += 200;
				else
					score += 100;
		for (int i = 0; i < 23; i++)
		{
			matchedcards[i] = selcards[i];
			selcards[i] = 0;
		}
		pause = 10;
		playSnd("MATCH.WAV");
		elevens++;
	}

	if (pause == 2)
	{
		if (ifSelectedCard(23))
			curdeckcard--;
		for (int i = 0; i < 23; i++)
			if (matchedcards[i] && cardexists[i])
			{
				selcards[i] = false;
				if (i != 22)
					cardexists[i] = false;
				else
					cards[22] = random(0, 9) + (16 * random(0, 4));
				matchedcards[i] = false;
			}
		curnum = 0;
	}

	if (frame > 222 && pause == 0 && frame < ULLONG_MAX - 0xfffffff)
	{
		timeleft = timer;

		if (clickedArea(click_areas[23]))
			flipCard();

		if (ifSelectedCard(22))
			selectCard(22);

		if (ifSelectedCard(21))
			selectCard(21);

		if (ifSelectedCard(20))
			selectCard(20);

		if (ifSelectedCard(19))
			if (!cardexists[21])
				selectCard(19);
			else
				playSnd(errorSnd);

		if (ifSelectedCard(18))
			if (!cardexists[21])
				selectCard(18);
			else
				playSnd(errorSnd);

		if (ifSelectedCard(17))
			if (!cardexists[20])
				selectCard(17);
			else
				playSnd(errorSnd);

		if (ifSelectedCard(16))
			if (!cardexists[20])
				selectCard(16);
			else
				playSnd(errorSnd);

		if (ifSelectedCard(15))
			if (!cardexists[19])
				selectCard(15);
			else
				playSnd(errorSnd);

		if (ifSelectedCard(14))
			if (!cardexists[18] && !cardexists[19])
				selectCard(14);
			else
				playSnd(errorSnd);

		if (ifSelectedCard(13))
			if (!cardexists[18])
				selectCard(13);
			else
				playSnd(errorSnd);

		if (ifSelectedCard(12))
			if (!cardexists[17])
				selectCard(12);
			else
				playSnd(errorSnd);

		if (ifSelectedCard(11))
			if (!cardexists[16] && !cardexists[17])
				selectCard(11);
			else
				playSnd(errorSnd);

		if (ifSelectedCard(10))
			if (!cardexists[16])
				selectCard(10);
			else
				playSnd(errorSnd);

		if (ifSelectedCard(9))
			if (!cardexists[14] && !cardexists[15])
				selectCard(9);
			else
				playSnd(errorSnd);

		if (ifSelectedCard(8))
			if (!cardexists[13] && !cardexists[14])
				selectCard(8);
			else
				playSnd(errorSnd);

		if (ifSelectedCard(7))
			selectCard(7);

		if (ifSelectedCard(6))
			if (!cardexists[11] && !cardexists[12])
				selectCard(6);
			else
				playSnd(errorSnd);

		if (ifSelectedCard(5))
			if (!cardexists[10] && !cardexists[11])
				selectCard(5);
			else
				playSnd(errorSnd);

		if (ifSelectedCard(4))
			if (!cardexists[8] && !cardexists[9])
				selectCard(4);
			else
				playSnd(errorSnd);

		if (ifSelectedCard(3))
			if (!cardexists[7])
				selectCard(3);
			else
				playSnd(errorSnd);

		if (ifSelectedCard(2))
			if (!cardexists[7])
				selectCard(2);
			else
				playSnd(errorSnd);

		if (ifSelectedCard(1))
			if (!cardexists[5] && !cardexists[6])
				selectCard(1);
			else
				playSnd(errorSnd);

		if (ifSelectedCard(0))
			if (!cardexists[2] && !cardexists[3])
				selectCard(0);
			else
				playSnd(errorSnd);
	}
	
	switch (frame)
	{
	case 10:
		playSnd("NEWROUND.WAV");
		break;
	case 134:
	case 138:
	case 142:
	case 146:
	case 150:
	case 154:
	case 158:
	case 162:
	case 166:
	case 170:
	case 174:
	case 178:
	case 182:
	case 186:
	case 190:
	case 194:
	case 198:
	case 202:
	case 206:
	case 210:
	case 214:
	case 218:
	case 222:
		playSnd(flipcardSnd);
		break;
	}

	drawing->End();

	d3ddev->EndScene();

	d3ddev->Present(NULL, NULL, NULL, NULL);

	curnum = 0;

	LMB = false;

	if (tick != 0 && !GetAsyncKeyState(VK_TAB))
		Sleep(tick);

	if (pause > 0)
		pause--;

	if (GetAsyncKeyState(VK_PAUSE))
		MessageBox(NULL,"PAUSED","11UP",MB_OK);

	if (frame >= ULLONG_MAX)
		exit(score);

	if (score >= hiscore)
	{
		stringstream.clear();
		stringstream << score;
		LPCSTR newscore = stringstream.str().c_str();
		stringstream.str("");
		stringstream << workingdir() << + "\\GAME.INI";
		WritePrivateProfileStringA("11UP", "Hiscore", newscore, stringstream.str().c_str());
		stringstream.str("");
		hiscore = score;
	}

	frame++;
}

void cleanD3D(void)
{
	d3ddev->Release();
	d3d->Release();
	exit(score);
}