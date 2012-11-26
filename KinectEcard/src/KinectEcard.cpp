#include "cinder/app/AppBasic.h"
#include "cinder/Rand.h"

#include "cinder/gl/Texture.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/Font.h"
#include "cinder/params/Params.h"

#include "cinder/qtime/QuickTime.h"
#include "cinder/qtime/MovieWriter.h"
#include "cinder/Utilities.h"
//#include "cinder/qtime/QuickTimeUtils.h"

#include "Kinect.h"

using namespace ci;
using namespace ci::app;

#include <list>
using namespace std;
using namespace KinectSdk;

class KinectEcard : public AppBasic {
 public:
	void		mouseDrag( MouseEvent event );
	void		keyDown( KeyEvent event );

	void		setup();

	void		update();
	void		draw();
protected:
	KinectRef	mKinect;

	// kinect callbacks
	void		onDepthSurfaceReady( ci::Surface16u depthSurface, const DeviceOptions& options );
	void		onVideoSurfaceReady( ci::Surface8u videoSurface, const DeviceOptions& options );

	// surfaces return from kinect and notifiers
	ci::Surface16u	mDepthSurface;	bool mHasNewDepthSurface;
	ci::Surface8u	mVideoSurface;	bool mHasNewVideoSurface;
	ci::Surface32f	mCorrectionMap; bool mRenderedCorrectionMap;

	// FBO's
	gl::Fbo			mDepthFbo;
	gl::Fbo			mVideoFbo;	
	gl::Fbo			mSubstractedVideoFbo;

	gl::Fbo			mMovieFbo;
	gl::Fbo			mStoredVideoFbo;

	// render call's
	void			renderDepth();
	void			renderVideo();
	void			renderSubstractedVideo();
	void			renderMovie();
	void			renderSubstractedVideoAgainstMovie();

	// shaders
	gl::GlslProg	mSubstractShader;
	gl::GlslProg	mDepthCompareShader;
	gl::GlslProg	mDepthCorrectionShader;

	// mParams
	params::InterfaceGl		mParams;
	bool					mUseDepthCorrection;
	bool					mDrawDepthCorrectionMap;
	int						mFps;

	//
	void					clear();
	void					snapshot();

	//
	ci::qtime::MovieWriter	mVideoWriter;

	ci::qtime::MovieGl	mVideo;

	bool					mRecording;

	//
	float					mPrevVideoTime;
};

void KinectEcard::setup(){
	// init app window settings
	app::setWindowSize( 1280, 960 );
	app::setWindowPos( 25, 25 );

	// kinect device options
	DeviceOptions options;
	options.enableDepth( true );
	options.enableNearMode( true );
	options.enableVideo( true );
	options.setDepthResolution( ImageResolution::NUI_IMAGE_RESOLUTION_320x240 );
	options.setVideoResolution( ImageResolution::NUI_IMAGE_RESOLUTION_1280x960 );

	// init and start kinect
	mKinect = Kinect::create();
	mKinect->addDepthCallback( &KinectEcard::onDepthSurfaceReady, this );
	mKinect->addVideoCallback( &KinectEcard::onVideoSurfaceReady, this );
	mKinect->removeBackground( true );
	mKinect->enableBinaryMode( false );
	mKinect->enableUserColor( true );
	mKinect->start( options );

	// init corrected video surface

	mCorrectionMap = Surface32f( 640, 480, false, SurfaceChannelOrder::RGB );
	mRenderedCorrectionMap = false;

	// init FBO's
	mDepthFbo				= gl::Fbo( 640, 480, false );
	mVideoFbo				= gl::Fbo( 640, 480, false );
	mSubstractedVideoFbo	= gl::Fbo( 640, 480, false );
	mStoredVideoFbo			= gl::Fbo( 640, 480, false );
	mMovieFbo				= gl::Fbo( 640, 480, false );

	// init shaders
	mSubstractShader		= gl::GlslProg( app::loadAsset( "passThru_vert.glsl" ), app::loadAsset( "substract_frag.glsl" ) );
	mDepthCompareShader		= gl::GlslProg( app::loadAsset( "passThru_vert.glsl" ), app::loadAsset( "depthcompare_frag.glsl" ) );
	mDepthCorrectionShader	= gl::GlslProg( app::loadAsset( "passThru_vert.glsl" ), app::loadAsset( "correction_frag.glsl" ) );

	// clear out fbo;s
	mStoredVideoFbo.bindFramebuffer();
	gl::clear( Color::white() );
	mStoredVideoFbo.unbindFramebuffer();

	// set notifiers to default values
	mHasNewVideoSurface = false;
	mHasNewDepthSurface	= false;

	// set openGL settings, should not change any time
	gl::disableDepthRead();
	gl::disableDepthWrite();

	// setup mParams
	mParams = params::InterfaceGl( "App parameters", Vec2i( 200, 150 ) );
	mParams.addParam( "Draw depth correction map", &mDrawDepthCorrectionMap, "" );
	mParams.addParam( "FPS", &mFps, "", true );
	mParams.addButton( "Clear buffers", boost::bind( &KinectEcard::clear, this ) );
	mParams.addButton( "Take snapshot", boost::bind( &KinectEcard::snapshot, this ) );
	mUseDepthCorrection = true;
	mDrawDepthCorrectionMap = false;
	mFps = 0;

	//
	mRecording		= false;
	mPrevVideoTime	= 0.0f;

	//
	mVideoWriter	= qtime::MovieWriter( getAssetPath("") / "video0.mov", 1280, 480, qtime::MovieWriter::Format( qtime::MovieWriter::CODEC_RAW, 1.0f ).enableTemporal( false ) );
}

