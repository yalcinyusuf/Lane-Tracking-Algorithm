#include <iostream>
#include <opencv2/opencv.hpp>
#include "LaneDetect.hpp"
#include "Desktop.h"
#include <thread>
#include <queue>

using namespace std;
using namespace cv;

int main()
{
	Mat imgSrc = imread("test.jpg");
	if (imgSrc.empty())
	{
		cout << "Resim yüklenemedi!\n";
		system("Pause");
		return 1;
	}
	//imshow("src", imgSrc);
	//waitKey();

	// istege bagli donusum istersen direkt gri formatta calisabilirsin. Daha cok golgeler oldugunda bu donusum isimize yariyor.
	Mat imgHls = laneCvtHls(imgSrc);

	Mat imgWhiteAndYellow = laneWhiteAndYellow(imgHls);
	
	Mat imgRoi = laneRegionOfInterest(imgWhiteAndYellow);

	Mat imgCanny = laneCannyEdge(imgRoi);

	vector<Vec4i> lines = laneHoughLine(imgCanny);

	if (lines.size() > 0)
	{
		vector<Vec4i> lanes = laneCreateLanes(imgCanny.rows, lines);
		Mat imgLanes = laneDrawLanes(imgSrc, lanes);

		Mat imgDst;
		addWeighted(imgSrc, 1, imgLanes, 0.6, 0, imgDst);
		//imshow("dst", imgDst);
		//waitKey();

		int dirVal = laneFindingCenterPoint(imgDst, lanes);
		cout << "DirVal: " << dirVal << endl;
	}
	else
	{
		//imshow("dst", imgSrc);
		//waitKey();
	}

	//video
	VideoCapture video;
	Mat frame, frameNonCrop;

	vector<Vec4i> lanes;
	Mat imgLanes, imgDst;
#if 0
	if (!video.open("video.mp4"))
	{
		cout << "Video Acilamadi!" << endl;
		system("Pause");
		return 1;
	}

	while (true)
	{
		video.read(frame);

		if (frame.empty())
			break;

		//---processing
		imgHls = laneCvtHls(frame);

		imgWhiteAndYellow = laneWhiteAndYellow(imgHls);

		imgRoi = laneRegionOfInterest(imgWhiteAndYellow);

		imgCanny = laneCannyEdge(imgRoi);

		lines = laneHoughLine(imgCanny);

		if (lines.size() > 0)
		{
			lanes = laneCreateLanes(imgCanny.rows, lines);
			imgLanes = laneDrawLanes(frame, lanes);

			imgDst;
			addWeighted(frame, 1, imgLanes, 0.6, 0, imgDst);
			//imshow("dst", imgDst);
			//waitKey();

			int dirVal = laneFindingCenterPoint(imgDst, lanes);
			cout << "DirVal: " << dirVal << endl;
		}
		else
		{
			imshow("dst", frame);
			//waitKey();
		}
		//---

		if (waitKey(1) == 27) // esc
			break;
	}

	video.release();
#else
	HWND window = GetDesktopWindow(); //FindWindow(0, "Euro Truck Simulator 2");

	thread t1(keySenderThread, window, ileri, 3000, 1);

	int i = 0;
	while (true)
	{
		frameNonCrop = hwnd2mat(window);
		frame = frameNonCrop(Rect(0, 0, 1024, 768));
		//imshow("src", frame);
		//waitKey();

		if (frame.empty())
			break;

		//---processsing
		imgHls = laneCvtHls(frame);
		imgWhiteAndYellow = laneWhiteAndYellow(imgHls);
		imgRoi = laneRegionOfInterest(imgWhiteAndYellow);
		imgCanny = laneCannyEdge(imgRoi);
		lines = laneHoughLine(imgCanny);

		if (lines.size() > 0)
		{
			lanes = laneCreateLanes(imgCanny.rows, lines);
			imgLanes = laneDrawLanes(frame, lanes);

			imgDst;
			addWeighted(frame, 1, imgLanes, 0.6, 0, imgDst);
			//imshow("dst", imgDst);
			//waitKey();

			int dirVal = laneFindingCenterPoint(imgDst, lanes);
			cout << "DirVal: " << dirVal << endl;

			static int turnControl = 0;

			if (dirVal < 0 && abs(dirVal) < 400)
			{
				if (taskQueue.size() < 8)
					taskQueue.push({ sol, abs(dirVal) / 6 });
				else
					while (++i < 5)
						taskQueue.pop();

				if (++turnControl > 3)
				{
					taskQueue.push({ sag, 65 });
					turnControl = 0;
				}
			}
			else if (dirVal > 0 && dirVal < 400)
			{
				if (taskQueue.size() < 8)
					taskQueue.push({ sag , dirVal / 6 });
				else
					while (++i < 5)
						taskQueue.pop();

				if (--turnControl < -3)
				{
					taskQueue.push({ sol, 65 });
					turnControl = 0;
				}
			}

			i = 0;
		}
		else
		{
			resize(frame, frame, Size(640, 480));
			imshow("dst", frame);
			//waitKey();
		}
		//---

		if (waitKey(40) == 27) // esc
			break;
	}
	//finish
	taskQueue.push({ 0, 0 }); // Desktop.h içerisinde threadleri durdurmak icin sifilari giriyoruz.

	t1.join();
#endif
	destroyAllWindows();

	return 0;
}