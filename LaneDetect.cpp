#include <opencv2/opencv.hpp>
#include "LaneDetect.hpp"

using namespace std;
using namespace cv;


vector<Vec2f> g_lanes;

// Beyaz ve Sari renklerin daha çok anlasilmasini saglayan fonksiyon
Mat laneCvtHls(Mat img)
{
	Mat imgHls;
	cvtColor(img, imgHls, COLOR_BGR2HLS); // imgyi bgr'dan hls'ye cevirerek imgHls uzerine atama
	//imshow("imghls", imgHls);
	//waitKey();
	return imgHls;
}

Mat laneWhiteAndYellow(Mat imgHls)
{
	// Hue degerleri HSV'de ve HLS'de benzerdir.

	Mat white, yellow, mask, dst;

	//find yellow
	inRange(imgHls, Scalar(22, 100, 100), Scalar(38, 255, 255), yellow); //H  L S , Hue, Light, Saturation
	//imshow("yellow", yellow);
	//waitKey();

	//find white
	inRange(imgHls, Scalar(0, 190, 5), Scalar(255, 255, 180), white); //H  L S
	//imshow("white", white);
	//waitKey();


	//sari ve beyaz renklerden olusan pikselleri mask uzerinde topluyoruz.
	bitwise_or(yellow, white, mask);
	//imshow("mask", mask);
	//waitKey();

	// Sadece imgHls, imgHls andledigimiz zaman birsey olmayacak ancak mask ekledigimizde sadece sari ve beyaz piksellerin oldugu yerler(istedigimiz yerler) alinmis olacak.
	bitwise_and(imgHls, imgHls, dst, mask);
	//imshow("dst", dst);
	//waitKey();

	// İlgili renklerin buludugu gerekli pikselleri aldiktan sonra gereksiz 3 kanalli renkten tek kanalliya donusturebiliriz.
	// Cunku piksel islemleri icin renk turu onemli degil artik sadece orada bir nokta oldugunu gosteren bir kanalli renk uzayi olmasi yeterli.
	cvtColor(dst, dst, COLOR_BGR2GRAY);
	//imshow("gray", dst);
	//waitKey();

	return dst;
}

Mat laneRegionOfInterest(Mat img)
{
	int cols = img.cols;
	int rows = img.rows;

	// Noktalar ne kadar azsa daha keskin poligon olur ne kadar fazlaysa o kadar esnek ıstegınıze bagli.
	
	int x1 = cols * 0, y1 = rows * 1;
	int x2 = cols * 0.10, y2 = rows * 0.8;
	int x3 = cols * 0.25, y3 = rows * 0.6;
	int x4 = cols * 0.75, y4 = rows * 0.6;
	int x5 = cols * 0.90, y5 = rows * 0.8;
	int x6 = cols * 1, y6 = rows * 1;
	
	//Eger kamera goruntulerini yerden ve aracın en onunden almak istiyorsak asagidaki degerler
	/*
	int x1 = cols * 0, y1 = rows * 1;
	int x2 = cols * 0.15, y2 = rows * 0.9;
	int x3 = cols * 0.35, y3 = rows * 0.8;
	int x4 = cols * 0.65, y4 = rows * 0.8;
	int x5 = cols * 0.85, y5 = rows * 0.9;
	int x6 = cols * 1, y6 = rows * 1;
	*/

	Mat mask = img.zeros(Size(cols, rows), CV_8UC1);

	Point points[6]
	{
		Point(x1, y1),
		Point(x2, y2),
		Point(x3, y3),
		Point(x4, y4),
		Point(x5, y5),
		Point(x6, y6)
	};

	fillConvexPoly(mask, points, 6, Scalar(255)); // 2 tane constructar var fillConvexPoly adinda
	//imshow("poly", mask);
	//waitKey();

	bitwise_and(img, mask, mask);
	//imshow("roi", mask);
	//waitKey();

	return mask;
}

