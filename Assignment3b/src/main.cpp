#include <stdio.h>
#include "cxcore.h"
#include "cv.h"
#include "highgui.h"

int main(int argc, char **argv) {

	// Check number of arguments.
	if ( argc != 2 )
		exit(1);

	IplImage *SrcImage, *WorkImage,*ComImage;
	
	// Load a gray scale picture.
	SrcImage = cvLoadImage(argv[1], 0);
	if (SrcImage == NULL)
		exit(1);

	// Create windows for debug.
	cvNamedWindow("SrcImage", CV_WINDOW_AUTOSIZE);
	cvNamedWindow("WorkImage", CV_WINDOW_AUTOSIZE);

	// Show the source image.
	cvShowImage("SrcImage", SrcImage);

	// Duplicate the source iamge.
	WorkImage = cvCloneImage(SrcImage);	
	
	/* If you're familiar with OpenCV, cvFindContours() will be a better way.*/
	cvSmooth(WorkImage, WorkImage);
	cvThreshold(WorkImage, WorkImage, 128, 255, CV_THRESH_BINARY);

	// Opening
	cvErode(WorkImage, WorkImage);
	cvDilate(WorkImage, WorkImage);
	
	// Duplicate the working iamge.
	ComImage=cvCloneImage(WorkImage);

	// Show the working image after preprocessing.
	cvShowImage("WorkImage", WorkImage);

	cvWaitKey(-1);
	return 0;
}