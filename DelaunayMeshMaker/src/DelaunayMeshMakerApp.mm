// NativeControls sample by Rich Eakin
// This sample demonstrates how to integrate your app with native iOS controls and view controllers.
// It demostrates the following:
//
// - how to specify your own root UIViewController and later add CinderView's to it
// - how add a CinderView as a child of another ViewController
// - how to connect native controls back up to your App class so your callbacks are in C++
//
// Note: this sample is compiled with ARC enabled

#include "cinder/app/AppNative.h"
#include "cinder/app/CinderViewCocoaTouch.h"
#include "cinder/Font.h"
#include "cinder/TriMesh.h"
#include "cinder/gl/Vbo.h"
#include "cinder/params/Params.h"
#include "cinder/gl/Texture.h"
#include "cinder/ImageIo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Fbo.h"
#include "cinder/Rand.h"
#include "cinder/ImageIo.h"
#include "cinder/Capture.h"

#include "CinderOpenCV.h"
#include "Tesselator.h"

#include "NativeViewController.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class DelaunayMeshMakerApp : public AppNative {
  public:
	void prepareSettings( Settings *settings );
	void		setup();
	void		draw();
	void		update();
    
    void        pictureTaken( Surface surface );
protected:
	void		tesselate();
    
	ci::TriMesh			mMesh;
    
	std::vector<Vec2f>	mBorderPoints;
	std::vector<Vec2f>	mSteinerPoints;
    
	std::vector<float>	mDepthRandom;
    
	Surface8u			mSurface;
	Surface8u			mEdgeDetectSurface;
    
	float				mAlpha;
	float				mDepthOffset; float mPrevDepthOffset;
	bool				mBlend; bool mPrevBlend;
	bool				mDrawWireframe;
	
	bool				mApplyPhongShading;
	gl::Fbo				mFbo;
    
	Surface8u			edgeDetect( Surface8u surface, double minThreshold, double maxThreshold, int scaleDown = 16 );
	double				mCannyMinThreshold;
	double				mCannyMaxThreshold;
	int					mScaleDown; int mScaleDownPrev;
    
	TesselatorRef		mTesselator;
	bool				mNeedsProcessing;
	void				onTesselationDone();
    bool                mHasTesselated;
    
	NativeViewController    *mNativeController;
};

void DelaunayMeshMakerApp::tesselate(){
	mEdgeDetectSurface = edgeDetect( mSurface , mCannyMinThreshold, mCannyMaxThreshold, mScaleDownPrev );
	mNeedsProcessing = true;
}

void DelaunayMeshMakerApp::onTesselationDone(){
	mMesh = mTesselator->getMesh();
	mDepthRandom = mTesselator->getDepthRandom();
}

void DelaunayMeshMakerApp::prepareSettings( Settings *settings )
{
	mNativeController = [NativeViewController new];
	settings->prepareWindow( Window::Format().rootViewController( mNativeController ) );
}

void DelaunayMeshMakerApp::setup()
{
	// Example of how to add Cinder's UIViewController to your native root UIViewViewController
	[mNativeController addCinderViewToFront];

    //[mNativeController.navigationItem.titleView setHidden:YES];
    // Example of how to add Cinder's UIView to your view heirarchy (comment above out first)
	//[mNativeController addCinderViewAsBarButton];

	// Example of how to add a std::function callback from a UIControl (NativeViewController's info button in the upper right)
	[mNativeController setPictureTaken: bind( &DelaunayMeshMakerApp::pictureTaken, this, std::__1::placeholders::_1 ) ];
    
    //mSurface = loadImage( app::loadAsset( "texture0.jpg" ) );
    
	mMesh.clear();
    
	mScaleDown			= 14;
	mScaleDownPrev		= mScaleDown;
	mCannyMinThreshold	= 60.0;
	mCannyMaxThreshold	= 70.0;
	mAlpha				= 0.0f;
	mBlend				= false;
	mApplyPhongShading	= false;
	mDrawWireframe		= false;
	mDepthOffset		= 0.0f;
	mPrevDepthOffset	= mDepthOffset;
	mPrevBlend			= mBlend;
    
    mHasTesselated      = false;
    
	mTesselator = Tesselator::makeTesselator();
	mTesselator->sDone.connect( boost::bind( &DelaunayMeshMakerApp::onTesselationDone, this ) );
}

