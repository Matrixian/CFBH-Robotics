#include <stdio.h>
#include <iostream>
#include <string>
#include <stack>
#include <list>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdexcept>

#include <cxcore.h>
#include <cv.h>
#include <highgui.h>

#define MIN_REGION_SIZE 500
#define MAX_REGION_COUNT 254
#define DEBUG 1

using namespace std;

/**
 * Represents a pixel position in a picture.
 */
class Pixel {
private:
	unsigned int x, y;
public:
	/**
	 * Sole constructor.
	 */
	Pixel(unsigned int initX, unsigned int initY)
		: x(initX),
		  y(initY) {
		;
	}

	/**
	 * @return the x value
	 */
	inline unsigned int getX() {
		return x;
	}

	/**
	 * @return the y value
	 */
	inline unsigned int getY() {
		return y;
	}
};

/**
 * Represents a region in the picture.
 */
class Region {

private:
	const unsigned char val;
	unsigned int size, xSum, ySum; // moments
	stack<Pixel> pixels;
	float principleAngle;

public:
	/**
	 * Sole constructor.
	 *
	 * @param initVal grayscale value of this region
	 */
	Region(const unsigned char initVal)
		: val(initVal),
		  size(0),
		  xSum(0),
		  ySum(0),
		  principleAngle(0){
		;
	}

	/**
	 * Get principle angle. If there are new added pixels, recalculate. If not,
	 * use the former value.
	 *
	 * @return principle angle regarding new added pixels, 0 if its uninitialized
	 */
	float getPrincipleAngle() {
		if (pixels.empty()) {
			return principleAngle;
		}
		const int xCenter = xSum / size, yCenter = ySum / size;
		int aTan1 = 0, aTan2 = 0; // both elements of atan2
		while (!pixels.empty()) {
			Pixel pixel = pixels.top();
			int xDiff = pixel.getX() - xCenter;
			int yDiff = pixel.getY() - yCenter;
			aTan1 += xDiff * yDiff;
			aTan2 += yDiff * yDiff - xDiff * xDiff;
			pixels.pop();
		}
		principleAngle = atan2((float) aTan1 * 2, (float) aTan2) / 2.0;
		return principleAngle;
	}

	/**
	 * Get the centroid of the region considering all pixels added so far.
	 *
	 * @return centroid of the region
	 */
	inline Pixel getCentroid() {
		return Pixel(xSum / size, ySum / size);
	}

	/**
	 * Add a point to this region and update the moments.
	 *
	 * @param x x-value
	 * @param y y-value
	 */
	void addPixel(unsigned int x, unsigned int y) {
		pixels.push(Pixel(x,y));
		size++;
		xSum += x;
		ySum += y;
	}

	/**
	 * @return the value (grayscale) of this region.
	 */
	inline unsigned char getVal() {
		return val;
	}

	/**
	 * @return the current size (moment m_00).
	 */
	inline unsigned int getSize() {
		return size;
	}

	/**
	 * @return the current sum of x values (moment m_10).
	 */
	inline unsigned int getXSum() {
		return xSum;
	}

	/**
	 * @return the current sum of y values (moment m_01).
	 */
	inline unsigned int getYSum() {
		return ySum;
	}
};

/**
 * Get a list of regions fetched from the image. The moments are collected
 * simultaneously.
 *
 * @param img binary input image
 * @return list of Region instances
 * @throws std::invalid_argument if there are more than 254 regions
 */
list<Region*>* getRegions(IplImage *img) {
	/* Basic strategy: iterate all pixel positions. If its white, assign a new
	 * color and thread it as new region (use FloodFill). If its black, continue.
	 * If its in between, assign this pixel to a existing region.
	 */
	Region* regions[MAX_REGION_COUNT + 1];
	unsigned char curColor = MAX_REGION_COUNT;
	const int nPixels = img->height * img->width;
	const int width = img->width;
	const CvScalar nullScalar = cvScalarAll(0);
	for (int i = 0; i < nPixels; i++) {
		unsigned char val = img->imageData[i];
		if (val == 0) { // Skip black pixels
			continue;
		}
		int x = i % width;
		int y = i / width;
		if (val == 255) { // Color new region
			if (curColor == 0) {
				throw invalid_argument("There are more than 254 regions.");
			}
			const CvPoint seed = cvPoint(x, y);
			CvConnectedComp *comp = new CvConnectedComp();
			cvFloodFill(img, seed, cvScalarAll(curColor),
						nullScalar, nullScalar, comp, 4, NULL);
			if (comp->area < MIN_REGION_SIZE) { // Discard
				if (DEBUG) {
					cout << "Discard region at " << x << "," << y << endl;
				}
				cvFloodFill(img, cvPoint(x, y), nullScalar, // Color black
							nullScalar, nullScalar, NULL, 4, NULL);
				delete comp;
				continue;
			}
			val = curColor--;
			Region *region = new Region(val);
			if (DEBUG) {
				cout << "New region " << ((int) val) << " seeded at ("
					 << x << "," << y << ")" << endl;
			}
			regions[MAX_REGION_COUNT - val] = region;
		}
		regions[MAX_REGION_COUNT - val]->addPixel(x,y);
	}
	// Drop small regions, build linked list of regions
	list<Region*> *result = new list<Region*>();
	for (int i = 0; i < MAX_REGION_COUNT - curColor; i++) {
		regions[i]->getPrincipleAngle(); // Space efficiency
		result->push_back(regions[i]);
	}
	return result;
}

