#include <opencv2/opencv.hpp>
#include "opencv2/core/core.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/xfeatures2d.hpp"
#include "videoHomography.h"
//#include "opencv2/nonfree/nonfree.hpp"


#include <iostream>
#include <stdio.h>

using namespace cv;
using namespace std;


void readme();
void drawRectOnFrame( Mat img_scene, std::vector<Mat> img_objects);
bool isNumeric(const std::string& str);

bool isNumeric(const std::string& str) {
    char *end;
    long val = std::strtol(str.c_str(), &end, 10);
    if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN)) || (errno != 0 && val == 0)) {
        //  if the converted value would fall out of the range of the result type.
        return false;
    }
    if (end == str) {
       // No digits were found.
       return false;
    }
    // check if the string was fully processed.
    return *end == '\0';
}

int main(int argc, char** argv)
{
	if( argc < 3 )
	{	readme(); return -1; }

	//VideoCapture cap(0); // open the default camera

	VideoCapture cap;
	if (isNumeric(argv[1]))
		cap.open(atoi(argv[1]));
	else
		cap.open(argv[1]);


	if(!cap.isOpened())  // check if we succeeded
		return -1;

	std::vector<Mat> objects;

	for (int i = 2; i < argc; i++){
		Mat object;
		try{
			object = imread(argv[i], CV_LOAD_IMAGE_COLOR);
		}catch(Exception& e){
			printf("Fail loading image '%s'", argv[i]);
		}
		objects.push_back(object);
	}

	VideoHomography vh = VideoHomography(cap, objects);
	vh.run();

	/*
	VideoWriter outputVideo;                                        // Open the output

	outputVideo.open("scene2_2.mp4", static_cast<int>(cap.get(CV_CAP_PROP_FOURCC)), cap.get(CV_CAP_PROP_FPS), Size((int) cap.get(CV_CAP_PROP_FRAME_WIDTH),    // Acquire input size
            (int) cap.get(CV_CAP_PROP_FRAME_HEIGHT)), true);

	if (!outputVideo.isOpened())
	{
		cout  << "Could not open the output video for write." << endl;
		return -1;
	}
	*/


    Mat edges;
    cv::namedWindow("edges",1);
    cv::CascadeClassifier();

    for(;;)
    {
        Mat frame;
        Mat processed_frame;

        cap >> frame; // get a new frame from camera

        drawRectOnFrame(frame, objects);

        //cv::cvtColor(frame, edges, CV_BGR2GRAY);
        //cv::GaussianBlur(edges, edges, Size(7,7), 1.5, 1.5);
        //cv::Canny(edges, edges, 0, 30, 3);


        imshow("edges", frame);
        //outputVideo << frame;
        if(waitKey(30) >= 0) break;
    }
    // the camera will be deinitialized automatically in VideoCapture destructor
    return 0;

}