void KinectEcard::onDepthSurfaceReady( ci::Surface16u depthSurface, const DeviceOptions& options ){
	mDepthSurface = depthSurface;
	mHasNewDepthSurface = true;
}

void KinectEcard::onVideoSurfaceReady( ci::Surface8u videoSurface, const DeviceOptions& options ){
	mVideoSurface = videoSurface;
	mHasNewVideoSurface = true;
}

void KinectEcard::mouseDrag( MouseEvent event )
{
}

void KinectEcard::keyDown( KeyEvent event )
{
	if( event.getChar() == 'f' )
		setFullScreen( ! isFullScreen() );
	else if ( event.getChar() == KeyEvent::KEY_SPACE ){
		snapshot();
	}
	else if( event.getChar() == KeyEvent::KEY_p ) {
		if( mParams.isVisible() )
			mParams.hide();
		else
			mParams.show();
	}else if( event.getChar() == KeyEvent::KEY_SPACE ){
		snapshot();
	}else if( event.getChar() == KeyEvent::KEY_ESCAPE ){
		shutdown();
	}else if( event.getChar() == KeyEvent::KEY_c ){
		clear();
	}else if( event.getChar() == KeyEvent::KEY_r ){
		if( !mRecording ){
			mRecording = true;
		}else{
			static int count = 0;
			static int prevCount = 0;

			count++;
			count%=2;
			mVideoWriter = qtime::MovieWriter( getAssetPath("") / ( "video" + ci::toString<int>(count) + ".mov" ).c_str(), 640, 480, qtime::MovieWriter::Format( qtime::MovieWriter::CODEC_RAW, 1.0f ).enableTemporal( false ) );
		
			mVideo = qtime::MovieGl( loadAsset( "video" + ci::toString<int>(prevCount) + ".mov") );
			prevCount++;
			prevCount%=2;

			mVideo.play();
			mVideo.setLoop();
		}
	}
}

