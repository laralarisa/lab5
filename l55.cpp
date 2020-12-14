#include <chrono>
#include <opencv2/opencv.hpp>
#include <vector>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
using namespace cv;
using std::vector;

int top = 120;
int base = 400;
int height = 170;
int maxtop = 500;
int maxbase = 460;
int maxheight = 500;




int main() {
  VideoCapture cap("solidWhiteRight.mp4");

  namedWindow("Road");


  // создаем ползунки
  createTrackbar("Top", "Road", &top, maxtop);
  createTrackbar("Bottom", "Road", &base, maxbase);
  createTrackbar("Height", "Road", &height, maxheight);

  while (1) {

    Mat src;
    cap >> src;

    if (src.empty()) {
      fprintf(stdout, "End of video\n");
      break;
    }

  Mat dst;
  src.copyTo(dst);
  Point2f src_vertices[4];
  src_vertices[0] = Point(488 - top, maxheight - height);
  src_vertices[1] = Point(488 + top, maxheight - height);
  src_vertices[2] = Point(505 + base, src.rows - 1);
  src_vertices[3] = Point(505 - base, src.rows - 1);

  Point2f dst_vertices[4];
  dst_vertices[0] = Point(0, 0);
  dst_vertices[1] = Point(dst.cols, 0);
  dst_vertices[2] = Point(dst.cols, dst.rows);
  dst_vertices[3] = Point(0, dst.rows);

  // сегментируем внутреннее пространство трапеции в новую матрицу
  Mat M = getPerspectiveTransform(src_vertices, dst_vertices);
  warpPerspective(src, dst, M, dst.size());

  // рисуем эту трапецию
  line(src, Point(488 - top, maxheight - height),
       Point(488 + top, maxheight - height), Scalar(0, 255, 0), 1);
  line(src, Point(488 + top, maxheight - height),
       Point(505 +base, src.rows - 1), Scalar(0, 255, 0), 1);
  line(src, Point(505 + base, src.rows - 1), Point(505 - base, src.rows - 1),
       Scalar(0, 255, 0), 1);
  line(src, Point(505 - base, src.rows - 1),
       Point(488 - top, maxheight - height), Scalar(0, 255, 0), 1);

  Mat bin;
  dst.copyTo(bin);

  // бинаризуем матрицу полученную из трапеции
  vector<Point2f> centers;
  cvtColor(bin, bin, COLOR_BGR2GRAY);
  threshold(bin, bin, 180, 255, THRESH_BINARY);
  
  Rect window(0, 0, 80, 80);
  vector<Point2f> nonZeros;

  for (window.y = 0; window.y + 80 <= bin.rows; window.y += 80) {
    for (window.x = 0; window.x + 40 < bin.cols; window.x += 40) {
      Mat clice = bin(window);
      vector<Point2f> roi;

      //ищем пиксели, на которых есть разметка
      findNonZero(clice, roi);
      if (!roi.empty()) {
          //сдвигаем координаты по икс и сохраняем в массив
        for (unsigned i = 0; i < roi.size(); ++i) roi[i].x += window.x;
        nonZeros.insert(nonZeros.end(), roi.begin(), roi.end());
      } else if (!nonZeros.empty()) {
          //ищем середины координат найденных разметок
        centers.push_back(
            Point((nonZeros.front().x + nonZeros.back().x) / 2,
                  window.y + (nonZeros.front().y + nonZeros.back().y) / 2));
        nonZeros.clear();
      }
    }
    //выполняем проверку если разметка оказалась на правой границе изображения
    if (!nonZeros.empty()) {
      centers.push_back(
          Point((nonZeros.front().x + nonZeros.back().x) / 2,
                window.y + (nonZeros.front().y + nonZeros.back().y) / 2));
      nonZeros.clear();
    }
  }

  // находим центры разметок на исходной матрице
  Mat reverseM = getPerspectiveTransform(dst_vertices, src_vertices);
  perspectiveTransform(centers, centers, reverseM);

  // отрисовываем их
  for (auto it : centers) {
    circle(src, it, 3, Scalar(0, 255, 0), FILLED);
  }


    //выводим полученные изображения
  imshow("Road", src);
  imshow("Trapeze", dst);
  
  if (waitKey(10) == 27)
	break;
  }

  cap.release();
  return 0;
}