/**
 * Draws centroids and principle angles into the image.
 *
 * @param img image to augment
 */
void augmentImage(IplImage *img, list<Region*> *regions) {
	list<Region*>::iterator i;
	float index = 0.0;
	const unsigned int regionSize = regions->size();
	for (i = regions->begin(); i != regions->end(); ++i) {
		const CvScalar s1 = cvScalar(
				150 * index / regionSize,
				255 * index / regionSize,
				255 * (1.0 - index / regionSize), 0.0);
		const float principleAngle = (*i)->getPrincipleAngle();
		Pixel centroid = (*i)->getCentroid();
		index++;

		// Text output
		cout << "REGION " << ((int) (*i)->getVal()) << ": " << endl;
		cout << "Centroid: (" << centroid.getX() << ","
							  << centroid.getY() << ")" << endl;
		cout << "Principle Angle: " << principleAngle << " ("
				<< (360 * principleAngle / M_2_PI) << " degree)" << endl;

		// Calculate anchor points for drawing the principle angle
		float calAngle = principleAngle;
	    if(calAngle > M_PI){
	      calAngle -= M_PI;
	    }
	    // Increase the angle by 90 degrees. This is the actual slope of the straight line.
	    calAngle += M_PI/2.0;
	    CvPoint p1 = cvPoint(0, 0);
	    CvPoint p2 = cvPoint(0, 0);
	    float slope = 0.0;
	    if(abs(principleAngle) >0.009){
	      slope = tan(calAngle);
	      p1.x = 0;
	      p1.y = centroid.getY() + slope * centroid.getX();
	      p2.x = img->width;
	      p2.y = centroid.getY() - slope * (img->width-centroid.getX());

	    } else {
	      p1.x = centroid.getX();
	      p1.y = 0;
	      p2.x = centroid.getX();
	      p2.y = img->height;
	    }
	    if (DEBUG) {
			cout << "Point 1 " << p1.x <<","<<p1.y<< endl;
			cout << "Point 2 " << p2.x <<","<<p2.y<< endl;
			cout << "Slope " << slope << endl;
	    }
	    cvLine(img, p1, p2, s1, 1, 8, 0);
	    cvCircle(img, cvPoint(centroid.getX(), centroid.getY()),
	    		 4, s1, -1, 8, 0);
	}
}

/**
 * Main runner method.
 */
int main(int argc, char **argv) {
	// Check number of arguments.
	if ( argc != 2 ) {
		exit(1);
	}

	IplImage *srcImage, *workImage, *resultImage;

	// Load a gray scale picture.
	srcImage = cvLoadImage(argv[1], 0);

	if (srcImage == NULL)
		exit(1);

	// Create windows for debug.
	cvNamedWindow("Regions", CV_WINDOW_AUTOSIZE);
	cvNamedWindow("ResultImage", CV_WINDOW_AUTOSIZE);

	// Duplicate the source image.
	workImage = cvCloneImage(srcImage);

	cvSmooth(workImage, workImage);
	cvThreshold(workImage, workImage, 128, 255, CV_THRESH_BINARY);

	// Opening
	cvErode(workImage, workImage);
	cvDilate(workImage, workImage);

	// Find regions, augment picture
	list<Region*> *regions = getRegions(workImage);
	resultImage=cvCreateImage(cvGetSize(srcImage), 8, 3);
	cvCvtColor(srcImage,resultImage,CV_GRAY2BGR);
	augmentImage(resultImage, regions);
	delete regions;

	// Show images
	cvShowImage("Regions", workImage);
	cvShowImage("ResultImage", resultImage);
	cvWaitKey(-1);
	return 0;
}