Mat laneCannyEdge(Mat img) // gray img
{
	Mat blur;
	GaussianBlur(img, blur, Size(15, 15), 0);
	//imshow("blur", blur);
	//waitKey();

	Mat canny, tmp;
	// blurdan gelen goruntuleri veriyoruz ki bu şekilde parazitleri temizlemiş canny'nin isini kolaylaştirmis oluyoruz.
	// sitesinde threshold degeri 1'e 3 ya da 1'e 2 olmalidir yaziyor. (50, 150 yaptim ben) 255'i gecmeyecek sekilde olmalıdır 2 degerde
	/* CannyEdgeDetection algoritmasi kenar tespiti yaptiği icin bu esik degerlerine uyacak sekilde renk degisimine sahip pikselleri alarak
	onlari nokta olarak kabul ediyor. Seritlerin tespiti cok kolay oldugu durumlarda 25 50 gibi dusuk degerler tercih edilebilir.*/
	Canny(blur, canny, 50, 150);
	resize(canny, tmp, Size(640, 480));
	imshow("canny", tmp);
	//waitKey();

	return canny;
}

vector<Vec4i> laneHoughLine(Mat img) // 4 tane integer tutan vector
{
	vector<Vec4i> lines;
	//HoughLines klasik olan standart fonksiyon.
	//HoughLinesP probabilistic yani olasılıksal daha iyi sonuc veriyor klasik olana göre. Tahminsel islemleri de dahil ediyor bu yüzden yuksek sonuc veriyor.
	//Bu degerler gorus acimiza gore degısebilir. 100 degeri(maxLineGap parametresi) max 100 piksellik bosluktaki cizgileri kabul ediyor.
	//bir önceki degeri(minLineGap parametresi) min 20 piksel uzunlugundaki nokta birlesimlerini cizgi kabul ediyor.
	HoughLinesP(img, lines, 1, CV_PI / 180, 40, 20, 100);

	// Hough Line'dan donen cizgiler icin
	Mat tmp = img.zeros(Size(img.cols, img.rows), CV_8UC3); // Cizgileri renkli gormek icin yeni mat nesnesi ve 3 kanallı olarak ayarliyoruz
	for (int i = 0; i < lines.size(); i++)
		line(tmp, Point(lines[i][0], lines[i][1]), Point(lines[i][2], lines[i][3]), Scalar(0, 0, 255));
	//imshow("lines", tmp);
	//waitKey();

	return lines;
}

vector<Vec2f> calcAvgSlope(vector<Vec4i> houghLines)
{
	float slope, intercept,
		sumOfSlopeLeft = 0.0f, sumOfInterceptLeft = 0.0f, sumOfSlopeRight = 0.0f, sumOfInterceptRight = 0.0f;

	Point a, b;
	vector<Vec2f> leftLines, rightLines;
	Vec2f avgValuesLeft, avgValuesRight;
	for (auto line : houghLines)
	{
		a = Point(line[0], line[1]);
		b = Point(line[2], line[3]);
		if (a.x == b.x) // x1 == x2 continues slope! sonsuz deger cikar
			continue;
		slope = (float)(b.y - a.y) / (b.x - a.x); // (y2-y1)/(x2-x1)
		intercept = a.y - (slope * a.x);
		if (slope < 0) //left line
		{
			leftLines.push_back(Vec2f(slope, intercept));
			sumOfSlopeLeft += slope;
			sumOfInterceptLeft += intercept;
		}
		else // right line
		{
			rightLines.push_back(Vec2f(slope, intercept));
			sumOfSlopeRight += slope;
			sumOfInterceptRight += intercept;
		}
	}

	if (leftLines.size() > 0)
		avgValuesLeft = Vec2f(sumOfSlopeLeft / leftLines.size(), sumOfInterceptLeft / leftLines.size());
	else
		avgValuesLeft = Vec2f(0.0f, 0.0f); //left lane is cannot find

	if (rightLines.size() > 0)
		avgValuesRight = Vec2f(sumOfSlopeRight / rightLines.size(), sumOfInterceptRight / rightLines.size());
	else
		avgValuesRight = Vec2f(0.0f, 0.0f); //right lane is cannot find	

	// 2 tane serit donduruyoruz sol ve sag serit olmak uzere
	return { avgValuesLeft, avgValuesRight }; // Vektor seklinde geri donmesi icin suslu paranteze aldık

}

