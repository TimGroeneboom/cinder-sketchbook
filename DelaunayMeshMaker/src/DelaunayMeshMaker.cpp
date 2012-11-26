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

#include "sweep/cdt.h"
#include "poly2tri.h"

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

	void		recalcMesh();
	
protected:
	ci::TriMesh			mMesh;
	gl::VboMesh			mVboMesh;

	std::vector<Vec2f>	mBorderPoints;
	std::vector<Vec2f>	mSteinerPoints;

	std::vector<float>	mDepthRandom;

	Surface32f			mSurface;

	params::InterfaceGl	mParams;
	float				mAlpha;
	float				mDepthOffset; float mPrevDepthOffset;
	bool				mBlend; bool mPrevBlend;
	
	bool				mApplyPhongShading;
	gl::Fbo				mFbo;
	gl::GlslProg		mPhongShader;		

	void				appendVertex( p2t::Point * p, Color & color );
};

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

		recalcMesh();
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

	gl::enableAlphaBlending();

	mParams = params::InterfaceGl( "Parameters", Vec2i( 200, 110 ) );
	mParams.addParam( "Alpha", &mAlpha, "max=1.0 min=0.0 step=0.005" );
	mParams.addParam( "Blend", &mBlend );
	mParams.addParam( "Phong Shading", &mApplyPhongShading );
	mParams.addParam( "Depth Offset", &mDepthOffset, "step=0.1" );

	mAlpha				= 1.0f;
	mBlend				= false;
	mApplyPhongShading	= false;
	mDepthOffset		= 1.0f;
	mPrevDepthOffset	= mDepthOffset;
	mPrevBlend			= mBlend;

	mPhongShader = gl::GlslProg( loadAsset("phong_vert.glsl"), loadAsset("phong_frag.glsl") );
}

void DelaunayMeshMaker::update(){
	if( mBlend != mPrevBlend ){
		mPrevBlend = mBlend;
		recalcMesh();
	}
	
	if( mDepthOffset != mPrevDepthOffset ){
		auto verts = mMesh.getVertices();
		std::vector<Vec3f> nVerts;

		for( auto itr = verts.begin(); itr != verts.end(); ++itr ){
			float z = (*itr).z;

			nVerts.push_back( Vec3f( (*itr).x, (*itr).y, (*itr).z * mDepthOffset ) );
		}

		TriMesh targetMesh = mMesh;
		targetMesh.getVertices().swap( nVerts );
		targetMesh.recalculateNormals();

		mVboMesh = gl::VboMesh( targetMesh );
		mPrevDepthOffset = mDepthOffset;
	}
}

void DelaunayMeshMaker::mouseDrag( MouseEvent event ){
}

void DelaunayMeshMaker::mouseMove( MouseEvent event ){
}

void DelaunayMeshMaker::mouseDown( MouseEvent event ){
	mSteinerPoints.push_back( event.getPos() );

	recalcMesh();
}

