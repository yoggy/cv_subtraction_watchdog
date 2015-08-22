//
// cv_subtraction_watchdog.exe
//
#pragma once

#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#pragma warning(disable: 4819)

#ifdef _DEBUG
#pragma comment(lib, "opencv_world300d.lib")
#else
#endif
#pragma comment(lib, "opencv_world300.lib")

#include <iostream>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <mmsystem.h>
#pragma comment(lib,"winmm.lib")

char *window_name = "cv_subtraction_watchdog";

#define INVALID_CV_RECT cv::Rect(-1, -1, -1, -1)
cv::Rect watch_roi;
cv::Rect drag_roi;
bool is_dragged = false;
float subtraction_ratio = 0.0f;
bool detect_subtraction = false;

cv::Point drag_sp;

cv::Mat capture_img;
cv::Mat old_capture_img;

void clear()
{
	watch_roi = INVALID_CV_RECT;
	drag_roi = INVALID_CV_RECT;
	capture_img.release();
	old_capture_img.release();

	subtraction_ratio = 0.0f;
	detect_subtraction = false;
}

cv::Rect create_rect(const cv::Point &p0, const cv::Point &p1)
{
	int x, y, w, h;

	if (p0.x < p1.x) {
		x = p0.x;
		w = p1.x - p0.x;
	}
	else {
		x = p1.x;
		w = p0.x - p1.x;
	}

	if (p0.y < p1.y) {
		y = p0.y;
		h = p1.y - p0.y;
	}
	else {
		y = p1.y;
		h = p0.y - p1.y;
	}

	return cv::Rect(x, y, w, h);
}

void on_mouse(int event, int x, int y, int, void* userdata)
{
	if (event & CV_EVENT_LBUTTONDOWN) {
		is_dragged = true;
		drag_sp = cv::Point(x, y);

		watch_roi = INVALID_CV_RECT;
		drag_roi = create_rect(drag_sp, drag_sp);
	}
	else if (event & CV_EVENT_LBUTTONUP) {
		is_dragged = false;

		cv::Point p = cv::Point(x, y);

		drag_roi = INVALID_CV_RECT;

		watch_roi = create_rect(drag_sp, p);
		if (watch_roi.width == 0 || watch_roi.height == 0) {
			watch_roi = INVALID_CV_RECT;
		}
	}

	if (is_dragged == true) {
		cv::Point p = cv::Point(x, y);
		drag_roi = create_rect(drag_sp, p);
	}
}

void play_warning_sound()
{
	static DWORD last_play_tick = 0;

	if (::GetTickCount() - last_play_tick < 3 * 1000) {
		return;
	}

	last_play_tick = ::GetTickCount();
	
	::PlaySound("warning.wav", NULL, SND_FILENAME | SND_ASYNC);

}

void process()
{
	cv::Mat tmp_img = capture_img;
	if (capture_img.empty()) return;
	if (watch_roi == INVALID_CV_RECT) return;

	if (old_capture_img.empty()) {
		capture_img.copyTo(old_capture_img);
		return;
	}

	cv::Mat now_img, old_img;
	cv::cvtColor(capture_img(watch_roi), now_img, cv::COLOR_BGR2GRAY);
	cv::cvtColor(old_capture_img(watch_roi), old_img, cv::COLOR_BGR2GRAY);

	cv::Mat diff_img;
	cv::absdiff(old_img, now_img, diff_img);

	cv::Mat bin_img;
	cv::threshold(diff_img, bin_img, 30, 255, cv::THRESH_BINARY);

	int white_pixel_count = cv::countNonZero(bin_img);
	int total_pixel_count = bin_img.cols * bin_img.rows;

	float subtraction_ratio = (float)white_pixel_count / (float)total_pixel_count;
	std::cout << "subtraction_ratio=" << (int)(subtraction_ratio * 100) << "%" << std::endl;

	if (subtraction_ratio >= 0.1f) {
		if (detect_subtraction == false) {
			// edge trigger action
			play_warning_sound();
		}

		detect_subtraction = true;
	}
	else {
		detect_subtraction = false;
	}

	capture_img.copyTo(old_capture_img);

	return;
}

void debug_draw()
{
	if (capture_img.empty()) return;

	cv::Mat canvas_img;
	capture_img.copyTo(canvas_img);

	if (drag_roi != INVALID_CV_RECT) {
		cv::rectangle(canvas_img, drag_roi, cv::Scalar(255, 0, 255), 2);
	}

	if (watch_roi != INVALID_CV_RECT) {
		if (detect_subtraction) {
			cv::rectangle(canvas_img, watch_roi, cv::Scalar(0, 0, 255), 3);
		}
		else {
			cv::rectangle(canvas_img, watch_roi, cv::Scalar(0, 255, 0), 2);
		}
	}

	cv::imshow(window_name, canvas_img);
}

int main(int argc, char* argv[])
{
	cv::VideoCapture capture;
	if (capture.open(0) == false) {
		std::cerr << "error : capture.open() failed..." << std::endl;
		return -1;
	}
	capture >> capture_img;
	clear();

	cv::namedWindow(window_name);
	cv::setMouseCallback(window_name, on_mouse);

	while (true) {
		capture >> capture_img;

		process();
		debug_draw();

		int c = cv::waitKey(1);
		if (c == 'c') {
			clear();
		}
		else if (c == 27) {
			break;
		}
	}
	cv::destroyAllWindows();

	return 0;
}

