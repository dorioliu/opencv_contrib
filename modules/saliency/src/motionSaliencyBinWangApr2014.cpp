/*M///////////////////////////////////////////////////////////////////////////////////////
 //
 //  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
 //
 //  By downloading, copying, installing or using the software you agree to this license.
 //  If you do not agree to this license, do not download, install,
 //  copy or use the software.
 //
 //
 //                           License Agreement
 //                For Open Source Computer Vision Library
 //
 // Copyright (C) 2013, OpenCV Foundation, all rights reserved.
 // Third party copyrights are property of their respective owners.
 //
 // Redistribution and use in source and binary forms, with or without modification,
 // are permitted provided that the following conditions are met:
 //
 //   * Redistribution's of source code must retain the above copyright notice,
 //     this list of conditions and the following disclaimer.
 //
 //   * Redistribution's in binary form must reproduce the above copyright notice,
 //     this list of conditions and the following disclaimer in the documentation
 //     and/or other materials provided with the distribution.
 //
 //   * The name of the copyright holders may not be used to endorse or promote products
 //     derived from this software without specific prior written permission.
 //
 // This software is provided by the copyright holders and contributors "as is" and
 // any express or implied warranties, including, but not limited to, the implied
 // warranties of merchantability and fitness for a particular purpose are disclaimed.
 // In no event shall the Intel Corporation or contributors be liable for any direct,
 // indirect, incidental, special, exemplary, or consequential damages
 // (including, but not limited to, procurement of substitute goods or services;
 // loss of use, data, or profits; or business interruption) however caused
 // and on any theory of liability, whether in contract, strict liability,
 // or tort (including negligence or otherwise) arising in any way out of
 // the use of this software, even if advised of the possibility of such damage.
 //
 //M*/

#include "precomp.hpp"