Surface8u DelaunayMeshMakerApp::edgeDetect( Surface8u surface, double minThreshold, double maxThreshold, int scaleDown ){
	// translate from cinder to OCV
	cv::Mat cvColorImage		= ci::toOcv( surface );
	cv::Mat cvResizedImage;
	cv::resize(cvColorImage, cvResizedImage, cv::Size( cvColorImage.cols/scaleDown, cvColorImage.rows/scaleDown ) );
	
	// make mat grayscale
	cv::Mat cvGrayScaleImage;
	cvtColor( cvResizedImage, cvGrayScaleImage, CV_RGB2GRAY );
    
	// create black and white output image from edge detection
	cv::Mat cvOutputImage;
	cv::Canny( cvGrayScaleImage, cvOutputImage, minThreshold, maxThreshold );
    
	// extract contours
	vector<vector<cv::Point> > contours;
	vector<cv::Vec4i> hierarchy;
	cv::findContours( cvOutputImage, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE );
    
	// copy steiner points
	mSteinerPoints.clear();
	for( auto itr = contours.begin(); itr != contours.end(); ++itr ){
		for( auto sec_itr = (*itr).begin(); sec_itr != (*itr).end(); ++sec_itr ){
			mSteinerPoints.push_back( Vec2f( (*sec_itr).x, (*sec_itr).y ) * scaleDown );
		}
	}
    
    // make border points
    mBorderPoints.clear();
    mBorderPoints.push_back( Vec2f( 0.0f, 0.0f ) );
    mBorderPoints.push_back( Vec2f( surface.getWidth(), 0.0f ) );
    mBorderPoints.push_back( Vec2f( surface.getWidth(), surface.getHeight() ) );
    mBorderPoints.push_back( Vec2f( 0.0f, surface.getHeight() ) );
    
	// translate output image back to cinder
	return Surface8u( ci::fromOcv( cvOutputImage ) );
}


void DelaunayMeshMakerApp::pictureTaken( Surface surface )
{
    /*
	UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Bonzo!!"
													message:@"This button callback was triggered by a std::function."
												   delegate:nil
										  cancelButtonTitle:@"Ok"
										  otherButtonTitles:nil];
	[alert show];*/

    mSurface = surface;
    tesselate();
    mHasTesselated = true;
}

void DelaunayMeshMakerApp::update()
{
	mTesselator->update();
    
	if( mNeedsProcessing && !mTesselator->isRunning() ){
		mNeedsProcessing = false;
		mTesselator->process( mBorderPoints, mSteinerPoints, mBlend, mSurface, mDepthOffset );
	}
    
	if( mBlend != mPrevBlend ){
		mPrevBlend = mBlend;
	}
	
	if( mDepthOffset != mPrevDepthOffset ){
		std::vector<Vec3f> verts = mMesh.getVertices();
		std::vector<Vec3f> newVerts;
        
		int count = 0;
		for( auto itr = verts.begin(); itr != verts.end(); ++itr ){
			newVerts.push_back( Vec3f( (*itr).x, (*itr).y, mDepthRandom[count] * mDepthOffset ) );
			count++;
		}
        
		mMesh.getVertices().swap( newVerts );
		mMesh.recalculateNormals();
        
		mPrevDepthOffset = mDepthOffset;
	}
    
	if( mScaleDown != mScaleDownPrev ){
		mScaleDownPrev = mScaleDown;
		mEdgeDetectSurface = edgeDetect( Surface8u( mSurface ), mCannyMinThreshold, mCannyMaxThreshold, mScaleDownPrev );
	}
}

void DelaunayMeshMakerApp::draw()
{
    gl::clear( Color( 0.25f, 0.25f, 0.25f ) );
    
    gl::pushModelView();
    gl::scale( 0.5f, 0.5f );
    
    if( mHasTesselated && !mTesselator->isRunning() ){
        gl::pushModelView();
        gl::color( ColorA::white() );
        
        if( mMesh.getNumTriangles() > 0 ){
            gl::enableDepthRead();
            gl::enableDepthWrite();
            
            gl::disableAlphaBlending();
            
            gl::draw( mMesh );
        }
        
        if( mAlpha > 0.0f ){
            gl::disableDepthRead();
            gl::disableDepthWrite();
            
            gl::enableAlphaBlending();
            
            gl::color( ColorA( 1,1,1, mAlpha ) );
            gl::draw( mSurface );
        }
        gl::popModelView();
    }else{
        gl::color( ColorA::white() );
        
        if( mSurface )
            gl::draw( mSurface );
    }
    gl::popModelView();
    
    gl::pushModelView();
    if( mTesselator->isRunning() ){
        
        gl::disableDepthRead();
        gl::disableDepthWrite();
        
        gl::enableAlphaBlending();
        
        gl::color( ColorA(0.0f, 0.0f, 0.0f, 0.5f ) );
        gl::drawSolidRect( Rectf( 0, 0, app::getWindowWidth(), app::getWindowHeight() ) );
        
        gl::pushModelView();
        gl::translate( app::getWindowSize()/2 );
        double time = getElapsedSeconds();
        double fraction = time - (int) time;
        int numFractions = 12;
        
        for(int i=0;i<numFractions;++i) {
            float a = (float) (fraction + i * (1.0f / (float)numFractions));
            a -= (int) a;
            
            gl::pushModelView();
            gl::rotate( i * ( -360.0f/(float)numFractions ) );
            gl::color( ColorA(1,1,1,1-a) );
            gl::drawSolidRect( Rectf(-6.0f, -44.0f, +6.0f, -31.0f) );
            gl::popModelView();
        }
        gl::popModelView();
    }
    gl::popModelView();
}

CINDER_APP_NATIVE( DelaunayMeshMakerApp, RendererGl )
