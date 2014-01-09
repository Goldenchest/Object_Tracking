#include <iostream>
#include <opencv2\highgui\highgui.hpp>
#include <opencv2\video\tracking.hpp>
#include <opencv2\imgproc\imgproc.hpp>
#include "colorhistogram.h"
#include "objectFinder.h"
using namespace std;

// defines the max number of iterations and error tolerance
cv::TermCriteria criteria(cv::TermCriteria::MAX_ITER,10,0.03);
ColorHistogram hc;   // contains functions that create histograms
ObjectFinder finder; // contains functions that use histograms to find objects
cv::Mat image;       // stores each unprocessed image as a matrix of numbers
cv::Mat hsv;         // stores the hsv version of the image
cv::Mat imageROI;    // defines the region of interest in the image
cv::Mat result;      // stores the processed image
cv::MatND colorhist; // stores the histogram of the object being tracked
int minSat = 15;     // if pixel saturation is under this value, the pixel
                     // shouldn't be processed

cv::Point point1, point2; // the two points used to draw the rectangle
cv::Rect rect; // stores the drawn rectangle
bool drag = false; // test if user is dragging the mouse
bool rectangleDrawn = false; // test if a rectangle has been drawn

void mouseEvent(int event, int x, int y, int flags, void* param) {
    if (event == CV_EVENT_LBUTTONDOWN && !drag && !rectangleDrawn) {
        point1 = cv::Point(x,y);
        drag = true;
    }
    if (event == CV_EVENT_LBUTTONDOWN && drag && !rectangleDrawn) {
        point2 = cv::Point(x,y);
        rect = cv::Rect(point1.x,point1.y,x-point1.x,y-point1.y);
        cv::rectangle(image,rect,cv::Scalar(255,255,0),2);
        cv::imshow("Processed Image",image);
    }
    if (event == CV_EVENT_MOUSEMOVE && drag && !rectangleDrawn) {
        point2 = cv::Point(x,y);
        rect = cv::Rect(point1.x,point1.y,x-point1.x,y-point1.y);
        cv::rectangle(image,rect,cv::Scalar(255,255,0),2);
        cv::imshow("Processed Image",image);
    }
    if (event == CV_EVENT_LBUTTONUP && !rectangleDrawn) {
        point2 = cv::Point(x,y);
        rect = cv::Rect(point1.x,point1.y,x-point1.x,y-point1.y);
        cv::rectangle(image,rect,cv::Scalar(255,255,0),2);
        drag = false;
        rectangleDrawn = true;
    }
}

// function to convert integer to string (used in printCoordinate)
string convertInt(int number) {
    std::stringstream ss;
    ss << number;
    return ss.str();
}
// prints the coordinate of the drawn rectangle
void printCoordinate(cv::Mat &image) {
    int x_coordinate = rect.x+rect.width/2;
    int y_coordinate = rect.y+rect.height/2;
    std::string coordinate = "(" + convertInt(x_coordinate) + "," + convertInt(y_coordinate) + ")";
    int fontface = cv::FONT_HERSHEY_SIMPLEX;
    double scale = 0.6;
    int thickness = 2;
    printf("(%i,%i)\n",x_coordinate,y_coordinate);
    cv::Point textOrg(x_coordinate-rect.width/5,y_coordinate);
    cv::putText(image, coordinate, textOrg, fontface, scale, cv::Scalar(255,255,0), thickness);
}
// sharpens the image to make object detection easier
void sharpen(const cv::Mat &image, cv::Mat &result, int strength=1.0) {
    for (int i=0; i<strength; i++){
        // Construct kernel (all entries initalized to 0)
        cv::Mat kernel(3,3,CV_32F,cv::Scalar(0));
        // assign kernel values
        kernel.at<float>(1,1) = 5.0;
        kernel.at<float>(0,1) = -1.0;
        kernel.at<float>(2,1) = -1.0;
        kernel.at<float>(1,0) = -1.0;
        kernel.at<float>(1,2) = -1.0;
        // filter the image
        cv::filter2D(image,result,image.depth(),kernel);
    }
}
// increases the image's contrast to make object detection easier
void increaseContrast(cv::Mat &image, float alpha) {
    cv::Mat new_image = cv::Mat::zeros(image.size(),image.type());
    for (int y=0; y<image.rows; y++) {
        for (int x=0; x<image.cols; x++) {
            for (int c=0; c<3; c++) {
                new_image.at<cv::Vec3b>(y,x)[c] =
                        cv::saturate_cast<uchar>(alpha*(image.at<cv::Vec3b>(y,x)[c]));
            }
        }
    }
    image = new_image;
}
// defines the area of interest and histogram of first frame
void initialize(cv::Mat &image) {
    imageROI = image(rect);
    colorhist = hc.getHueHistogram(imageROI);
    finder.setHistogram(colorhist);
    cv::rectangle(image,rect,cv::Scalar(255,255,0),2);
    printCoordinate(image);
}
// applies mean shift algorithm to find the object and draw a new rectangle
void drawNewRectangle(cv::Mat &image, cv::Rect& rect) {
    cv::cvtColor(image,hsv,CV_BGR2HSV); // convert image to hsv
    result = finder.find(hsv); // find the pixels that best match the histogram
    cv::meanShift(result,rect,criteria); // use mean-shift to move the rectangle
    cv::rectangle(image,rect,cv::Scalar(200,255,0),3); // draw the new rectangle
    printCoordinate(image);
}

int main() {
    cv::VideoCapture webcam(0); // default at 0
    static bool firstPass = true;
    if(webcam.isOpened() == false) { // if webcam not detected...
        // print out an error message
        std::cout << "Webcam not accessed successfully.\n\n" << std::endl;
        return 1; // termininate the program
    }
    int keyCheck = 0; // define a variable to check keypresses
    while(keyCheck != 13) { // while "Enter" key not pressed...
        if (webcam.read(image) == NULL) // if webcam is disconnected...
            break; // terminate the program
        if (rectangleDrawn) { // if a rectangle has been drawn...
            increaseContrast(image,1.2); // increase the image's contrast
			sharpen(image,image); // sharpen the image
            if (firstPass) { // if image has not been initialized...
                initialize(image); // initialize the image
                firstPass = false; // image has been initialized
            }
            else drawNewRectangle(image,rect); // update the rectangle
        }
        cv::namedWindow("Processed Image"); // create a window in which to display the video
        cv::imshow("Processed Image",image); // display image in the "result" window
        cvSetMouseCallback("Processed Image",mouseEvent,0);
        keyCheck = cv::waitKey(10); // check for "Enter" key every 10 milliseconds
    }
}
