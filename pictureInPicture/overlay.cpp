/**------------------------------------------------------------*
 * @file    overlay.cpp
 * @brief   warpAffineを用いて画像内に別の画像を描画する
 *          （ピクチャー・イン・ピクチャー機能）
 *------------------------------------------------------------*/
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
 
#include <stdlib.h>
#include <stdio.h>
 
using namespace std;
 
const char winName[]="picture in picture";
 
/**------------------------------------------------------------*
 * @fn          OpenCVのピクチャーインピクチャ
 * @brief       画像内に画像を貼り付ける（並行移動量指定）
 * @param[in ]  srcImg  背景画像
 * @param[in ]  smallImg    前景画像
 * @param[in ]  tx  前景画像の左上x座標
 * @param[in ]  ty  前景画像の左上y座標
 *------------------------------------------------------------*/
 void PinP_tr(const cv::Mat &srcImg, const cv::Mat &smallImg, const int tx, const int ty)
 {
    //背景画像の作成
    cv::Mat dstImg = srcImg.clone();
 
    //前景画像の変形行列
    cv::Mat mat = (cv::Mat_<double>(2,3)<<1.0, 0.0, tx, 0.0, 1.0, ty);
 
    //アフィン変換の実行
    cv::warpAffine(smallImg, dstImg, mat, dstImg.size(), cv::INTER_LINEAR, cv::BORDER_TRANSPARENT);
    imshow(winName, dstImg);
 }
 
/**------------------------------------------------------------*
 * @fn          OpenCVのピクチャーインピクチャ
 * @brief       画像内に画像を貼り付ける（回転角度指定）
 * @param[in ]  srcImg  背景画像
 * @param[in ]  smallImg    前景画像
 * @param[in ]  angle   回転角度[degree]
 *------------------------------------------------------------*/
 void PinP_rot(const cv::Mat &srcImg, const cv::Mat &smallImg, const double angle)
 {
    //背景画像の作成
    cv::Mat dstImg = srcImg.clone();
 
    //前景画像の変形行列
    cv::Point2d ctr(smallImg.cols/2, smallImg.rows/2);//前景画像の回転中心
    cv::Mat mat = cv::getRotationMatrix2D(ctr, angle, 1.0);//回転行列の作成
 
    cv::warpAffine(smallImg, dstImg, mat, dstImg.size(), cv::INTER_LINEAR, cv::BORDER_TRANSPARENT);
    imshow(winName, dstImg);
 }
 
/**------------------------------------------------------------*
 * @fn          OpenCVのピクチャーインピクチャ
 * @brief       画像内に画像を貼り付ける（回転角度、移動量指定）
 * @param[in ]  srcImg  背景画像
 * @param[in ]  smallImg    前景画像
 * @param[in ]  angle   回転角度[degree]
 * @param[in ]  tx  前景画像の左上x座標
 * @param[in ]  ty  前景画像の左上y座標
 *------------------------------------------------------------*/
 void PinP_rot_tr(const cv::Mat &srcImg, const cv::Mat &smallImg, const double angle, const double tx, const double ty)
 {
    //背景画像の作成
    cv::Mat dstImg = srcImg.clone();
 
    //前景画像の変形行列
    cv::Point2d ctr(smallImg.cols/2, smallImg.rows/2);//前景画像の回転中心
    cv::Mat mat = cv::getRotationMatrix2D(ctr, angle, 1.0);//回転行列の作成
    mat.at<double>(0,2) +=tx;//回転後の平行移動量
    mat.at<double>(1,2) +=ty;//回転後の平行移動量
 
    //アフィン変換の実行
    cv::warpAffine(smallImg, dstImg, mat, dstImg.size(), cv::INTER_LINEAR, cv::BORDER_TRANSPARENT);
    imshow(winName, dstImg);
 }
 
/**------------------------------------------------------------*
 * @fn          OpenCVのピクチャーインピクチャ
 * @brief       画像内に画像を貼り付ける（位置を座標で指定）
 * @param[in ]  srcImg  背景画像
 * @param[in ]  smallImg    前景画像
 * @param[in ]  p0  前景画像の左上座標
 * @param[in ]  p1  前景画像の右下座標
 *------------------------------------------------------------*/
 void PinP_point(const cv::Mat &srcImg, const cv::Mat &smallImg, const cv::Point2f p0, const cv::Point2f p1)
 {
    //背景画像の作成
    cv::Mat dstImg = srcImg.clone();
 
    //３組の対応点を作成
    vector<cv::Point2f> src, dst;
    src.push_back(cv::Point2f(0, 0));
    src.push_back(cv::Point2f(smallImg.cols, 0));
    src.push_back(cv::Point2f(smallImg.cols, smallImg.rows));
 
    dst.push_back(p0);
    dst.push_back(cv::Point2f(p1.x, p0.y));
    dst.push_back(p1);
 
    //前景画像の変形行列
    cv::Mat mat = cv::getAffineTransform(src, dst);
 
    //アフィン変換の実行
    cv::warpAffine(smallImg, dstImg, mat, dstImg.size(), cv::INTER_LINEAR, cv::BORDER_TRANSPARENT);
    imshow(winName, dstImg);
 }
 
/**------------------------------------------------------------*
 * @fn          main
 * @brief       メイン関数
 *------------------------------------------------------------*/
int main(int argc, char** argv)
{
    cv::Mat srcImg, smallImg;
 
    //画像読み込み
    srcImg   = cv::imread("images/biei.jpg");
    smallImg = cv::imread("images/lena_small.jpg");
 
    if( (srcImg.data==NULL) || (smallImg.data==NULL) )
    {
        printf("------------------------------\n");
        printf("image not exist\n");
        printf("------------------------------\n");
        return EXIT_FAILURE;
    }
    else
    {
        printf("------------------------------\n");
        printf("Press ANY key to progress\n");
        printf("------------------------------\n");
    }
 
    cv::namedWindow(winName); //ウィンドウ生成
 
    //平行移動
    PinP_tr(srcImg, smallImg, 100, 100);
    cv::waitKey(0); //キーボード処理
 
    //回転
    PinP_rot(srcImg, smallImg, 45);
    cv::waitKey(0); //キーボード処理
 
    //回転＋平行移動
    PinP_rot_tr(srcImg, smallImg, 45, 100, 100);
    cv::waitKey(0); //キーボード処理
 
    //座標指定
    cv::Point2f p0(100,100);
    cv::Point2f p1(450,380);
    PinP_point(srcImg, smallImg, p0, p1);
    cv::waitKey(0); //キーボード処理
 
    return EXIT_SUCCESS;
}
