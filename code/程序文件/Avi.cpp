#include "Avi.h"
#include "stdlib.h"
#include "math.h"

//初始化AVI
Avi::Avi (const char* file)
{
	filename = file;

	CvCapture* capture = Avi::ReadAvi ();
	frame_size.height = (int) cvGetCaptureProperty(capture,CV_CAP_PROP_FRAME_HEIGHT);
	frame_size.width = (int) cvGetCaptureProperty(capture,CV_CAP_PROP_FRAME_WIDTH);

	fps = cvGetCaptureProperty(capture,CV_CAP_PROP_FPS);	
	outCompressCodec = (int) cvGetCaptureProperty(capture,CV_CAP_PROP_FOURCC);

	writer_counter = cvCreateVideoWriter("counter.avi",outCompressCodec,fps,frame_size);
	writer_GaussBg = cvCreateVideoWriter("GaussBg.avi",outCompressCodec,fps,frame_size);
	writer_OpticalFlow = cvCreateVideoWriter("OpticalFlow.avi",outCompressCodec,fps,frame_size);
}

//读取AVI文件
CvCapture* Avi::ReadAvi ()
{
	CvCapture* capture = cvCaptureFromAVI(filename);
	if(capture)
		return capture;
	else
	{
		printf("can not read the avi file\n");
		exit(0);
	}
}

//显示原始图像
void Avi::ShowOriginImage (int nFrmNum,IplImage* frame)
{ 
	if(nFrmNum == 1)
	{
		cvNamedWindow("origin",1);
		cvMoveWindow("origin",10,0);
	}
	cvShowImage("origin",frame);
 	cvWaitKey(10);
}

IplImage* Frame_current = NULL;
IplImage* Frame_previous = NULL;
IplImage* Frame_fore = NULL;

CvMat* pFrMat = NULL;
CvMat* pFrMat_cur = NULL;
CvMat* pFrMat_pre = NULL;

//帧差法
IplImage* Avi::ShowCounterImage(int nFrmNum,IplImage* pFrImg_cur,IplImage* pFrImg_pre)
{
	if(nFrmNum == 2)
	{
		cvNamedWindow("counter",1);
		cvMoveWindow("counter",400,0);
		
		Frame_current = cvCreateImage(frame_size,IPL_DEPTH_8U,1);
		Frame_previous = cvCreateImage(frame_size,IPL_DEPTH_8U,1);
		Frame_fore = cvCreateImage(frame_size,IPL_DEPTH_8U,1);
		
		pFrMat = cvCreateMat(frame_size.height,frame_size.width,CV_32FC1);
		pFrMat_cur = cvCreateMat(frame_size.height,frame_size.width,CV_32FC1); 
		pFrMat_pre = cvCreateMat(frame_size.height,frame_size.width,CV_32FC1);
	}

	cvCvtColor(pFrImg_cur,Frame_current,CV_BGR2GRAY);
	cvConvert(Frame_current,pFrMat_cur);

	cvCvtColor(pFrImg_pre,Frame_previous,CV_BGR2GRAY);
	cvConvert(Frame_previous,pFrMat_pre);

	cvSmooth(pFrMat_cur,pFrMat_cur,CV_GAUSSIAN,3,0,0);
	
	cvAbsDiff(pFrMat_cur,pFrMat_pre,pFrMat);
	cvThreshold(pFrMat,Frame_fore,50,255.0,CV_THRESH_BINARY);

	cvErode(Frame_fore,Frame_fore,0,1);
	cvDilate(Frame_fore,Frame_fore,0,1);

	Frame_fore -> origin = 1;
	cvShowImage("counter",Frame_fore);
	cvWaitKey(10);
	return Frame_fore;
}

//保存图像到AVI视频文件中
void Avi::SaveImage(CvVideoWriter* writer,IplImage* frame)
{
	//CvVideoWriter* writer = cvCreateVideoWriter("counter_out.avi",outCompressCodec,fps,size);
	cvWriteFrame(writer,frame);
}


IplImage* pFrImg = NULL;
IplImage* pBkImg = NULL;

IplImage* dstB = NULL;
IplImage* dstG = NULL;
IplImage* dstR = NULL;

CvMat* pMatB = NULL;
CvMat* pMatG = NULL;
CvMat* pMatR = NULL;

