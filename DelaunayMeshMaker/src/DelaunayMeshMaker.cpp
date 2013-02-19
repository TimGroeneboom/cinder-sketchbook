#include "cinder/app/AppBasic.h"
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
#include "cinder/MayaCamUI.h"

#include "CinderOpenCV.h"
#include "Tesselator.h"
#include "cinder/Sphere.h"

using namespace ci;
using namespace ci::app;
using namespace std;

typedef boost::shared_ptr< p2t::CDT>	CDTRef;

class DelaunayMeshMaker : public AppBasic {
public:
	void		setup();
	void		draw();
	void		update();

	void		keyDown( KeyEvent event ) { ; }
	void		mouseDown( MouseEvent event );
	void		mouseDrag( MouseEvent event );
	void		mouseMove( MouseEvent event );
	void		fileDrop( FileDropEvent event );
protected:
	void		tesselate();

	ci::TriMesh			mMesh;
	gl::VboMesh			mVboMesh;

	std::vector<Vec2f>	mBorderPoints;
	std::vector<Vec2f>	mSteinerPoints;

	std::vector<float>	mDepthRandom;

	Surface32f			mSurface;
	Surface8u			mEdgeDetectSurface;

	params::InterfaceGl	mParams;
	float				mAlpha;
	float				mDepthOffset; float mPrevDepthOffset;
	bool				mBlend; bool mPrevBlend;
	bool				mDrawWireframe;
	
	bool				mApplyPhongShading;
	gl::Fbo				mFbo;
	gl::GlslProg		mPhongShader;		

	void				appendVertex( p2t::Point * p, Color & color );
	Surface8u			edgeDetect( Surface8u surface, double minThreshold, double maxThreshold, int scaleDown = 16 );
	double				mCannyMinThreshold; 
	double				mCannyMaxThreshold;
	int					mScaleDown; int mScaleDownPrev;

	TesselatorRef		mTesselator;
	bool				mNeedsProcessing;
	void				onTesselationDone();
};

void DelaunayMeshMaker::tesselate(){
	mEdgeDetectSurface = edgeDetect( Surface8u( mSurface ), mCannyMinThreshold, mCannyMaxThreshold, mScaleDownPrev );
	mNeedsProcessing = true;
}

void DelaunayMeshMaker::onTesselationDone(){
	mMesh = mTesselator->getMesh();
	mVboMesh = gl::VboMesh( mMesh );
	mDepthRandom = mTesselator->getDepthRandom();
}

void DelaunayMeshMaker::fileDrop( FileDropEvent event ){
	try {
		mSurface = loadImage( event.getFile( 0 ) );

		int width = mSurface.getWidth();
		int height = mSurface.getHeight();
		app::setWindowSize( width, height );

		mFbo = gl::Fbo( width, height, false );

		mBorderPoints.clear();
		mBorderPoints.push_back( Vec2f::zero() );
		mBorderPoints.push_back( Vec2f( (float)width, 0 ) );
		mBorderPoints.push_back( Vec2f( (float)width ,(float)height ) );
		mBorderPoints.push_back( Vec2f( 0 ,(float)height ) );

		mSteinerPoints.clear();
		mMesh.clear();

		tesselate();
	}
	catch( ... ) {
		console() << "unable to load the texture file!" << std::endl;
	};
	
}