// şeritlerin koordinatlarini belirleyen fonk
Vec4i slopeToPoint(int y1, int y2, Vec2f lineValues)
{
	float slope = lineValues[0];
	float intercept = lineValues[1];

	int x1, x2;
	if (slope != 0)
	{
		x1 = (int)((y1 - intercept) / slope); //y = mx+b -> x = (y-b)/m
		x2 = (int)((y2 - intercept) / slope);
	}
	else
		x1 = x2 = 0;

	return { x1, y1, x2, y2 };
}

// Seritleri olusturan fonk
// rows image'in heightini belirtiyor. houglines image'in hougLines'ini belirtiyor. // houglines laneHoughLine fonksiyonundan donen cizgiler olacak.
vector<Vec4i> laneCreateLanes(int rows, vector<Vec4i> houghLines)
{
	// 2 adet float tipinde veri tutan bir vektor
	g_lanes = calcAvgSlope(houghLines); // left avg slope and intercept, right avg slope and intercept
	// intercept degeri orjine olan uzaklıgı belirtiyor. Slope ise dogrularin egimi formulunden cikacak.

	int y1 = rows; // Goruntunun yuksekigi y1 burada goruntun en alt kismini tutacak.
	//int y2 = y1 * 0.70; // özel
	int y2 = y1 * 0.75; // goruntunun yuzde 75. piksele kadar,
	/*int y2 = y1 * 0.6; // goruntunun yuzde 60. piksele kadar, */

	Vec4i leftLane, rightLane; // serit tanımladık
	if (g_lanes[0][0] == 0 && g_lanes[0][1] == 0) // sol serit yok  lanes[0][0] = egim,   lanes[0][1] = intercept
		leftLane = { 0, 0, 0, 0 }; // x1,y1,x2,y2 degerleri
	else
		leftLane = slopeToPoint(y1, y2, g_lanes[0]); // y1 y2, eğim ve kesisim degerlerimizi vererek birleştirerek x1 x2 hesaplayıp bize geri donduruyor.

	if (g_lanes[1][0] == 0 && g_lanes[1][1] == 0) // sag serit yok  
		rightLane = { 0, 0, 0, 0 }; // x1,y1,x2,y2 degerleri
	else
		rightLane = slopeToPoint(y1, y2, g_lanes[1]);

	return { leftLane, rightLane };
}

// Seritleri cizen fonk
Mat laneDrawLanes(Mat img, vector<Vec4i> lanes, Scalar laneColor, short laneBold)
{
	Mat laneImg = img.zeros(img.rows, img.cols, img.type());
	for (int i = 0; i < lanes.size(); i++)
		if (lanes[i][0] == 0 && lanes[i][1] == 0)
			continue;
		else
			line(laneImg, Point(lanes[i][0], lanes[i][1]), Point(lanes[i][2], lanes[i][3]), laneColor, laneBold);
	//imshow("lanes", laneImg);
	//waitKey();

	return laneImg;
}

