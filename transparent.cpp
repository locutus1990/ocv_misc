/**------------------------------------------------------------*
 * @file	overwriteImage.cpp
 * @brief	warpAffineを用いて画像内に別の画像を描画する
 *			（ピクチャー・イン・ピクチャー機能）
 *------------------------------------------------------------*/
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <stdlib.h>
#include <stdio.h>

using namespace std;

const char winName[]="transparent";

/**-----------------------------------------------------------*
 * @fn	DrawTransPinP
 * @brief	透過画像を重ねて描画する
 * @param[out] img_dst
 * @param[in ] transImg 前景画像。アルファチャンネル付きであること(CV_8UC4)
 * @param[in ] baseImg	背景画像。アルファチャンネル不要(CV_8UC3)
 *------------------------------------------------------------*/
 void DrawTransPinP(cv::Mat &img_dst, const cv::Mat transImg, const cv::Mat baseImg, vector<cv::Point2f> tgtPt)
{
	cv::Mat img_rgb, img_aaa, img_1ma;
	vector<cv::Mat>planes_rgba, planes_rgb, planes_aaa, planes_1ma;
	int maxVal = pow(2, 8*baseImg.elemSize1())-1;

	//透過画像はRGBA, 背景画像はRGBのみ許容。ビット深度が同じ画像のみ許容
	if(transImg.data==NULL || baseImg.data==NULL || transImg.channels()<4 ||baseImg.channels()<3 || (transImg.elemSize1()!=baseImg.elemSize1()) )
	{
		img_dst = cv::Mat(100,100, CV_8UC3);
		img_dst = cv::Scalar::all(maxVal);
		return;
	}

	//書き出し先座標が指定されていない場合は背景画像の中央に配置する
	if(tgtPt.size()<4)
	{
		//座標指定(背景画像の中心に表示する）
		int ltx = (baseImg.cols - transImg.cols)/2;
		int lty = (baseImg.rows - transImg.rows)/2;
		int ww  = transImg.cols;
		int hh  = transImg.rows;

		tgtPt.push_back(cv::Point2f(ltx   , lty));
		tgtPt.push_back(cv::Point2f(ltx+ww, lty));
		tgtPt.push_back(cv::Point2f(ltx+ww, lty+hh));
		tgtPt.push_back(cv::Point2f(ltx   , lty+hh));
	}

	//変形行列を作成
	vector<cv::Point2f>srcPt;
	srcPt.push_back( cv::Point2f(0, 0) );
	srcPt.push_back( cv::Point2f(transImg.cols-1, 0) );
	srcPt.push_back( cv::Point2f(transImg.cols-1, transImg.rows-1) );
	srcPt.push_back( cv::Point2f(0, transImg.rows-1) );
	cv::Mat mat = cv::getPerspectiveTransform(srcPt, tgtPt);

	//出力画像と同じ幅・高さのアルファ付き画像を作成
	cv::Mat alpha0(baseImg.rows, baseImg.cols, transImg.type() );
	alpha0 = cv::Scalar::all(0);
	cv::warpPerspective(transImg, alpha0, mat,alpha0.size(), cv::INTER_CUBIC, cv::BORDER_TRANSPARENT);

	//チャンネルに分解
	cv::split(alpha0, planes_rgba);

	//RGBA画像をRGBに変換   
	planes_rgb.push_back(planes_rgba[0]);
	planes_rgb.push_back(planes_rgba[1]);
	planes_rgb.push_back(planes_rgba[2]);
	merge(planes_rgb, img_rgb);

	//RGBA画像からアルファチャンネル抽出   
	planes_aaa.push_back(planes_rgba[3]);
	planes_aaa.push_back(planes_rgba[3]);
	planes_aaa.push_back(planes_rgba[3]);
	merge(planes_aaa, img_aaa);

	//背景用アルファチャンネル   
	planes_1ma.push_back(maxVal-planes_rgba[3]);
	planes_1ma.push_back(maxVal-planes_rgba[3]);
	planes_1ma.push_back(maxVal-planes_rgba[3]);
	merge(planes_1ma, img_1ma);

	img_dst = img_rgb.mul(img_aaa, 1.0/(double)maxVal) + baseImg.mul(img_1ma, 1.0/(double)maxVal);
}

/**------------------------------------------------------------*
 * @fn			main
 * @brief		メイン関数
 *------------------------------------------------------------*/
int main(int argc, char** argv)
{
	cv::Mat transImg, baseImg, img_dst;

	//画像読み込み
	transImg   = cv::imread("images/OpenCV_Logo.png", cv::IMREAD_UNCHANGED);
	baseImg = cv::imread("images/lena.jpg");

	if( (transImg.data==NULL) || (baseImg.data==NULL) )
	{
		printf("------------------------------\n");
		printf("image not exist\n");
		printf("------------------------------\n");
		return EXIT_FAILURE;
	}
	else
	{
		printf("------------------------------\n");
		printf("Press ANY key to quit\n");
		printf("------------------------------\n");
	}

	cv::namedWindow(winName); //ウィンドウ生成

	//座標指定(背景画像の中心に表示する）
	int ltx = (baseImg.cols - transImg.cols)/2;
	int lty = (baseImg.rows - transImg.rows)/2;
	int ww  = transImg.cols;
	int hh  = transImg.rows;
	vector<cv::Point2f>tgtPt;

	tgtPt.push_back(cv::Point2f(ltx   , lty));
	tgtPt.push_back(cv::Point2f(ltx+ww, lty));
	tgtPt.push_back(cv::Point2f(ltx+ww, lty+hh));
	tgtPt.push_back(cv::Point2f(ltx   , lty+hh));

 	DrawTransPinP(img_dst, transImg, baseImg, tgtPt)  ;
	cv::imshow(winName, img_dst);
	cv::waitKey(0); //キーボード処理

	return EXIT_SUCCESS;
}