namespace cv
{

cv::Ptr<Size> MotionSaliencyBinWangApr2014::getWsize()
{
  return imgSize;
}
void MotionSaliencyBinWangApr2014::setWsize( const cv::Ptr<Size>& newSize )
{
  imgSize = newSize;
}

MotionSaliencyBinWangApr2014::MotionSaliencyBinWangApr2014()
{
  N_DS = 2;  // Number of template to be downsampled and used in lowResolutionDetection function
  K = 3;  // Number of background model template
  N = 4;   // NxN is the size of the block for downsampling in the lowlowResolutionDetection
  alpha = 0.01;  // Learning rate
  L0 = 6000;  // Upper-bound values for C0 (efficacy of the first template (matrices) of backgroundModel
  L1 = 4000;  // Upper-bound values for C1 (efficacy of the second template (matrices) of backgroundModel
  thetaL = 2500;  // T0, T1 swap threshold
  thetaA = 200;
  gamma = 3;

  className = "BinWangApr2014";
}

bool MotionSaliencyBinWangApr2014::init()
{

  epslonPixelsValue = Mat( imgSize->height, imgSize->width, CV_32F );
  potentialBackground = Mat( imgSize->height, imgSize->width, CV_32FC2 );
  backgroundModel = std::vector<Mat>( K + 1, Mat::zeros( imgSize->height, imgSize->width, CV_32FC2 ) );
  //TODO set to nan
  potentialBackground.setTo( 0 );

  //TODO set to nan
  for ( size_t i = 0; i < backgroundModel.size(); i++ )
  {
    backgroundModel[i].setTo( 0 );
  }

  epslonPixelsValue.setTo( 48.5 );  // Median of range [18, 80] advised in reference paper.
                                    // Since data is even, the median is estimated using two values ​​that occupy
                                    // the position (n / 2) and ((n / 2) +1) (choose their arithmetic mean).

  /* epslonPixelsValue = Mat::zeros( imgSize->height, imgSize->width, CV_8U );
   potentialBackground = Mat::NAN( imgSize->height, imgSize->width, CV_32FC2 );
   backgroundModel = std::vector<Mat>( 4, Mat::zeros( imgSize->height, imgSize->width, CV_32FC2 ) );*/

  return true;

}

MotionSaliencyBinWangApr2014::~MotionSaliencyBinWangApr2014()
{

}

// classification (and adaptation) functions
bool MotionSaliencyBinWangApr2014::fullResolutionDetection( const Mat& image2, Mat& highResBFMask )
{
  Mat image=image2.clone();
  float* currentB;
  float* currentC;
  float currentPixelValue;
  float currentEpslonValue;
  bool backgFlag = false;

  // Initially, all pixels are considered as foreground and then we evaluate with the background model
  highResBFMask.create( image.rows, image.cols, CV_8UC1 );
  highResBFMask.setTo( 1 );

  uchar* pImage;
  float* pEpslon;
  uchar* pMask;

  // Scan all pixels of image
  for ( int i = 0; i < image.rows; i++ )
  {
    pImage= image.ptr<uchar>(i);
    pEpslon= epslonPixelsValue.ptr<float>(i);
    pMask= highResBFMask.ptr<uchar>(i);
    for ( int j = 0; j < image.cols; j++ )
    {
      backgFlag = false;
      // TODO replace "at" with more efficient matrix access
      //currentPixelValue = image.at<uchar>( i, j );
      //currentEpslonValue = epslonPixelsValue.at<float>( i, j );
      currentPixelValue = pImage[j];
      currentEpslonValue= pEpslon[j];


      // scan background model vector
      for ( size_t z = 0; z < backgroundModel.size(); z++ )
      {
        // TODO replace "at" with more efficient matrix access
        currentB = &backgroundModel[z].at<Vec2f>( i, j )[0];
        currentC = &backgroundModel[z].at<Vec2f>( i, j )[1];

        if( *currentC > 0 )  //The current template is active
        {
          // If there is a match with a current background template
          if( abs( currentPixelValue - * ( currentB ) ) < currentEpslonValue && !backgFlag )
          {
            // The correspondence pixel in the  BF mask is set as background ( 0 value)
            // TODO replace "at" with more efficient matrix access
            //highResBFMask.at<uchar>( i, j ) = 0;
            pMask[j]=0;
            if( ( *currentC < L0 && z == 0 ) || ( *currentC < L1 && z == 1 ) || ( z > 1 ) )
              *currentC += 1;  // increment the efficacy of this template

            *currentB = ( ( 1 - alpha ) * * ( currentB ) ) + ( alpha * currentPixelValue );  // Update the template value
            backgFlag = true;
            //break;
          }
          else
          {
            currentC -= 1;  // decrement the efficacy of this template
          }

        }
      }  // end "for" cicle of template vector

    }
  }  // end "for" cicle of all image's pixels

  return true;
}

bool MotionSaliencyBinWangApr2014::lowResolutionDetection( const Mat& image, Mat& lowResBFMask )
{
  //double t = (double) getTickCount();

  float currentPixelValue;
  float currentEpslonValue;
  float currentB;
  float currentC;

  /*// Create a mask to select ROI in the original Image and Backgound model and at the same time compute the mean
   Mat ROIMask( image.rows, image.cols, CV_8UC1 );
   ROIMask.setTo( 0 );*/

  Rect roi( Point( 0, 0 ), Size( N, N ) );
  Scalar imageROImean;
  Scalar backGModelROImean;
  Mat currentModel;

  // Initially, all pixels are considered as foreground and then we evaluate with the background model
  //lowResBFMask.create( image.size().height / ( N * N ), image.size().width / ( N * N ), CV_8UC1 );
  //lowResBFMask.setTo( 1 );

  lowResBFMask.create( image.rows, image.cols, CV_8UC1 );
  lowResBFMask.setTo( 1 );
  /*t = ( (double) getTickCount() - t ) / getTickFrequency();
   cout << "INITIALIZATION TIME: " << t << "s" << endl << endl;*/

  // Scan all the ROI of original matrices
  for ( int i = 0; i < image.rows / N; i++ )
  {
    for ( int j = 0; j < image.cols / N; j++ )
    {
      //double t = (double) getTickCount();
      //double t1 = (double) getTickCount();
      // Reset and update ROI mask
      //ROIMask.setTo( 0 );
      //rectangle( ROIMask, roi, Scalar( 255 ), FILLED );

      //double t3 = (double) getTickCount();
      // Compute the mean of image's block and epslonMatrix's block based on ROI
      // TODO replace "at" with more efficient matrix access
      Mat roiImage = image( roi );
      Mat roiEpslon = epslonPixelsValue( roi );
      currentPixelValue = mean( roiImage ).val[0];
      currentEpslonValue = mean( roiEpslon ).val[0];

      //t3 = ( (double) getTickCount() - t3 ) / getTickFrequency();
      //     cout << "MEAN time: " << t3 << "s" << endl << endl;

      //t1 = ( (double) getTickCount() - t1 ) / getTickFrequency();
      //cout << "FASE 1 time: " << t1 << "s" << endl << endl;

      //double t2 = (double) getTickCount();
      // scan background model vector
      for ( int z = 0; z < N_DS; z++ )
      {

        //double t = (double) getTickCount();
        // Select the current template 2 channel matrix, select ROI and compute the mean for each channel separately
        Mat roiTemplate = backgroundModel[z]( roi );
        Scalar templateMean = mean( roiTemplate );
        currentB = templateMean[0];
        currentC = templateMean[1];
        //t = ( (double) getTickCount() - t ) / getTickFrequency();
        //cout << "currentB and currentC MEAN time" << t << "s" << endl << endl;

        if( currentC > 0 )  //The current template is active
        {
          // If there is a match with a current background template
          if( abs( currentPixelValue - ( currentB ) ) < currentEpslonValue )
          {
            // The correspondence pixel in the  BF mask is set as background ( 0 value)
            // TODO replace "at" with more efficient matrix access
            //lowResBFMask.at<uchar>( i, j ) = 0;
            //lowResBFMask.setTo( 0, ROIMask );
            rectangle( lowResBFMask, roi, Scalar( 0 ), FILLED );
            break;
          }
        }
      }

      //t2 = ( (double) getTickCount() - t2 ) / getTickFrequency();
      //cout << "ALL TEMPLATE  time: " << t2 << " s" << endl << endl;
      // Shift the ROI from left to right follow the block dimension
      roi = roi + Point( N, 0 );
      //t = ( (double) getTickCount() - t ) / getTickFrequency();
      //cout << "low res PIXEL : " << t << " s" << endl << endl;
      //exit(0);
    }
    //Shift the ROI from up to down follow the block dimension, also bringing it back to beginning of row
    roi.x = 0;
    roi.y += N;
  }
  //t = ( (double) getTickCount() - t ) / getTickFrequency();
  //cout << "Scan all the ROI of original matrices time: " << t << "s" << endl << endl;

  // UPSAMPLE the lowResBFMask to the original image dimension, so that it's then possible to compare the results
  // of lowlResolutionDetection with the fullResolutionDetection
  //resize( lowResBFMask, lowResBFMask, image.size(), 0, 0, INTER_LINEAR );
  return true;
}

/*bool MotionSaliencyBinWangApr2014::templateUpdate( Mat highResBFMask )
 {

 return true;
 }*/

bool inline pairCompare( pair<float, float> t, pair<float, float> t_plusOne )
{

  return ( t.second > t_plusOne.second );

}

// Background model maintenance functions
bool MotionSaliencyBinWangApr2014::templateOrdering()
{
  vector<pair<float, float> > pixelTemplates( backgroundModel.size() );
  float temp;

  // Scan all pixels of image
  for ( int i = 0; i < backgroundModel[0].rows; i++ )
  {
    for ( int j = 0; j < backgroundModel[0].cols; j++ )
    {
      // scan background model vector from T1 to Tk
      for ( size_t z = 1; z < backgroundModel.size(); z++ )
      {
        // Fill vector of pairs
        pixelTemplates[z - 1].first = backgroundModel[z].at<Vec2f>( i, j )[0];  // Current B (background value)
        pixelTemplates[z - 1].second = backgroundModel[z].at<Vec2f>( i, j )[1];  // Current C (efficacy value)
      }

      //SORT template from T1 to Tk
      std::sort( pixelTemplates.begin(), pixelTemplates.end(), pairCompare );

      //REFILL CURRENT MODEL ( T1...Tk)
      for ( size_t zz = 1; zz < backgroundModel.size(); zz++ )
      {
        backgroundModel[zz].at<Vec2f>( i, j )[0] = pixelTemplates[zz - 1].first;  // Replace previous B with new ordered B value
        backgroundModel[zz].at<Vec2f>( i, j )[1] = pixelTemplates[zz - 1].second;  // Replace previous C with new ordered C value
      }

      // SORT Template T0 and T1
      if( backgroundModel[1].at<Vec2f>( i, j )[1] > thetaL && backgroundModel[0].at<Vec2f>( i, j )[1] < thetaL )
      {

        // swap B value of T0 with B value of T1 (for current model)
        temp = backgroundModel[0].at<Vec2f>( i, j )[0];
        backgroundModel[0].at<Vec2f>( i, j )[0] = backgroundModel[1].at<Vec2f>( i, j )[0];
        backgroundModel[1].at<Vec2f>( i, j )[0] = temp;

        // set new C0 value for current model)
        temp = backgroundModel[0].at<Vec2f>( i, j )[1];
        backgroundModel[0].at<Vec2f>( i, j )[1] = gamma * thetaL;
        backgroundModel[1].at<Vec2f>( i, j )[1] = temp;

      }

    }
  }

  return true;
}
bool MotionSaliencyBinWangApr2014::templateReplacement( const Mat& finalBFMask, const Mat& image )
{
  float roiSize = 3;  // FIXED ROI SIZE, not change until you first appropriately adjust the following controls in the EVALUATION section!
  int countNonZeroElements = NAN;
  std::vector<Mat> mv;
  Mat replicateCurrentBAMat( roiSize, roiSize, CV_32FC1 );
  Mat backgroundModelROI( roiSize, roiSize, CV_32FC1 );
  Mat diffResult( roiSize, roiSize, CV_32FC1 );

  // Scan all pixels of finalBFMask and all pixels of others models (the dimension are the same)
  for ( int i = 0; i < finalBFMask.rows; i++ )
  {
    for ( int j = 0; j < finalBFMask.cols; j++ )
    {
      /////////////////// MAINTENANCE of potentialBackground model ///////////////////
      if( finalBFMask.at<uchar>( i, j ) == 1 )  // i.e. the corresponding frame pixel has been market as foreground
      {
        /* For the pixels with CA= 0, if the current frame pixel has been classified as foreground, its value
         * will be loaded into BA and CA will be set to 1*/
        if( potentialBackground.at<Vec2f>( i, j )[1] == 0 )
        {
          potentialBackground.at<Vec2f>( i, j )[0] = image.at<uchar>( i, j );
          potentialBackground.at<Vec2f>( i, j )[1] = 1;
        }

        /*the distance between this pixel value and BA is calculated, and if this distance is smaller than
         the decision threshold epslon, then CA is increased by 1, otherwise is decreased by 1*/
        else if( abs( image.at<uchar>( i, j ) - potentialBackground.at<Vec2f>( i, j )[0] ) < epslonPixelsValue.at<float>( i, j ) )
        {
          potentialBackground.at<Vec2f>( i, j )[1] += 1;
        }
        else
        {
          potentialBackground.at<Vec2f>( i, j )[1] -= 1;
        }
        /*}*/  /////////////////// END of potentialBackground model MAINTENANCE///////////////////
        /////////////////// EVALUATION of potentialBackground values ///////////////////
        if( potentialBackground.at<Vec2f>( i, j )[1] > thetaA )
        {
          // replicate currentBA value
          replicateCurrentBAMat.setTo( potentialBackground.at<Vec2f>( i, j )[0] );

          for ( size_t z = 0; z < backgroundModel.size(); z++ )
          {
            // Neighborhood of current pixel in the current background model template.
            // The ROI is centered in the pixel coordinates

            /*if( ( i - floor( roiSize / 2 ) >= 0 ) && ( j - floor( roiSize / 2 ) >= 0 )
             && ( i + floor( roiSize / 2 ) <= ( backgroundModel[z].rows - 1 ) )
             && ( j + floor( roiSize / 2 ) <= ( backgroundModel[z].cols - 1 ) ) ) */
            if( i > 0 && j > 0 && i < ( backgroundModel[z].rows - 1 ) && j < ( backgroundModel[z].cols - 1 ) )
            {
              split( backgroundModel[z], mv );
              backgroundModelROI = mv.at( 0 )( Rect( j - floor( roiSize / 2 ), i - floor( roiSize / 2 ), roiSize, roiSize ) );
            }
            else if( i == 0 && j == 0 )  // upper left
            {
              split( backgroundModel[z], mv );
              backgroundModelROI = mv.at( 0 )( Rect( j, i, ceil( roiSize / 2 ), ceil( roiSize / 2 ) ) );
            }
            else if( j == 0 && i > 0 && i < ( backgroundModel[z].rows - 1 ) )  // middle left
            {
              split( backgroundModel[z], mv );
              backgroundModelROI = mv.at( 0 )( Rect( j, i - floor( roiSize / 2 ), ceil( roiSize / 2 ), roiSize ) );
            }
            else if( i == ( backgroundModel[z].rows - 1 ) && j == 0 )  //down left
            {
              split( backgroundModel[z], mv );
              backgroundModelROI = mv.at( 0 )( Rect( j, i - floor( roiSize / 2 ), ceil( roiSize / 2 ), ceil( roiSize / 2 ) ) );
            }
            else if( i == 0 && j > 0 && j < ( backgroundModel[z].cols - 1 ) )  // upper - middle
            {
              split( backgroundModel[z], mv );
              backgroundModelROI = mv.at( 0 )( Rect( ( j - floor( roiSize / 2 ) ), i, roiSize, ceil( roiSize / 2 ) ) );
            }
            else if( i == ( backgroundModel[z].rows - 1 ) && j > 0 && j < ( backgroundModel[z].cols - 1 ) )  //down middle
            {
              split( backgroundModel[z], mv );
              backgroundModelROI = mv.at( 0 )( Rect( j - floor( roiSize / 2 ), i - floor( roiSize / 2 ), roiSize, ceil( roiSize / 2 ) ) );
            }
            else if( i == 0 && j == ( backgroundModel[z].cols - 1 ) )  // upper right
            {
              split( backgroundModel[z], mv );
              backgroundModelROI = mv.at( 0 )( Rect( j - floor( roiSize / 2 ), i, ceil( roiSize / 2 ), ceil( roiSize / 2 ) ) );
            }
            else if( j == ( backgroundModel[z].cols - 1 ) && i > 0 && i < ( backgroundModel[z].rows - 1 ) )  // middle - right
            {
              split( backgroundModel[z], mv );
              backgroundModelROI = mv.at( 0 )( Rect( j - floor( roiSize / 2 ), i - floor( roiSize / 2 ), ceil( roiSize / 2 ), roiSize ) );
            }
            else if( i == ( backgroundModel[z].rows - 1 ) && j == ( backgroundModel[z].cols - 1 ) )  // down right
            {
              split( backgroundModel[z], mv );
              backgroundModelROI = mv.at( 0 )( Rect( j - floor( roiSize / 2 ), i - floor( roiSize / 2 ), ceil( roiSize / 2 ), ceil( roiSize / 2 ) ) );
            }

            /* Check if the value of current pixel BA in potentialBackground model is already contained in at least one of its neighbors'
             * background model
             */
            resize( replicateCurrentBAMat, replicateCurrentBAMat, Size( backgroundModelROI.cols, backgroundModelROI.rows ), 0, 0, INTER_LINEAR );
            resize( diffResult, diffResult, Size( backgroundModelROI.cols, backgroundModelROI.rows ), 0, 0, INTER_LINEAR );

            absdiff( replicateCurrentBAMat, backgroundModelROI, diffResult );
            threshold( diffResult, diffResult, epslonPixelsValue.at<float>( i, j ), 255, THRESH_BINARY_INV );
            countNonZeroElements = countNonZero( diffResult );

            if( countNonZeroElements > 0 )
            {
              /////////////////// REPLACEMENT of backgroundModel template ///////////////////
              //replace TA with current TK

              //TODO CHECK BACKGROUND MODEL COUNTER ASSIGNEMENT
              //backgroundModel[backgroundModel.size()-1].at<Vec2f>( i, j )[0]=potentialBackground.at<Vec2f>( i, j )[0];
              //backgroundModel[backgroundModel.size()-1].at<Vec2f>( i, j )[1]= potentialBackground.at<Vec2f>( i, j )[1];
              backgroundModel[backgroundModel.size() - 1].at<Vec2f>( i, j ) = potentialBackground.at<Vec2f>( i, j );
              break;
            }
          }  // end for backgroundModel size
        }  // close if of EVALUATION
      }  // end of  if( finalBFMask.at<uchar>( i, j ) == 1 )  // i.e. the corresponding frame pixel has been market as foreground

    }  // end of second for
  }  // end of first for

  return true;
}

bool MotionSaliencyBinWangApr2014::computeSaliencyImpl( const InputArray image, OutputArray saliencyMap )
{

  Mat highResBFMask;
  Mat lowResBFMask;
  Mat not_lowResBFMask;
  Mat noisePixelsMask;
  double tt = (double) getTickCount();

  double t = (double) getTickCount();
  fullResolutionDetection( image.getMat(), highResBFMask );
  t = ( (double) getTickCount() - t ) / getTickFrequency();
  cout << "fullResolutionDetection time: " << t << "s" << endl << endl;

  t = (double) getTickCount();
  lowResolutionDetection( image.getMat(), lowResBFMask );
  t = ( (double) getTickCount() - t ) / getTickFrequency();
  cout << "lowResolutionDetection time: " << t << "s" << endl << endl;

  t = (double) getTickCount();
// Compute the final background-foreground mask. One pixel is marked as foreground if and only if it is
// foreground in both masks (full and low)
  bitwise_and( highResBFMask, lowResBFMask, saliencyMap );

// Detect the noise pixels (i.e. for a given pixel, fullRes(pixel) = foreground and lowRes(pixel)= background)
  bitwise_not( lowResBFMask, not_lowResBFMask );
  bitwise_and( highResBFMask, not_lowResBFMask, noisePixelsMask );
  t = ( (double) getTickCount() - t ) / getTickFrequency();
  cout << "and not and time: " << t << "s" << endl << endl;

  t = (double) getTickCount();
  templateOrdering();
  t = ( (double) getTickCount() - t ) / getTickFrequency();
  cout << "ordering1 : " << t << "s" << endl << endl;

  t = (double) getTickCount();
  templateReplacement( saliencyMap.getMat(), image.getMat() );
  t = ( (double) getTickCount() - t ) / getTickFrequency();
  cout << "replacement : " << t << "s" << endl << endl;

  t = (double) getTickCount();
  templateOrdering();
  t = ( (double) getTickCount() - t ) / getTickFrequency();
  cout << "ordering2 : " << t << "s" << endl << endl;

  tt = ( (double) getTickCount() - tt ) / getTickFrequency();
  cout << "TOTAL : " << tt << "s" << endl << endl;

  return true;
}

}  // namespace cv