void DelaunayMeshMaker::recalcMesh()
{
	// clear previous mesh
	mMesh.clear();

	// get the 4 outline points of the screen 
	std::vector<p2t::Point*> borderPoints;
	for( auto itr = mBorderPoints.begin(); itr != mBorderPoints.end(); ++itr ){
		borderPoints.push_back( new p2t::Point( (*itr).x, (*itr).y ) );
	}

	// make cdt out of border points
	p2t::CDT * cdt = new p2t::CDT( borderPoints );

	// add steinerPoints within screen
	std::vector<p2t::Point*> steinerPoints;
	for( auto itr = mSteinerPoints.begin(); itr != mSteinerPoints.end(); itr++ ){
		steinerPoints.push_back( new p2t::Point( (*itr).x, (*itr).y ) );
		cdt->AddPoint( steinerPoints.back()  );
	}

	// this is where the magic happens
	cdt->Triangulate();

	// get the triangles
	std::vector<p2t::Triangle*> triangles = cdt->GetTriangles();
		
	// make mesh out of triangles
	for( size_t i = 0 ; i < triangles.size(); i++ ){
		p2t::Point *p1 = triangles[i]->GetPoint(0);
		p2t::Point *p2 = triangles[i]->GetPoint(1);
		p2t::Point *p3 = triangles[i]->GetPoint(2);

		Color color1;
		Color color2;
		Color color3;
		if( mBlend ){
			color1 = mSurface.getPixel( Vec2i( (int)p1->x, (int)p1->y ) );
			color2 = mSurface.getPixel( Vec2i( (int)p2->x, (int)p2->y ) );
			color3 = mSurface.getPixel( Vec2i( (int)p3->x, (int)p3->y ) );
		}else{
			color1 = ( mSurface.getPixel( Vec2i( (int)p1->x, (int)p1->y ) ) + mSurface.getPixel( Vec2i( (int)p2->x, (int)p2->y ) ) + mSurface.getPixel( Vec2i( (int)p3->x, (int)p3->y ) ) ) / ( 3.0f );
			color2 = color1;
			color3 = color1;
		}

		appendVertex( p1, color1 );
		appendVertex( p2, color2 );
		appendVertex( p3, color3 );

		mMesh.appendTriangle( mMesh.getVertices().size() - 3, mMesh.getVertices().size() - 2, mMesh.getVertices().size() - 1 );
	}

	ci::TriMesh targetMesh = mMesh;
	auto verts = targetMesh.getVertices();
	std::vector<Vec3f> mNewVerts;
	for( auto itr = verts.begin(); itr != verts.end(); ++itr ){
		float z = (*itr).z;

		mNewVerts.push_back( Vec3f( (*itr).x, (*itr).y, (*itr).z * mDepthOffset ) );
	}
	targetMesh.getVertices().swap( mNewVerts );
	targetMesh.recalculateNormals();

	// make a VBO Mesh out of it
	mVboMesh = gl::VboMesh( targetMesh );

	// delete allocated memory
	for( size_t i = 0 ; i < borderPoints.size(); i++ ){
		delete borderPoints[i];
	}
	for( size_t i = 0 ; i < steinerPoints.size(); i++ ){
		delete steinerPoints[i];
	}
	delete cdt;
}

void DelaunayMeshMaker::appendVertex( p2t::Point * p, Color & color )
{
	// get vertices
	std::vector<Vec3f> & verts = mMesh.getVertices();

	// the following for loop makes sure vertices on the same xy get the same 'random' z as well
	bool unique = true;
	for( auto itr = verts.cbegin(); itr != verts.cend(); ++itr ){
		if( (*itr).x == p->x && (*itr).y == p->y ){
			float r = (*itr).z;
			mDepthRandom.push_back( r );

			unique = false;
			mMesh.appendVertex( Vec3f( (float)p->x, (float)p->y, (*itr).z ) );

			break;
		}
	}
	if( unique ){
		float r = ci::randFloat( -1.0f, 1.0f );
		mDepthRandom.push_back( r );
		mMesh.appendVertex( Vec3f( (float)p->x, (float)p->y, r ) );
	}

	mMesh.appendTexCoord( Vec2f( (float)p->x, (float)p->y ) / app::getWindowSize() );
	mMesh.appendColorRgb( color );
}

void DelaunayMeshMaker::draw()
{
	gl::clear();

	gl::pushModelView();
		gl::color( ColorA::white() );
		
		if( mMesh.getNumTriangles() > 0 ){
			if( mApplyPhongShading )
				mPhongShader.bind();

			gl::enableDepthRead();
			gl::enableDepthWrite();
			gl::draw( mVboMesh );

			if( mApplyPhongShading )
				mPhongShader.unbind();
		}

		gl::color( ColorA( 1, 1, 1, mAlpha ) );
		gl::disableDepthRead();
		gl::disableDepthWrite();

		gl::draw( mSurface );
	gl::popModelView();

	mParams.draw();
}

CINDER_APP_BASIC( DelaunayMeshMaker, RendererGl )