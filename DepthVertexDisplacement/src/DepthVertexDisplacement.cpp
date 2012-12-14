#include "cinder/app/AppBasic.h"
#include "cinder/Rand.h"
#include "cinder/gl/Vbo.h"
#include "cinder/TriMesh.h"
#include "cinder/gl/GlslProg.h"

#include "cinder/ImageIo.h"
#include "cinder/gl/Texture.h"

#include "cinder/params/Params.h"
#include "cinder/MayaCamUI.h"

using namespace ci;
using namespace ci::app;

#include <list>
using namespace std;

class DepthVertexDisplacement : public AppBasic {
 public:
	void mouseDown( MouseEvent event );
	void mouseDrag( MouseEvent event );
	void keyDown( KeyEvent event );

	void draw();
	void setup();
	void update();
protected:
	void makeMesh();

	gl::VboMesh		mMesh;
	gl::GlslProg	mDisplaceShader;

	gl::Texture		mTexture;

	params::InterfaceGl mParams;
	int					mMeshStepSize;
	int					mPrevMeshStepSize;
	float				mMultiplier;
	bool				mEnableWireFrame;

	int					mScreenWidth;
	int					mScreenHeight;

	MayaCamUI			mCam;
};

void DepthVertexDisplacement::mouseDown( MouseEvent event ){
	mCam.mouseDown( event.getPos() );
}

void DepthVertexDisplacement::mouseDrag( MouseEvent event )
{
	mCam.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
}

void DepthVertexDisplacement::keyDown( KeyEvent event )
{
}

void DepthVertexDisplacement::update(){
	if( mMeshStepSize != mPrevMeshStepSize ){
		mPrevMeshStepSize = mMeshStepSize;

		makeMesh();
	}
}

void DepthVertexDisplacement::draw()
{
	gl::clear( ColorA( 0.2f, 0.2f, 0.2f ) );

	gl::pushMatrices();
	gl::setMatrices( mCam.getCamera() );

	gl::color( Color::white() );
	glLineWidth( 1.0f ); 

	gl::translate( 0.0f, (float)mScreenHeight, 0.0f );
	gl::scale( 1.0f, -1.0f, 1.0f );

	if( mEnableWireFrame )
		gl::enableWireframe();
	else
		gl::disableWireframe();

	mTexture.enableAndBind();
	mDisplaceShader.bind();
	mDisplaceShader.uniform( "multiplier", mMultiplier );
	mDisplaceShader.uniform( "tex", 0 );

	gl::draw( mMesh );

	mDisplaceShader.unbind();

	gl::popMatrices();

	mParams.draw();
}

void DepthVertexDisplacement::makeMesh(){
	TriMesh mesh;
	for( int x = 0 ; x < (mScreenWidth-1)/mMeshStepSize ; x++ ){
		for( int y = 0 ; y < (mScreenHeight-1)/mMeshStepSize ; y++ ){
			Vec3f p1 = Vec3f( (float)( x * mMeshStepSize ), (float)( y * mMeshStepSize ), 0.0f );
			Vec3f p2 = Vec3f( (float)( x * mMeshStepSize ) + mMeshStepSize, (float)( y * mMeshStepSize ), 0.0f );
			Vec3f p3 = Vec3f( (float)( x * mMeshStepSize ), (float)( y * mMeshStepSize ) + mMeshStepSize, 0.0f );

			mesh.appendVertex( p1 );
			mesh.appendTexCoord( Vec2f( p1.x / (float)mScreenWidth, p1.y / (float)mScreenHeight ) );
			mesh.appendVertex( p2 );
			mesh.appendTexCoord( Vec2f( p2.x / (float)mScreenWidth, p2.y / (float)mScreenHeight ) );
			mesh.appendVertex( p3 );
			mesh.appendTexCoord( Vec2f( p3.x / (float)mScreenWidth, p3.y / (float)mScreenHeight ) );
			
			mesh.appendTriangle( mesh.getNumVertices() - 3, mesh.getNumVertices() - 2, mesh.getNumVertices() - 1 );

			p1 = Vec3f( (float)( x * mMeshStepSize ) + mMeshStepSize, (float)( y * mMeshStepSize ), 0.0f );
			p2 = Vec3f( (float)( x * mMeshStepSize ) + mMeshStepSize, (float)( y * mMeshStepSize ) + mMeshStepSize, 0.0f );
			p3 = Vec3f( (float)( x * mMeshStepSize ), (float)( y * mMeshStepSize ) + mMeshStepSize, 0.0f ); 

			mesh.appendVertex( p1 );
			mesh.appendTexCoord( Vec2f( p1.x / (float)mScreenWidth, p1.y / (float)mScreenHeight ) );
			mesh.appendVertex( p2 );
			mesh.appendTexCoord( Vec2f( p2.x / (float)mScreenWidth, p2.y / (float)mScreenHeight ) );
			mesh.appendVertex( p3 );
			mesh.appendTexCoord( Vec2f( p3.x / (float)mScreenWidth, p3.y / (float)mScreenHeight ) );

			mesh.appendTriangle( mesh.getNumVertices() - 3, mesh.getNumVertices() - 2, mesh.getNumVertices() - 1 );
		}
	}
	
	mMesh = gl::VboMesh( mesh );
}

void DepthVertexDisplacement::setup(){
	// for now, use 640 x 480 as resolution for depth image, color image and screen resolution
	mScreenWidth = 640;
	mScreenHeight = 480;

	//
	app::setWindowSize( mScreenWidth, mScreenHeight );

	//
	mMultiplier = 0.0f;
	mMeshStepSize = 3;
	mPrevMeshStepSize = mMeshStepSize;
	mEnableWireFrame = false;

	//
	mParams = params::InterfaceGl( "Parameters", Vec2i( 200, 100 ) );
	mParams.addParam( "Multiplier", &mMultiplier );
	mParams.addParam( "Mesh density", &mMeshStepSize, "min = 1" );
	mParams.addParam( "Enable wireframe", &mEnableWireFrame );

	//
	app::setFrameRate( 1000.0f );
	gl::disableVerticalSync();
	gl::enableDepthRead();
	gl::enableDepthWrite();

	//
	try{
		mDisplaceShader = gl::GlslProg( app::loadAsset("displace_v.glsl"), app::loadAsset("displace_f.glsl") );
	}catch( gl::Exception & e ){
		app::console() << e.what() << std::endl;
	}

	//
	mTexture = gl::Texture( loadImage( app::loadAsset( "texture.jpg" ) ) );

	//
	makeMesh();

	// setup camera and inverse the world up, because for some reason ??? mayaCam seems to flip it
	CameraPersp cameraPerspective;
	cameraPerspective.setEyePoint( Vec3f( mScreenWidth/2.0f, mScreenHeight/2.0f, 400.0f ) );
	cameraPerspective.setCenterOfInterestPoint( Vec3f(mScreenWidth/2.0f, mScreenHeight/2.0f, 0.0f) );
	cameraPerspective.setPerspective( 60.0f, getWindowAspectRatio(), 1.0f, 1000.0f );

	// make maya cam 
	mCam = MayaCamUI( cameraPerspective );
}

// This line tells Flint to actually create the application
CINDER_APP_BASIC( DepthVertexDisplacement, RendererGl )