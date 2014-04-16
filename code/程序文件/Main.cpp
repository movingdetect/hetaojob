#include "Avi.h"

int main(int argc,char** argv)
{

	Avi x("test.avi");
	CvCapture* capture = x.ReadAvi ();

	IplImage* pFrImg_cur = NULL;
	IplImage* pFrImg_pre = NULL;
	int nFrmNum = 0;

	while(pFrImg_cur = cvQueryFrame(capture))
	{
		nFrmNum ++;

		//��ʾԭʼͼ��
 		x.ShowOriginImage(nFrmNum,pFrImg_cur);	
		
		
		if(nFrmNum == 1)
		{
			pFrImg_pre = cvCreateImage(cvSize(pFrImg_cur->width,pFrImg_cur->height),IPL_DEPTH_8U,3);
			cvCopy(pFrImg_cur,pFrImg_pre,NULL);
		}
		else
		{	
			//֡�����˶�Ŀ�꣬���洦����ͼ��																					
 			x.SaveImage (x.writer_counter, x.ShowCounterImage (nFrmNum,pFrImg_cur,pFrImg_pre));	

			//ϡ��������������Lucas-Kanande�㷨������˶�Ŀ�꣬�����洦����ͼ��
			x.SaveImage (x.writer_OpticalFlow ,x.ShowOpticalFlowImage (nFrmNum,pFrImg_cur,pFrImg_pre));
			cvCopy(pFrImg_cur,pFrImg_pre,NULL);
		}
		
		//��ϸ�˹������ģ��������֣����洦����ͼ��
		x.SaveImage(x.writer_GaussBg, x.ShowGaussBgImage (nFrmNum,pFrImg_cur));
		
	}
	cvReleaseVideoWriter(&x.writer_OpticalFlow ); 
	cvReleaseVideoWriter(&x.writer_counter );
	cvReleaseVideoWriter(&x.writer_GaussBg );
	return 0;
}