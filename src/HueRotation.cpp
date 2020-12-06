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

cv::Mat mkGammaLUT(double g)
{
  cv::Mat lut(1, 256, CV_8U);
  uchar *p = lut.data;
  for(int i = 0; i < lut.cols; ++i)
    p[i] = cv::saturate_cast<uchar>(255.0 * pow(i / 255.0, g));
  return lut;
}

cv::Mat mkRotGammaLUT(double g)
{
  cv::Mat lut(1, 256, CV_8U);
  uchar *p = lut.data;
  for(int i = 0; i < lut.cols; ++i)
    p[i] = 255 - cv::saturate_cast<uchar>(255.0 * pow((255 - i) / 255.0, g));
  return lut;
}

string drift(int ac, char **av)
{
  // cv::Mat gammaLUT = mkGammaLUT(2.2); // (1 / 2.2);
  cv::Mat gammaLUT = mkRotGammaLUT(2.2); // (1 / 2.2);
  vector<string> wn({"original", "gray", "Hue", "Alpha"});
  for(vector<string>::iterator i = wn.begin(); i != wn.end(); ++i)
    cv::namedWindow(*i, CV_WINDOW_AUTOSIZE | CV_WINDOW_FREERATIO);
  int cam_id = 0; // 1; // 0; // may be 'ManyCam Virtual Webcam'
  int width = 640, height = 480, fourcc;
  double fps = 30.0;
  cv::VideoCapture cap(cv::CAP_DSHOW + cam_id); // use DirectShow
  if(!cap.isOpened()) return "error: open camera";
  // cout << cv::getBuildInformation() << endl;
  if(!cap.set(cv::CAP_PROP_FRAME_WIDTH, width)) cout << "width err" << endl;
  if(!cap.set(cv::CAP_PROP_FRAME_HEIGHT, height)) cout << "height err" << endl;
  if(!cap.set(cv::CAP_PROP_FPS, fps)) cout << "fps err" << endl;
  // fourcc = cv::VideoWriter::fourcc('M', 'J', 'P', 'G');
  // fourcc = cv::VideoWriter::fourcc('m', 'p', '4', 'v');
  // fourcc = cv::VideoWriter::fourcc('X', 'V', 'I', 'D');
  // fourcc = cv::VideoWriter::fourcc('D', 'I', 'V', 'X');
  // fourcc = cv::VideoWriter::fourcc('X', '2', '6', '4');
  fourcc = 0x00000020; // fallback tag
  bool col = true;
  cv::VideoWriter wr("HueRot.mp4", fourcc, fps, cv::Size(width, height), col);
  int cnt = 0;
  cv::Mat frm;
  while(cap.read(frm)){
    cv::imshow(wn[0], frm);
    cv::Mat gr, hsv;
    cv::cvtColor(frm, gr, CV_BGR2GRAY);
    cv::GaussianBlur(gr, gr, cv::Size(7, 7), 1.5, 1.5);
    cv::LUT(gr, gammaLUT, gr);
    cv::imshow(wn[1], gr);
    cv::cvtColor(frm, hsv, CV_BGR2HSV);
    vector<cv::Mat> pl; // H S V planes
    cv::split(hsv, pl);
    uchar z = (uchar)(144 + 48 * sin((cnt % 360) * CV_PI / 180)); // M_PI
    uchar *u = gr.data;
    for(int j = 0; j < hsv.rows; ++j){
      uchar *h = pl[0].ptr<uchar>(j);
      uchar *s = pl[1].ptr<uchar>(j);
      uchar *v = pl[2].ptr<uchar>(j);
      for(int i = 0; i < hsv.cols; ++i){
#if 1
        int x = i - hsv.cols / 2, y = j - hsv.rows / 2;
        int w = x * x + y * y, p = hsv.cols * hsv.rows / 16;
        int q = p / 4;
        bool f = w > p || w < q;
        uchar hue = h[i];
        if(!f){
#if 0
          uchar t = (uchar)(179 * (w - q) / (p - q));
#else
          uchar t = (uchar)(179 * (sqrt(w) - sqrt(q)) / (sqrt(p) - sqrt(q)));
#endif
          hue = 179 - ((t + 15) % 180);
        }
        h[i] = cv::saturate_cast<uchar>(hue % 180); // H
        s[i] = cv::saturate_cast<uchar>(f ? s[i] : z); // S
        // v[i] = cv::saturate_cast<uchar>(v[i]); // V
#else
        // bool f = ( >= s) && ( <= e);
        uchar t = *u++;
        bool f = t >= 166;
        uchar hue = f ? 2 * (t - 166) : 0;
        h[i] = cv::saturate_cast<uchar>((179 - hue) % 180); // H
        s[i] = cv::saturate_cast<uchar>(f ? s[i] : 0); // S
        v[i] = cv::saturate_cast<uchar>(v[i]); // V
#endif
      }
    }
    cv::merge(pl, hsv);
    cv::cvtColor(hsv, hsv, CV_HSV2BGR); // assume BGR as HSV
    cv::imshow(wn[2], hsv); // hsv.channels() == 3
#if 0
    cv::Mat im(frm.rows, frm.cols, CV_8UC4);
    vector<cv::Mat> pa; // B G R A planes
    cv::split(im, pa);
    cv::split(hsv, pl);
    pa[0] = pl[0];
    pa[1] = pl[1];
    pa[2] = pl[2];
    pa[3] = 255;
    cv::merge(pa, im);
#else
    cv::Mat im(frm);
    hsv.copyTo(im, pl[1]);
#endif
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