void KinectEcard::update(){
	mKinect->update();

	if( mHasNewVideoSurface ){
		if( !mRenderedCorrectionMap ){
			int width = 640;
			int height = 480;
			
			for( int x = 0 ; x < width; x++ ){
				for( int y = 0 ; y < height ; y++ ){
					Vec2i pos = mKinect->getColorPixelCoordinateFromDepthPixel( Vec2i( x, y ), ImageResolution::NUI_IMAGE_RESOLUTION_640x480, ImageResolution::NUI_IMAGE_RESOLUTION_640x480 );
					mCorrectionMap.setPixel( Vec2i( x, y ), ColorA( ( ( pos.x )/(float)width), ( ( pos.y ) /(float)height), 0.0f, 0.0f ) );
				}
			}
			mRenderedCorrectionMap = true;
		}

		renderVideo();
	}

	if( mHasNewDepthSurface )
		renderDepth();
	
	if( mHasNewDepthSurface || mHasNewVideoSurface ){
		renderSubstractedVideo();

		if( mHasNewDepthSurface ) mHasNewDepthSurface = false;
		if( mHasNewVideoSurface ) mHasNewVideoSurface = false;
	}

	renderMovie();

	mFps = (int)getAverageFps();
}

void KinectEcard::renderMovie(){
	mMovieFbo.bindFramebuffer();
	gl::pushMatrices();
	{
		gl::clear( Color::white() );
		gl::color( Color::white() );
		gl::setMatricesWindowPersp( mMovieFbo.getSize() );

		if( mVideo )
			gl::draw( mVideo.getTexture() );
	}
	gl::popMatrices();
	mMovieFbo.unbindFramebuffer();
}

void	KinectEcard::clear(){
	mStoredVideoFbo.bindFramebuffer();
	gl::clear( Color::white() );
	mStoredVideoFbo.unbindFramebuffer();
}

void	KinectEcard::snapshot(){
	/*
	gl::Fbo fbo( 640, 480, false );

	// render stored depth FBO
	fbo.bindFramebuffer();
	gl::pushMatrices();
		gl::clear( Color::black() );
		gl::color( Color::white() );
		gl::setMatricesWindowPersp( fbo.getSize() );

		mDepthCompareShader.bind();

		mDepthCompareShader.uniform( "tex0", 0 );
		mDepthCompareShader.uniform( "tex1", 1 );

		mDepthFbo.bindTexture(0);
		mStoredDepthFbo.bindTexture(1);

		gl::translate( 0, 480 );
		gl::scale( 1, -1 );

		gl::drawSolidRect( fbo.getBounds() );

		mDepthCompareShader.unbind();
	gl::popMatrices();
	fbo.unbindFramebuffer();

	// save stored depth fbo
	fbo.blitTo( mStoredDepthFbo, fbo.getBounds(), mStoredDepthFbo.getBounds() );

	// render substracted video fbo
	fbo.bindFramebuffer();
	gl::pushMatrices();
		gl::clear( Color::black() );
		gl::color( Color::white() );
		gl::setMatricesWindowPersp( mSubstractedVideoFbo.getSize() );
		gl::draw( mSubstractedVideoFbo.getTexture() );
	gl::popMatrices();
	fbo.unbindFramebuffer();

	// save it
	fbo.blitTo( mStoredVideoFbo, fbo.getBounds(), mStoredVideoFbo.getBounds() );*/
}

void KinectEcard::renderDepth(){
	if( !mDepthSurface || !mVideoSurface )
		return;

	gl::pushMatrices();
	{
		mDepthFbo.bindFramebuffer();

		// clear FBO
		gl::clear( Color::black() );
		gl::color( Color::white() );

		//
		gl::setMatricesWindowPersp( mDepthFbo.getSize() );

		// draw depth 
		gl::pushMatrices();
			gl::scale( 2, 2 );
			gl::draw( gl::Texture( mDepthSurface ) );
		gl::popMatrices();

		mDepthFbo.unbindFramebuffer();
	}
	gl::popMatrices();
}

void KinectEcard::renderVideo(){
	if( !mVideoSurface )
		return;

	gl::pushMatrices();
	{
		//
		mVideoFbo.bindFramebuffer();

		// clear FBO
		gl::clear( Color::black() );
		gl::color( Color::white() );

		// 
		gl::Texture tex0( mVideoSurface );
		gl::Texture tex1( mCorrectionMap );

		mDepthCorrectionShader.bind();
		mDepthCorrectionShader.uniform( "tex0", 0 );
		mDepthCorrectionShader.uniform( "tex1", 1 );

		tex0.bind(0);
		tex1.bind(1);

		gl::setMatricesWindowPersp( mVideoFbo.getSize() );

		// draw video surface
		gl::drawSolidRect( mVideoFbo.getBounds() );
		//gl::draw( mCorrectionMap );

		mDepthCorrectionShader.unbind();

		mVideoFbo.unbindFramebuffer();
	}
	gl::popMatrices();
}

