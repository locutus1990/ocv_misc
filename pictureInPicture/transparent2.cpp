/*------------------------------------------------------------*
 * パースペクティブ画像を作成する
 *------------------------------------------------------------*/
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/calib3d/calib3d.hpp>

#include <stdlib.h>
#include <stdio.h>

using namespace std;

const char winName[]="Pers image";
vector<cv::Point2f> m_persRect;
int m_rectIdx=-1, m_mouseUpFlag=-1;
int m_imgW, m_imgH;
bool m_enterFlag=false;

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
	//cv::warpPerspective(transImg, alpha0, mat,alpha0.size(), cv::INTER_NEAREST, cv::BORDER_TRANSPARENT);

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

/*------------------------------------------------------------
 * @function CalcDistance
 *------------------------------------------------------------*/
double CalcDistance(cv::Point srcPt, cv::Point dstPt)
{
	double dd = sqrt( pow(dstPt.x - srcPt.x , 2) + pow(dstPt.y - srcPt.y, 2) );
	return dd;
}

/*------------------------------------------------------------*
 * 一つの多角形を描画する
 *------------------------------------------------------------*/
void DrawPersRect(cv::Mat &img, const vector<cv::Point2f> &rect, const cv::Scalar col, const int thickness)
{
	int ww = img.cols;
	int hh = img.rows;
	cv::Point2f anchorSize=cv::Point2f(10,10);

	for(int j=0;j<rect.size();j++)
	{
		cv::Point2f srcPt = rect[j];
		cv::Point2f dstPt = rect[(j+1)%rect.size()];

		cv::line(img, srcPt, dstPt, col, thickness, cv::LINE_AA, 0);
		cv::rectangle(img, srcPt-anchorSize, srcPt+anchorSize, col, cv::FILLED, cv::LINE_AA);
	}
}

/*------------------------------------------------------------
 * @function InitRect
 * @brief パース設定四角形の初期化
 *------------------------------------------------------------*/
void InitRect(cv::Mat transImg, cv::Mat baseImg)
{
	int ltx = (baseImg.cols - transImg.cols)/2;
	int lty = (baseImg.rows - transImg.rows)/2;
	int ww  = transImg.cols;
	int hh  = transImg.rows;

	//変形前
	m_persRect.clear();
	m_persRect.push_back(cv::Point2f(ltx   , lty));
	m_persRect.push_back(cv::Point2f(ltx+ww, lty));
	m_persRect.push_back(cv::Point2f(ltx+ww, lty+hh));
	m_persRect.push_back(cv::Point2f(ltx   , lty+hh));
}

/*------------------------------------------------------------
 * @function MouseClick
 *------------------------------------------------------------*/
void on_mouse( int event, int x, int y, int flags, void *param)
{
	cv::Point2i curPt;
	//アンカーポイントでのクリック判定
	if(event==cv::EVENT_LBUTTONDOWN)
	{
		m_rectIdx=-1;
		curPt = cv::Point(x, y);

		for(int i=0;i<4;i++)
		{
			double dd = CalcDistance(curPt, m_persRect[i]);
			if(dd<30)
			{
				m_rectIdx=i;
			}
			
		}
	}

	//アンカーポイントのドラッグ時
	if(m_rectIdx>-1)
	{
		if(x<2)x=2;else if(x>(m_imgW-2))x=m_imgW-2;
		if(y<2)y=2;else if(y>(m_imgH-2))y=m_imgH-2;
		curPt = cv::Point2f(x, y);
		m_persRect[m_rectIdx] = curPt;
	}

	//アンカーポイントの移動終了時
	if(event==cv::EVENT_LBUTTONUP)
	{
		if(m_rectIdx>-1)m_mouseUpFlag=1;
	}

	//ポリゴンの内外判定
	double inout = pointPolygonTest(m_persRect, cv::Point2i(x, y), 5);
	if(inout>-5)m_enterFlag=true;else m_enterFlag=false;
}

/*------------------------------------------------------------*
 * Main routine
 *------------------------------------------------------------*/
int main(int argc, char** argv)
{
	bool drawFlag=false;
	cv::Mat transImg, baseImg, tmpImg;
	bool enterState=false;
	cv::Scalar col = CV_RGB(255, 255, 255);

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
		printf("Press ESC key to quit\n");
		printf("------------------------------\n");
	}

	m_imgW = baseImg.cols;
	m_imgH = baseImg.rows;

	//パース枠の初期化
	InitRect(transImg, baseImg);
	
	//ウィンドウ生成
	cv::namedWindow(winName);

	//マウスコールバック設定
	cv::setMouseCallback(winName, on_mouse, 0);
	
	//初期画面の描画
	DrawTransPinP(tmpImg, transImg, baseImg, m_persRect);
	DrawPersRect(tmpImg, m_persRect, col, 1);
	imshow(winName, tmpImg);

	while(1)
	{
		//アンカーポイントのドラッグ時
		if(m_rectIdx>-1){drawFlag=true;}

		//アンカーポイントの移動終了時
		if(m_mouseUpFlag>0)
		{
			m_mouseUpFlag=-1;
			m_rectIdx=-1;
			drawFlag=true;
		}
		
		//キーボード処理
		int a = cv::waitKey(1);
		if(a==27)break;

		if(enterState != m_enterFlag)
		{
			enterState = m_enterFlag;
			if(enterState)col = CV_RGB(255, 0, 0);
			else col = CV_RGB(255, 255, 255);
			drawFlag=true;
		}

		if(drawFlag)
		{

			DrawTransPinP(tmpImg, transImg, baseImg, m_persRect);
			if(m_enterFlag)DrawPersRect(tmpImg, m_persRect, col, 1);
			imshow(winName, tmpImg);

			drawFlag=false;
		}
	}

	return EXIT_SUCCESS;
}