void DelaunayMeshMaker::setup()
{
	mSurface = loadImage( app::loadAsset( "texture0.jpg" ) );
	
	int width = mSurface.getWidth();
	int height = mSurface.getHeight();
	app::setWindowSize( width, height );

	mFbo = gl::Fbo( width, height, false );

	mBorderPoints.push_back( Vec2f::zero() );
	mBorderPoints.push_back( Vec2f( (float)width, 0 ) );
	mBorderPoints.push_back( Vec2f( (float)width ,(float)height ) );
	mBorderPoints.push_back( Vec2f( 0 ,(float)height ) );

	mMesh.clear();

	mParams = params::InterfaceGl( "Parameters", Vec2i( 300, 250 ) );
	mParams.addParam( "Alpha", &mAlpha, "max=1.0 min=0.0 step=0.005" );
	mParams.addParam( "Blend", &mBlend );
	mParams.addParam( "Phong Shading", &mApplyPhongShading );
	mParams.addParam( "Depth Offset", &mDepthOffset, "step=0.1" );
	mParams.addParam( "Draw wireframe", &mDrawWireframe );
	
	mParams.addSeparator();
	mParams.addText("Edge detection");
	mParams.addParam( "Edge detection scale down", &mScaleDown, "min=1" );
	mParams.addParam( "Minimum threshold", &mCannyMinThreshold, "min=0.0f" );
	mParams.addParam( "Maximum threshold", &mCannyMaxThreshold, "min=0.0f" );

	mParams.addSeparator();
	mParams.addButton( "Tesselate", std::bind( &DelaunayMeshMaker::tesselate, this ) );

	mScaleDown			= 8;
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

	mPhongShader = gl::GlslProg( loadAsset("phong_vert.glsl"), loadAsset("phong_frag.glsl") );

	mEdgeDetectSurface = edgeDetect( Surface8u( mSurface ), mCannyMinThreshold, mCannyMaxThreshold, mScaleDownPrev );

	mTesselator = Tesselator::makeTesselator();
	mTesselator->sDone.connect( boost::bind( &DelaunayMeshMaker::onTesselationDone, this ) );
}

Surface8u DelaunayMeshMaker::edgeDetect( Surface8u surface, double minThreshold, double maxThreshold, int scaleDown ){
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

	// translate output image back to cinder
	return Surface32f( ci::fromOcv( cvOutputImage ) );
}

void DelaunayMeshMaker::update(){
	mTesselator->update();

	if( mNeedsProcessing && mTesselator->isRunning() ){
		mTesselator->interrupt();
	}else if( mNeedsProcessing && !mTesselator->isRunning() ){
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
			float z = (*itr).z;

			newVerts.push_back( Vec3f( (*itr).x, (*itr).y, mDepthRandom[count] * mDepthOffset ) );
			count++;
		}

		mMesh.getVertices().swap( newVerts );
		mMesh.recalculateNormals();

		mVboMesh = gl::VboMesh( mMesh );
		mPrevDepthOffset = mDepthOffset;
	}

	if( mScaleDown != mScaleDownPrev ){
		mScaleDownPrev = mScaleDown;
		mEdgeDetectSurface = edgeDetect( Surface8u( mSurface ), mCannyMinThreshold, mCannyMaxThreshold, mScaleDownPrev );
	}
}


void DelaunayMeshMaker::mouseMove( MouseEvent event ){
}

void DelaunayMeshMaker::mouseDown( MouseEvent event ){
}

void DelaunayMeshMaker::mouseDrag( MouseEvent event )
{
}

void DelaunayMeshMaker::draw()
{
	gl::clear();

	gl::pushMatrices();

	gl::pushModelView();
		gl::color( ColorA::white() );

		if( mMesh.getNumTriangles() > 0 ){
			if( mApplyPhongShading )
				mPhongShader.bind();

			gl::enableDepthRead();
			gl::enableDepthWrite();

			gl::disableAlphaBlending();

			if( mDrawWireframe )
				gl::enableWireframe();

			gl::draw( mVboMesh );

			if( mDrawWireframe )
				gl::disableWireframe();

			if( mApplyPhongShading )
				mPhongShader.unbind();
		}

		if( mAlpha > 0.0f ){
			gl::disableDepthRead();
			gl::disableDepthWrite();

			gl::enableAlphaBlending();

			gl::color( ColorA( 1,1,1, mAlpha ) );
			gl::draw( mSurface );
		}
	gl::popModelView();

	gl::popMatrices();

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

	mParams.draw();
}

CINDER_APP_BASIC( DelaunayMeshMaker, RendererGl )