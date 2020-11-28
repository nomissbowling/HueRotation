/*
  HueRotation.cpp

  test with OpenCV 3.4.12 https://github.com/opencv/opencv/releases/tag/3.4.12
    x64/vc15/bin
      opencv_world3412.dll
      opencv_ffmpeg3412_64.dll

  use Microsoft Visual Studio 2017
  x64 compiler/linker
  "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Tools\MSVC\14.16.27023\bin\Hostx64\x64\cl.exe"
   -source-charset:utf-8 -execution-charset:utf-8
   -EHsc -Fe..\bin\HueRotation.exe ..\src\HueRotation.cpp
   -I..\include
   -IC:\OpenCV3\include
   -link
   /LIBPATH:C:\OpenCV3\x64\vc15\lib
   /LIBPATH:"C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Tools\MSVC\14.16.27023\lib\x64"
   /LIBPATH:"C:\Program Files (x86)\Windows Kits\10\Lib\10.0.17763.0\ucrt\x64"
   /LIBPATH:"C:\Program Files (x86)\Windows Kits\10\Lib\10.0.17763.0\um\x64"
   opencv_world3412.lib
  del ..\bin\HueRotation.obj
*/

#include "HueRotation.h"

using namespace huerotation;

string drift(int ac, char **av)
{
  vector<string> wn({"original", "gray", "Hue", "Alpha"});
  for(vector<string>::iterator i = wn.begin(); i != wn.end(); ++i)
    cv::namedWindow(*i, CV_WINDOW_AUTOSIZE | CV_WINDOW_FREERATIO);
  int cam_id = 1; // 0; // may be 'ManyCam Virtual Webcam'
  int width = 640, height = 480, fourcc;
  double fps = 30.0;
  cv::VideoCapture cap(cv::CAP_DSHOW + cam_id); // use DirectShow
  if(!cap.isOpened()) return "error: open camera";
  if(!cap.set(cv::CAP_PROP_FRAME_WIDTH, width)) cout << "width err" << endl;
  if(!cap.set(cv::CAP_PROP_FRAME_HEIGHT, height)) cout << "height err" << endl;
  if(!cap.set(cv::CAP_PROP_FPS, fps)) cout << "fps err" << endl;
  fourcc = cv::VideoWriter::fourcc('m', 'p', '4', 'v');
  bool col = true;
  cv::VideoWriter wr("HueRot.mp4", fourcc, fps, cv::Size(width, height), col);
  int cnt = 0;
  cv::Mat frm;
  while(cap.read(frm)){
    cv::imshow(wn[0], frm);
    cv::Mat gr, hsv;
    cv::cvtColor(frm, gr, CV_BGR2GRAY);
    cv::GaussianBlur(gr, gr, cv::Size(17, 17), 1.5, 1.5);
    cv::cvtColor(gr, gr, CV_GRAY2BGR);
    cv::imshow(wn[1], gr);
    vector<cv::Mat> pl; // B G R planes
    cv::split(gr, pl);
    for(int j = 0; j < gr.rows; ++j){
      uchar *b = pl[0].ptr<uchar>(j);
      uchar *g = pl[1].ptr<uchar>(j);
      uchar *r = pl[2].ptr<uchar>(j);
      for(int i = 0; i < gr.cols; ++i){
        uchar s = 76, e = 255, hue = b[i] - 76; // 76-255 -> 0-179
        //uchar s = 211, e = 255, hue = 4 * (b[i] - 211); // 211-255 -> 0-179
        //uchar s = 210, e = 254, hue = 4 * (b[i] - 210); // 210-254 -> 0-179
        //uchar s = 232, e = 254, hue = 8 * (b[i] - 232); // 232-254 -> 0-179
        bool f = (b[i] >= s) && (b[i] <= e);
        b[i] = cv::saturate_cast<uchar>((cnt + hue) % 180); // H
        g[i] = cv::saturate_cast<uchar>(f ? 255 : 0); // S
        r[i] = cv::saturate_cast<uchar>(f ? 255 : 0); // V
      }
    }
    cv::merge(pl, hsv);
    cv::cvtColor(hsv, hsv, CV_HSV2BGR); // assume BGR as HSV
    cv::imshow(wn[2], hsv); // hsv.channels() == 3
    cv::Mat im(gr.rows, gr.cols, CV_8UC4);
    vector<cv::Mat> pa; // B G R A planes
    cv::split(im, pa);
    cv::split(hsv, pl);
    pa[0] = pl[0];
    pa[1] = pl[1];
    pa[2] = pl[2];
    pa[3] = 255;
    cv::merge(pa, im);
    wr << im;
    cv::imshow(wn[3], im);
    ++cnt;
    int k = cv::waitKey(1); // 1ms > 15ms ! on Windows
    if(k == 'q' || k == '\x1b') break;
  }
  wr.release();
  cap.release();
  cv::destroyAllWindows();
  return "ok";
}

int main(int ac, char **av)
{
  cout << drift(ac, av) << endl;
  return 0;
}
