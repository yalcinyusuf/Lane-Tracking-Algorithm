#pragma once
#include <Windows.h>
#include <opencv2\opencv.hpp>

std::queue<cv::Vec2i> taskQueue; // {keyValue, timeMs}

//Sendkey----------------------------------------------------------------
#define sol 0x41//A
#define sag 0x44//D
#define ileri 0x57//W
#define geri 0x53//S

void sendKey(HWND window, WORD key, short isReleased)
{
	if (window)
	{
		if (key == ileri || key == geri || key == sag || key == sol)
		{
			HWND tmp = FindWindow(0, L"Euro Truck Simulator 2");
			SetForegroundWindow(tmp); //Pencereyi Onplana Getirme
			INPUT input;
			input.type = INPUT_KEYBOARD;

			input.ki.wScan = MapVirtualKey(key, MAPVK_VK_TO_VSC);

			input.ki.dwFlags = 0; //KEYEVENTF_KEYDOWN
			input.ki.wVk = key;
			SendInput(1, &input, sizeof(INPUT));
			std::cout << "Key basildi: " << key << std::endl;

			if (isReleased)
			{
				input.ki.dwFlags = KEYEVENTF_KEYUP;
				SendInput(1, &input, sizeof(INPUT));
				std::cout << "Key birakildi: " << key << std::endl;
			}

			//HWND hwndDesktop = FindWindow(0, "My Window");
			//SetForegroundWindow(hwndDesktop);
		}
	}
	else
		std::cout << "Windows is not found!" << std::endl;
}
//Sendkey----------------------------------------------------------------

void keySenderThread(HWND window, WORD key, short timeMs, short isReleased)
{
	taskQueue.push({ key, timeMs });
	cv::Vec2i data;
	while (true)
	{
		//read
		if (taskQueue.empty())
		{
			Sleep(300);
			continue;
		}
		data = taskQueue.front(); // {key, timeMs}
		taskQueue.pop();

		std::cout << "queuesize: " << taskQueue.size() << std::endl;

		//finish control
		if (data[0] == 0)
			break;

		//do
		sendKey(window, data[0], 0); //up
		Sleep(data[1]);
		sendKey(window, data[0], 1); //down

		//sleep
		Sleep(100);
	}
}

cv::Mat hwnd2mat(HWND hwnd)
{
	HDC hwindowDC, hwindowCompatibleDC;

	int height, width, srcheight, srcwidth;
	HBITMAP hbwindow;
	cv::Mat src;
	BITMAPINFOHEADER  bi;

	hwindowDC = GetDC(hwnd);
	hwindowCompatibleDC = CreateCompatibleDC(hwindowDC);
	SetStretchBltMode(hwindowCompatibleDC, COLORONCOLOR);

	RECT windowsize;    // get the height and width of the screen
	GetClientRect(hwnd, &windowsize);

	srcheight = windowsize.bottom;
	srcwidth = windowsize.right;
	height = windowsize.bottom / 1;  //change this to whatever size you want to resize to
	width = windowsize.right / 1;

	src.create(height, width, CV_8UC4);

	// create a bitmap
	hbwindow = CreateCompatibleBitmap(hwindowDC, width, height);
	bi.biSize = sizeof(BITMAPINFOHEADER);    //http://msdn.microsoft.com/en-us/library/windows/window/dd183402%28v=vs.85%29.aspx
	bi.biWidth = width;
	bi.biHeight = -height;  //this is the line that makes it draw upside down or not
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;

	// use the previously created device context with the bitmap
	SelectObject(hwindowCompatibleDC, hbwindow);
	// copy from the window device context to the bitmap device context
	StretchBlt(hwindowCompatibleDC, 0, 0, width, height, hwindowDC, 0, 0, srcwidth, srcheight, SRCCOPY); //change SRCCOPY to NOTSRCCOPY for wacky colors !
	GetDIBits(hwindowCompatibleDC, hbwindow, 0, height, src.data, (BITMAPINFO *)&bi, DIB_RGB_COLORS);  //copy from hwindowCompatibleDC to hbwindow

																									   // avoid memory leak
	DeleteObject(hbwindow);
	DeleteDC(hwindowCompatibleDC);
	ReleaseDC(hwnd, hwindowDC);

	return src;
}