void KinectEcard::renderSubstractedVideo(){
	mSubstractedVideoFbo.bindFramebuffer();

	gl::pushMatrices();
	{
		// clear FBO
		gl::clear( Color::black() );
		gl::color( Color::white() );

		//
		gl::setMatricesWindowPersp( mSubstractedVideoFbo.getSize() );

		mSubstractShader.bind();
		mSubstractShader.uniform( "tex0", 0 );
		mSubstractShader.uniform( "tex1", 1 );

		mVideoFbo.bindTexture(0);
		mDepthFbo.bindTexture(1);
		
		gl::drawSolidRect( mSubstractedVideoFbo.getBounds() );

		mVideoFbo.unbindTexture();
		mDepthFbo.unbindTexture();
		
		mSubstractShader.unbind();
	}
	gl::popMatrices();

	mSubstractedVideoFbo.unbindFramebuffer();
}

void KinectEcard::renderSubstractedVideoAgainstMovie(){
	m
}

void KinectEcard::draw()
{
	gl::clear( Color::black() );
	gl::color( Color::white() );

	//
	gl::setViewport( app::getWindowBounds() );

	// draw FBO's
	gl::pushMatrices();
	{
		// draw video stream and info string
		gl::pushMatrices();
			mVideoFbo.getTexture().setFlipped();
			gl::draw( mVideoFbo.getTexture() );

			gl::pushMatrices();
				gl::enableAlphaBlending();
				gl::drawString( "Video stream ", Vec2f( 20, 10 ), ColorA::black(), Font( "Arial", 40 )  );
				gl::disableAlphaBlending();
			gl::popMatrices();
		gl::popMatrices();

		gl::pushMatrices();
			gl::translate( mVideoFbo.getWidth(), 0 );
			mDepthFbo.getTexture().setFlipped();
			gl::draw( mDepthFbo.getTexture() );

			gl::pushMatrices();
				gl::enableAlphaBlending();
				gl::drawString( "Depth stream ", Vec2f( 20, 10 ), ColorA::black(), Font( "Arial", 40 )  );
				gl::disableAlphaBlending();
			gl::popMatrices();
		gl::popMatrices();

		gl::pushMatrices();
			gl::translate( mVideoFbo.getWidth(),  mVideoFbo.getHeight() );
			gl::draw( mSubstractedVideoFbo.getTexture() );

			gl::pushMatrices();
				gl::enableAlphaBlending();
				//gl::drawString( "Depth stream ", Vec2f( 20, 10 ), ColorA::black(), Font( "Arial", 40 )  );
				gl::disableAlphaBlending();
			gl::popMatrices();
		gl::popMatrices();
		/*
		if( mVideo ){
			gl::pushMatrices();
				gl::translate( 0, mVideoFbo.getHeight() );
				gl::draw( mVideo.getTexture() );
			gl::popMatrices();
		}*/
		
		if( mDrawDepthCorrectionMap ){
			gl::pushMatrices();
			{
				gl::scale( 0.25f, 0.25f );
				gl::draw( mCorrectionMap );
			}
			gl::popMatrices();
		}
	}
	gl::popMatrices();

	// draw bounderies for niceness
	gl::pushMatrices();
		gl::color( ColorA::black() );
		gl::drawLine( Vec3i( 0, mVideoFbo.getHeight(), 0 ), Vec3i( mVideoFbo.getWidth()*2, mVideoFbo.getHeight(), 0 ) );
		gl::drawLine( Vec3i( mVideoFbo.getWidth(), 0, 0 ), Vec3i( mVideoFbo.getWidth(), mVideoFbo.getHeight()*2, 0 ) );
	gl::popMatrices();

	mParams.draw();
}

// This line tells Flint to actually create the application
CINDER_APP_BASIC( KinectEcard, RendererGl )