void drawRectOnFrame( Mat img_scene, std::vector<Mat> img_objects){
  Mat img_object_gray;
  Mat img_scene_gray;

  cv::cvtColor(img_scene, img_scene_gray, CV_BGR2GRAY);

  for(int i = 0; i < img_objects.size(); i++){

	  cv::cvtColor(img_objects[i], img_object_gray, CV_BGR2GRAY);

	  //-- Step 1: Detect the keypoints using SURF Detector
	  int minHessian = 400;

	  //SurfFeatureDetector detector( minHessian );

	  Ptr<cv::xfeatures2d::SURF> detector=cv::xfeatures2d::SURF::create(minHessian);

	  std::vector<KeyPoint> keypoints_object, keypoints_scene;

	  detector->detect( img_object_gray, keypoints_object );
	  detector->detect( img_scene_gray, keypoints_scene );

	  //-- Step 2: Calculate descriptors (feature vectors)
	  //SurfDescriptorExtractor extractor;
	  Ptr<cv::xfeatures2d::SURF> extractor = cv::xfeatures2d::SURF::create();

	  Mat descriptors_object, descriptors_scene;

	  extractor->compute( img_object_gray, keypoints_object, descriptors_object );
	  extractor->compute( img_scene_gray, keypoints_scene, descriptors_scene );

	  //-- Step 3: Matching descriptor vectors using FLANN matcher
	  FlannBasedMatcher matcher;
	  //std::vector< vector<DMatch> > all_matches;

	  /*
	  std::vector<DMatch> match;
	  //matcher.knnMatch( descriptors_object, descriptors_scene , all_matches, 2);
	  matcher.match( descriptors_object, descriptors_scene , match);


	  //all_matches.push_back(match);
	  //std::vector<vector<DMatch>> all_good_matches;

	  //for (int j = 0; j < number_of_matches; j++){
	  double max_dist = 0; double min_dist = 100;

	  //-- Quick calculation of max and min distances between keypoints
	  for( int i = 0; i < descriptors_object.rows; i++ )
	  {
		  double dist = match[i].distance;
		  if( dist < min_dist ) min_dist = dist;
		  if( dist > max_dist ) max_dist = dist;
	  }

	  //printf("-- Max dist : %f \n", max_dist );
	  //printf("-- Min dist : %f \n", min_dist );

		//-- Draw only "good" matches (i.e. whose distance is less than 3*min_dist )
	  std::vector< DMatch > good_matches;

	  float nndr_ratio = 3;
	  for( int i = 0; i < descriptors_object.rows; i++ )
	  { if( match[i].distance < max(nndr_ratio * min_dist, 0.02) )
	  { good_matches.push_back( match[i]); }
	  }
	   */

	  std::vector<vector<DMatch > > matches;
	  matcher.knnMatch(descriptors_object, descriptors_scene, matches, 2);
	  std::vector< DMatch > good_matches;
	  float nndr_ratio = 0.7f;
	  for(int i = 0; i < min(descriptors_object.rows-1,(int) matches.size()); i++) //THIS LOOP IS SENSITIVE TO SEGFAULTS
	  {
	      if((matches[i][0].distance < nndr_ratio*(matches[i][1].distance)) && ((int) matches[i].size()<=2 && (int) matches[i].size()>0))
	      {
	          good_matches.push_back(matches[i][0]);
	      }
	  }

	  if (good_matches.size() < 8){
		  continue;
	  }


	  //all_good_matches.push_back(good_matches);


	  /*
		  drawMatches( img_object_gray, keypoints_object, img_scene_gray, keypoints_scene,
						good_matches, img_matches, Scalar::all(-1), Scalar::all(-1),
					   vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS );
	   */



	  //-- Localize the object from img_1 in img_2
	  std::vector<Point2f> obj;
	  std::vector<Point2f> scene;

	  for( size_t i = 0; i < good_matches.size(); i++ )
	  {
		  //-- Get the keypoints from the good matches
		  obj.push_back( keypoints_object[ good_matches[i].queryIdx ].pt );
		  scene.push_back( keypoints_scene[ good_matches[i].trainIdx ].pt );
	  }

	  Mat H;

	  try
	  {

		  H = findHomography( obj, scene, CV_RANSAC );


		  //-- Get the corners from the image_1 ( the object to be "detected" )
		  std::vector<Point2f> obj_corners(4);
		  obj_corners[0] = Point(0,0);
		  obj_corners[1] = Point( img_object_gray.cols, 0 );
		  obj_corners[2] = Point( img_object_gray.cols, img_object_gray.rows );
		  obj_corners[3] = Point( 0, img_object_gray.rows );
		  std::vector<Point2f> scene_corners(4);

		  perspectiveTransform( obj_corners, scene_corners, H);
		  /*
		  for (int i = 0; i <= 3; i++)
			  std::cout << obj_corners[i] <<"\n";

		  for (int i = 0; i <= 3; i++)
			  std::cout << scene_corners[i] << "\n";
		   */

		  //-- Draw lines between the corners (the mapped object in the scene - image_2 )
		  //Point2f offset( (float)img_object_gray.cols, 0);
		  Point2f offset( 0, 0);
		  line( img_scene, scene_corners[0] + offset, scene_corners[1] + offset, Scalar(0, 255, 0), 4 );
		  line( img_scene, scene_corners[1] + offset, scene_corners[2] + offset, Scalar( 0, 255, 0), 4 );
		  line( img_scene, scene_corners[2] + offset, scene_corners[3] + offset, Scalar( 0, 255, 0), 4 );
		  line( img_scene, scene_corners[3] + offset, scene_corners[0] + offset, Scalar( 0, 255, 0), 4 );
	  }
	  catch(Exception& e){}
	}
}


  /*
  //-- Show detected matches
  //cv::namedWindow("Good Matches & Object detection");
  Mat resized_img_matches = img_matches;
  //cv::resize(img_matches, resized_img_matches, cv::Size(), 0.1, 0.1, cv::INTER_LANCZOS4);

  imshow( "Good Matches & Object detection", resized_img_matches );
  */

  //cv::waitKey(0);
//}



void readme()
{ printf(" Usage: ./SURF_Homography <img1>\n"); }