//混合背景高斯建模，背景差分法
IplImage* Avi::ShowGaussBgImage (int nFrmNum,IplImage* pFrImg_cur)
{
	if(nFrmNum == 1)
	{
		cvNamedWindow("GaussBg",1);
		cvMoveWindow("GaussBg",10,330);

		pBkImg = cvCreateImage(frame_size,IPL_DEPTH_8U,3);
		pFrImg = cvCreateImage(frame_size,IPL_DEPTH_8U,3);
		dstB = cvCreateImage(frame_size,IPL_DEPTH_8U,1);
		dstG = cvCreateImage(frame_size,IPL_DEPTH_8U,1);
		dstR = cvCreateImage(frame_size,IPL_DEPTH_8U,1);

		pMatB = cvCreateMat(frame_size.height,frame_size.width,CV_8UC1);
		pMatG = cvCreateMat(frame_size.height,frame_size.width,CV_8UC1);
		pMatR = cvCreateMat(frame_size.height,frame_size.width,CV_8UC1);

		bg_model = (CvGaussBGModel*)cvCreateGaussianBGModel(pFrImg_cur,0);
	}
	else
	{
		cvUpdateBGStatModel(pFrImg_cur,(CvBGStatModel *)bg_model);
		cvCopy(bg_model->background,pBkImg,0);

		for(int i=0; i<pFrImg_cur->height; i++)
			for(int j=0; j<pFrImg_cur->width; j++)
			{
				s_fr = cvGet2D(pFrImg_cur,i,j);
				s_bg = cvGet2D(pBkImg,i,j);

				if((fabs(s_fr.val[0] - s_bg.val[0]) >= threshold) &&
					(fabs(s_fr.val[1] - s_bg.val[1]) >= threshold) &&
					(fabs(s_fr.val[2] - s_bg.val[2]) >= threshold))
				{
					cvSet2D(pFrImg,i,j,s_fr);
				}
				else
				{
					s_fr.val[0] = 0;
					s_fr.val[1] = 0;
					s_fr.val[2] = 0;
					cvSet2D(pFrImg,i,j,s_fr);
				}
			}
		cvErode(pBkImg,pBkImg,0,1);
		cvDilate(pBkImg,pBkImg,0,1);

		//通道分离的BGR，进行高斯滤波
		/*cvSplit(pFrImg,dstB,dstG,dstR,0);
		cvConvert(dstB,pMatB);
		cvConvert(dstG,pMatG);
		cvConvert(dstR,pMatR);

		cvSmooth(pMatB,pMatB,CV_GAUSSIAN,3,0,0);
		cvSmooth(pMatG,pMatG,CV_GAUSSIAN,3,0,0);
		cvSmooth(pMatR,pMatR,CV_GAUSSIAN,3,0,0);

		cvGetImage(pMatB,dstB);
		cvGetImage(pMatG,dstG);
		cvGetImage(pMatR,dstR);

		cvMerge(dstB,dstG,dstR,0,pFrImg);*/

		//形态学滤波
		/*cvErode(pFrImg,pFrImg,0,1);	
		cvDilate(pFrImg,pFrImg,0,1);*/

		pBkImg->origin = 1;
		pFrImg->origin = 1;

		cvShowImage("GaussBg",pFrImg);
		cvWaitKey(10);
	}
	return pFrImg;
}

inline static double square(int a)
{
	return a*a;
}

//光流法
IplImage* Avi::ShowOpticalFlowImage (int nFrmNum,IplImage* pFrImg_cur,IplImage* pFrImg_pre)
{
	if(nFrmNum == 2)
	{
		cvNamedWindow("OpticalFlow",CV_WINDOW_AUTOSIZE);
		cvMoveWindow("OpticalFlow",400,330);
	}

	static IplImage* frame1 = NULL,*frame1_1C = NULL,*frame2_1C = 
	NULL,*eig_image = NULL,*temp_image = NULL,*pyramid1 = NULL,*pyramid2 = NULL;

	frame1_1C = cvCreateImage(frame_size,IPL_DEPTH_8U,1);
	cvConvertImage(pFrImg_pre,frame1_1C,CV_CVTIMG_FLIP);

	frame1 = cvCreateImage(frame_size,IPL_DEPTH_8U,3);
	cvConvertImage(pFrImg_pre,frame1,CV_CVTIMG_FLIP);

	frame2_1C = cvCreateImage(frame_size,IPL_DEPTH_8U,1);
	cvConvertImage(pFrImg_cur,frame2_1C,CV_CVTIMG_FLIP);

	CvPoint2D32f frame1_features[400];

	int number_of_features;
	number_of_features = 400;

	cvGoodFeaturesToTrack(frame1_1C,eig_image,temp_image,frame1_features,&number_of_features,.01,.01,NULL);
	CvPoint2D32f frame2_features[400];

	char optical_flow_found_feature[400];
	float optical_flow_feature_error[400];

	CvSize optical_flow_window = cvSize(3,3);

	CvTermCriteria optical_flow_termination_criteria = cvTermCriteria(CV_TERMCRIT_ITER | CV_TERMCRIT_EPS,20,.3);

	pyramid1 = cvCreateImage(frame_size,IPL_DEPTH_8U,1);
	pyramid2 = cvCreateImage(frame_size,IPL_DEPTH_8U,1);

	cvCalcOpticalFlowPyrLK(frame1_1C,frame2_1C,pyramid1,pyramid2,frame1_features,
		frame2_features,number_of_features,optical_flow_window,5,
		optical_flow_found_feature,optical_flow_feature_error,
		optical_flow_termination_criteria,0);

	for(int i=0; i<number_of_features; i++)
	{
		if(optical_flow_found_feature[i] == 0) continue;
		int line_thickness; line_thickness = 1;
 		CvScalar line_color; line_color = CV_RGB(255,0,0);

		CvPoint p,q;
		p.x = (int) frame1_features[i].x;
		p.y = (int) frame1_features[i].y;
		q.x = (int) frame2_features[i].x;
		q.y = (int) frame2_features[i].y;

		double angle; angle = atan2( (double) p.y - q.y, (double) p.x - q.x);
		double hypotenuse; hypotenuse = sqrt(square(p.y - q.y) + square(p.x - q.x));

		q.x = (int) (p.x - 3 * hypotenuse * cos(angle));
		q.y = (int) (p.y - 3 * hypotenuse * sin(angle));

		cvLine(frame1,p,q,line_color,line_thickness,CV_AA,0);

		p.x = (int) (q.x + 9 * cos(angle + pi/4));
		p.y = (int) (q.y + 9 * sin(angle + pi/4));
		cvLine(frame1,p,q,line_color,line_thickness,CV_AA,0);

		p.x = (int) (q.x + 9 * cos(angle - pi/4));
		p.y = (int) (q.y + 9 * sin(angle - pi/4));
		cvLine(frame1,p,q,line_color,line_thickness,CV_AA,0);
	}
	cvShowImage("OpticalFlow",frame1);
	cvWaitKey(10);
	return frame1;
}