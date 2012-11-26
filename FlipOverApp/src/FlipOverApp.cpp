#include "cinder/app/AppBasic.h"
#include "cinder/Rand.h"
#include "cinder/gl/Vbo.h"
#include "cinder/TriMesh.h"
#include "cinder/Utilities.h"
#include "cinder/ImageIo.h"
#include "cinder/gl/Texture.h"

#include "cinder/gl/GlslProg.h"

#include "cinder/Camera.h"
//#include "cinder/params/Params.h"
#include "cinder/gl/Fbo.h"

#include "FlipOvers.h"

using namespace ci;
using namespace ci::app;

#include <list>
using namespace std;

class FlipOverApp : public AppBasic {
 public:
	void mouseDown( MouseEvent event );
	void mouseDrag( MouseEvent event );
	void mouseMove( MouseEvent event );

	void keyDown( KeyEvent event );

	void draw();
	void update();

	void setup();
	void renderColoredTiles();

protected:
	CameraPersp		mCamera;
	gl::Texture		mTexture0;
	gl::Texture		mTexture1;

	//params::InterfaceGl	mParams;
	gl::Fbo			mColorPickFbo;
	gl::Fbo			mPickedColorFbo;

	FlipOversRef	mFlipOvers;
	float			mPrevTime;

	bool			mEnableVSync;
	bool			mEnableVSyncPrev;
	bool			mEnableYFlip;
	bool			mDrawColorCode;
};

void FlipOverApp::setup(){
	mTexture0 = gl::Texture( loadImage( loadAsset( "texture0.jpg" ) ) );
	mTexture1 = gl::Texture( loadImage( loadAsset( "texture1.jpg" ) ) );

	// General openGL settings
	mTexture0.bind(0);
	mTexture1.bind(1);
	gl::disableVerticalSync();

	// Enable depth read/write
	gl::enableDepthRead();
	gl::enableDepthWrite();

	//
	gl::disableAlphaBlending();
	gl::disableAlphaTest();	

	//
	gl::enable( GL_CULL_FACE );

	/*
	mParams = params::InterfaceGl( "App Parameters", Vec2i( 200, 100 ) );
	mParams.addParam( "Enable Y-flip", &mEnableYFlip );
	mParams.addParam( "Enable V-sync", &mEnableVSync );
	mParams.addParam( "Draw Color Code", &mDrawColorCode );
	mParams.hide();
	*/
	mEnableYFlip		= false;
	mEnableVSync		= false;
	mEnableVSyncPrev	= false;
	mDrawColorCode		= false;

	Vec2i flipoverNum( 15, 5 );
	Vec2f flipoverSize( 100, 100 );
	mFlipOvers	= FlipOvers::createFlipOvers( flipoverNum, flipoverSize );
	mPrevTime	= 0.0f;

	int windowWidth		= flipoverNum.x * (int)flipoverSize.x;
	int windowHeight	= flipoverNum.y * (int)flipoverSize.y;
	app::setWindowSize( windowWidth, windowHeight );
	app::setFrameRate( 1000.0f );

	gl::Fbo::Format fmt;
	//fmt.enableMipmapping(false);
	//fmt.setMagFilter( GL_NEAREST );
	//fmt.setMinFilter( GL_NEAREST );
	//fmt.setSamples(0);
	//fmt.setCoverageSamples(0);

	mColorPickFbo		= gl::Fbo( windowWidth, windowHeight );
	mPickedColorFbo		= gl::Fbo( 1, 1 );
}

void FlipOverApp::renderColoredTiles(){
	mColorPickFbo.bindFramebuffer();

	gl::setViewport( mColorPickFbo.getBounds() );
	gl::setMatricesWindowPersp( app::getWindowSize() );

	// Front face culling
	glCullFace( GL_FRONT );
	
	gl::clear( Color( 0.2f, 0.2f, 0.2f ) );
	gl::color( Color::white() );

	gl::translate( 0, mColorPickFbo.getHeight() );
	gl::scale( 1, -1 );

	mFlipOvers->drawColored();

	mColorPickFbo.unbindFramebuffer();
	//
}

void FlipOverApp::mouseDown( MouseEvent event )
{
	mFlipOvers->mouseDown( event );
}

void FlipOverApp::mouseMove( MouseEvent event ){
	mColorPickFbo.blitTo( mPickedColorFbo, Area( event.getPos().x - 2, event.getPos().y - 2, event.getPos().x + 2, event.getPos().y + 2 ), Area( 0, 0, 4, 4 ) );
}

void FlipOverApp::mouseDrag( MouseEvent event )
{
	mFlipOvers->mouseDrag( event );
}

void FlipOverApp::keyDown( KeyEvent event )
{
	if( event.getCode() == KeyEvent::KEY_ESCAPE )
		shutdown();
	else if( event.getCode() == KeyEvent::KEY_p ){
		/*if( !mParams.isVisible() )
			mParams.show();
		else
			mParams.hide();*/
	}
	else if( event.getCode() == KeyEvent::KEY_v )
		mEnableVSync = !mEnableVSync;
	else if( event.getCode() == KeyEvent::KEY_c )
		mDrawColorCode = !mDrawColorCode;
}

void FlipOverApp::update(){
	float now		= (float) app::getElapsedSeconds();
	float elapsed	= now - mPrevTime;
	mPrevTime		= now;
	elapsed			= fmodf( elapsed, 1.0f );

	mFlipOvers->update( elapsed );
	if( mFlipOvers->getYFlipEnabled() != mEnableYFlip )
		mFlipOvers->enableYFlip( mEnableYFlip );

	if( mEnableVSync != mEnableVSyncPrev ){
		mEnableVSyncPrev = mEnableVSync;
		
		if( mEnableVSync ){
			gl::enableVerticalSync();
		}else{
			gl::disableVerticalSync();
		}
	}

	renderColoredTiles();
}

void FlipOverApp::draw()
{
	gl::setMatricesWindowPersp( app::getWindowSize() );

	gl::clear( Color( 0.2f, 0.2f, 0.2f ) );
	gl::color( Color::white() );

	if( !mDrawColorCode ){
		glCullFace( GL_FRONT );
		gl::enableDepthRead();
		gl::enableDepthWrite();
		mFlipOvers->draw();

		glCullFace( GL_BACK );
		gl::disableDepthRead();
		gl::disableDepthWrite();
		gl::translate( -50, -50 );
		gl::draw( mPickedColorFbo.getTexture(), mPickedColorFbo.getBounds(), Rectf(0,0,50,50) );
	}else{
		glCullFace( GL_BACK );
		gl::disableDepthRead();
		gl::disableDepthWrite();

		gl::draw( mColorPickFbo.getTexture() );
	}
	
	//mParams.draw();
}

// This line tells Flint to actually create the application
CINDER_APP_BASIC( FlipOverApp, RendererGl )