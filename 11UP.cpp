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

LPDIRECT3DTEXTURE9 textures[2];
LPD3DXSPRITE drawing;
RECT rect, card_rect, serect, card_rect_num[20], card_rect_suit[4], card_rect_next[2], click_areas[24];
LPRECT lprect;
D3DXVECTOR2 card_pos[23], card_suit_pos[23], card_next_pos[2];
LPD3DXFONT fonts[16];
POINT mousepos;
HFONT hfonts[16];

DWORD tick = 16, score, hiscore, elevens;

byte cards[23], deck[0x7fffff], timer, curnum, level = 0, maxlvl, bonuslvl;

bool cardexists[23], selcards[23], LMB;

unsigned int seed, seedampmin, seedampmax, seedamp, decksize;

unsigned long long frame;

std::stringstream stringstream;

LPCSTR title = "11UP", ini;

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
	seedamp = int(rand()*seedampmax + seedampmin);
	tick = GetPrivateProfileInt(title, "TickRate", 16, ini);
	hiscore = GetPrivateProfileInt(title, "Hiscore", 50000, ini);
	maxlvl = GetPrivateProfileInt(title, "Rounds", 2, ini);
	decksize = GetPrivateProfileIntA(title, "DeckSize", 30, ini);

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
	for (int i = 0; i <= 1; i++)
		D3DXCreateFont(d3ddev, hfonts[i], &fonts[i]);

	NewTexture("BACK.DDS", D3DFMT_R5G6B5, &textures[0]);
	NewTexture("CARD.DDS", D3DFMT_A1R5G5B5, &textures[1]);

	D3DXCreateSprite(d3ddev, &drawing);

	rect.top = 0;
	rect.left = 0;
	rect.right = 640;
	rect.bottom = 480;

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
		playSnd("ERROR.WAV");
}

void null() { }

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
		if (cardexists[i] && frame >= 130 + (4 * i))
		{
			drawCard(i);
			if (selcards[i])
				drawing->Draw(textures[1], &serect, NULL, NULL, 0, &card_pos[i], 0xFFFFFFFF);
		}
		else
			break;

	if (frame > 130 + (4 * 22))
	{
		drawing->Draw(textures[1], &card_rect_next[0], NULL, NULL, 0, &card_next_pos[0], 0xFFFFFFFF);
		drawing->Draw(textures[1], &card_rect_next[1], NULL, NULL, 0, &card_next_pos[1], 0xFFFFFFFF);
	}

	fonts[0]->Begin();

	stringstream << "Frame: " << frame << "\nSeed: " << seed << "\nSeed Amplified (" << seedampmin << "-" << seedampmax << "x):\n" << seed*seedamp << "\nDeck Size: " << decksize << "\nMouse Position: " << mousepos.x << ", " << mousepos.y;
	for (int i = 0; i < 23; i++)
		stringstream << " \nCard " << i << ": " << ((cards[i] >> 0) & 0x01) + getCardNum(i) << " / " << ((cards[i] >> 5) & 0x01) << ((cards[i] >> 4) & 0x01) << " " << ((cards[i] >> 3) & 0x01) << ((cards[i] >> 2) & 0x01) << ((cards[i] >> 1) & 0x01) << ((cards[i] >> 0) & 0x01);
	stringstream << "\nCurrent Number: " << int(curnum);
	drawText(0, const_cast<char *>(stringstream.str().c_str()), 0, 0, 640, 480, D3DCOLOR_XRGB(0, 255, 255));
	stringstream.str("");

	fonts[0]->End();

	fonts[1]->Begin();

	if (frame == 10)
	{
		for (byte i = 0; i < 22; i++)
			cards[i] = random(0, 9) + (16 * random(0, 4));
		for (byte i = 0; i < 30; i++)
			deck[i] = byte(random(0, 9) + (16 * random(0, 4)));
		cards[23] = deck[decksize];
	}

	if (frame >= 10 && frame <= 130)
	{
		stringstream << "ROUND " << level + 1;
		drawText(1, const_cast<char *>(stringstream.str().c_str()),215,200,500,400,D3DCOLOR_XRGB(255,255,0));
	}

	fonts[1]->End();

	if (frame > 222)
	{
		if (clickedArea(click_areas[0]))
			if (!(cardexists[2] || cardexists[3]))
				selectCard(0);

		if (clickedArea(click_areas[1]))
			if (!(cardexists[5] || cardexists[6]))
				selectCard(1);

		if (clickedArea(click_areas[2]))
			if (!cardexists[8])
				selectCard(2);

		if (clickedArea(click_areas[3]))
			if (!cardexists[8])
				selectCard(3);

		if (clickedArea(click_areas[4]))
			if (!(cardexists[8] || cardexists[9]))
				selectCard(4);

		if (clickedArea(click_areas[5]))
			if (!(cardexists[10] || cardexists[11]))
				selectCard(5);

		if (clickedArea(click_areas[6]))
			if (!(cardexists[11] || cardexists[12]))
				selectCard(6);

		if (clickedArea(click_areas[8]))
			if (!(cardexists[13] || cardexists[14]))
				selectCard(8);

		if (clickedArea(click_areas[10]))
			if (!(cardexists[15] || cardexists[16]))
				selectCard(10);

		if (clickedArea(click_areas[11]))
			if (!(cardexists[16] || cardexists[17]))
				selectCard(11);

		if (clickedArea(click_areas[12]))
			if (!cardexists[18])
				selectCard(12);

		if (clickedArea(click_areas[13]))
			if (!(cardexists[17] || cardexists[18]))
				selectCard(13);

		if (clickedArea(click_areas[14]))
			selectCard(14);

		if (clickedArea(click_areas[15]))
			selectCard(15);

		if (clickedArea(click_areas[16]))
			if (!(cardexists[18] || cardexists[19]))
				selectCard(16);

		if (clickedArea(click_areas[17]))
			if (!cardexists[19])
				selectCard(17);

		if (clickedArea(click_areas[18]))
			if (!cardexists[21])
				selectCard(18);

		if (clickedArea(click_areas[19]))
			if (!cardexists[21])
				selectCard(19);

		if (clickedArea(click_areas[20]))
			selectCard(20);

		if (clickedArea(click_areas[21]))
			selectCard(21);

		if (clickedArea(click_areas[7]))
			selectCard(7);

		if (clickedArea(click_areas[22]))
			selectCard(22);
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
		playSnd("FLIPCARD.WAV");
		break;
	}

	drawing->End();

	d3ddev->EndScene();

	d3ddev->Present(NULL, NULL, NULL, NULL);

	curnum = 0;

	LMB = false;

	if (tick != 0 && !GetAsyncKeyState(VK_TAB))
		Sleep(tick);

	frame++;
}

void cleanD3D(void)
{
	d3ddev->Release();
	d3d->Release();
	exit(score);
}