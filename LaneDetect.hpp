#pragma once
#include <iostream>
#include <opencv2\opencv.hpp>

cv::Mat laneCvtHls(cv::Mat img);
cv::Mat laneWhiteAndYellow(cv::Mat imgHls);
cv::Mat laneRegionOfInterest(cv::Mat img);
cv::Mat laneCannyEdge(cv::Mat img); // gray img
std::vector<cv::Vec4i> laneHoughLine(cv::Mat img);
std::vector<cv::Vec2f> calcAvgSlope(std::vector<cv::Vec4i> houghLines);
cv::Vec4i slopeToPoint(int y1, int y2, cv::Vec2f lineValues);
std::vector<cv::Vec4i> laneCreateLanes(int rows, std::vector<cv::Vec4i> houghLines);
cv::Mat laneDrawLanes(cv::Mat img, std::vector<cv::Vec4i> lanes, cv::Scalar laneColor = cv::Scalar(0, 0, 255), short laneBold = 10);
int laneFindingCenterPoint(cv::Mat img, std::vector<cv::Vec4i> lanes);