int laneFindingCenterPoint(Mat img, std::vector<Vec4i> lanes)
{
	Vec4i leftCenterLine, rightCenterLine, centerLine, screenCenterLine, screenHorizontalLine;
	int directionMltp = 0, directionVal = 0;

	if ((lanes[0][0] == 0 && lanes[0][1] == 0) || (lanes[1][0] == 0 && lanes[1][1] == 0))
	{ // tek serit var ise cizim yapma, donus degeri ayri hesaplanacak
		Mat tmpImg;
		resize(img, tmpImg, Size(640, 480));
		imshow("dst", tmpImg);
		//imshow("dst", img);
		//waitKey();
		float refPoint = 0.1f;

		if ((lanes[0][0] == 0 && lanes[0][1] == 0)) // sol serit yok ise
		{
			if (lanes[1][0] > (img.cols * (1.0f - refPoint))) // alttaki bolgelerdeki noktalar x1, usttekiler x2
			{
				directionVal = abs(lanes[1][0] - img.cols * (1.0f - refPoint));
				directionMltp = 1;
				return ((directionVal * directionMltp) * 1000) / img.cols;
			}
			else if (lanes[1][0] < (img.cols * (1.0f - refPoint * 2.0f)))
			{
				directionVal = abs(img.cols * (1.0f - refPoint * 2.0f) - lanes[1][0]);
				directionMltp = -1;
				return ((directionVal * directionMltp) * 1000) / img.cols;
			}
		}
		else if ((lanes[1][0] == 0 && lanes[1][1] == 0)) // sag serit yok ise
		{
			if (lanes[0][0] < (img.cols * refPoint))
			{
				directionVal = abs(img.cols * refPoint - lanes[0][0]);
				directionMltp = -1;
				return ((directionVal * directionMltp) * 1000) / img.cols;
			}
			else if (lanes[0][0] > (img.cols * (1.0f - refPoint * 8.0f)))
			{
				directionVal = abs(lanes[0][0] - img.cols * (1.0f - refPoint * 8.0f));
				directionMltp = 1;
				return ((directionVal * directionMltp) * 1000) / img.cols;
			}
		}

		return 0;
	}
	/* //özel
	leftCenterLine = slopeToPoint(img.rows * 0.75, img.rows * 0.80, g_lanes[0]); 
	rightCenterLine = slopeToPoint(img.rows * 0.75, img.rows * 0.80, g_lanes[1]);
	*/ 
	leftCenterLine = slopeToPoint(img.rows * 0.80, img.rows * 0.85, g_lanes[0]); //laneCreateLanes'ya y2'yi %60'tan %75'e cektigimiz icin burada 0.70,0.75 yerine 0.80,0.90'la carpiyoruz.
	rightCenterLine = slopeToPoint(img.rows * 0.80, img.rows * 0.85, g_lanes[1]);
	
	//x1=x2=(x1+x2)/2
	leftCenterLine[0] = leftCenterLine[2] = (int)((leftCenterLine[0] + leftCenterLine[2]) / 2);
	rightCenterLine[0] = rightCenterLine[2] = (int)((rightCenterLine[0] + rightCenterLine[2]) / 2);

	centerLine = { (leftCenterLine[0] + rightCenterLine[0]) / 2, leftCenterLine[1],
				   (leftCenterLine[0] + rightCenterLine[0]) / 2, leftCenterLine[3] };

	screenCenterLine = { img.cols / 2, centerLine[3], img.cols / 2, img.rows };

	if (screenCenterLine[0] < centerLine[0]) // yatay cizgi solda
	{
		screenHorizontalLine = { screenCenterLine[0], centerLine[3], centerLine[0], centerLine[3] };
		directionMltp = 1; // saga git
	}
	else // yatay cizgi sagda
	{
		screenHorizontalLine = { centerLine[0], centerLine[3], screenCenterLine[0], centerLine[3] };
		directionMltp = -1; // sola git
	}

	directionVal = screenHorizontalLine[2] - screenHorizontalLine[0];

	vector<Vec4i> lines = { leftCenterLine, rightCenterLine, centerLine, screenCenterLine, screenHorizontalLine };
	Mat centerLinesImg = laneDrawLanes(img, lines, Scalar(0, 255, 0), 3);
	addWeighted(img, 1, centerLinesImg, 0.8, 0, img);
	resize(img, img, Size(640, 480));
	imshow("dst", img); // 2 şerit varsa kismi bura
	//waitKey();

	return ((directionVal * directionMltp) * 1000) / img.cols; // goruntunun genisligine gore bindelik degeri
}