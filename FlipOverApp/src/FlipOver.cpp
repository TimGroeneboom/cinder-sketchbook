#include "FlipOver.h"

#include "cinder/app/AppBasic.h"
#include "cinder/CinderMath.h"
#include "cinder/ObjLoader.h"
#include "cinder/TriMesh.h"

using namespace ci;

ci::gl::VboMesh		FlipOver::mMesh;
ci::gl::GlslProg	FlipOver::mColorPickShader;

FlipOver::FlipOver( ci::Vec2f position, ci::Vec2f size ){
	mPosition	= position;
	mSize		= size;
	mRotation	= Vec3f::zero();

	if( !mMesh ){
		ObjLoader loader( app::loadAsset( "half_a_cube.obj" ) );
		
		TriMesh mesh;
		loader.load( &mesh );

		std::vector<Vec3f> vertices;
		for( auto itr = mesh.getVertices().begin(); itr < mesh.getVertices().end(); itr++ )
			vertices.push_back( Vec3f( (*itr).x * mSize.x, (*itr).y * mSize.y, 0.0f ) );
		
		mesh.getVertices().swap( vertices );

		mMesh = gl::VboMesh( mesh );
	}

	if( !mColorPickShader )
		mColorPickShader = gl::GlslProg( app::loadAsset("colorPick_vert.glsl"), app::loadAsset("colorPick_frag.glsl") );
}

void FlipOver::drawColored(){
	// TODO : bind this shader in FlipOvers for better performance
	mColorPickShader.bind();

	gl::pushModelView();

	gl::translate( mPosition );
	gl::rotate( mRotation );

	gl::color( ColorA( mColor, 0.0f, 0.0f, 1.0f ) );
	gl::draw( mMesh );

	gl::rotate( Vec3f( 0.0f, 180.0f, 0.0f ) );
	gl::color( ColorA( mColor + 0.5f, 0.0f, 0.0f, 1.0f ) );
	gl::draw( mMesh );

	gl::popModelView();

	mColorPickShader.unbind();
}

void FlipOver::draw( ci::gl::GlslProg shader ){
	gl::pushModelView();

	gl::translate( mPosition );
	gl::rotate( mRotation );

	shader.uniform( "tex", 0 );
	gl::draw( mMesh );

	shader.uniform( "tex", 1 );
	gl::rotate( Vec3f( 0.0f, 180.0f, 0.0f ) );
	gl::draw( mMesh );

	gl::popModelView();
}

void FlipOver::update( float elapsed ){
	mImpulse *= 1.0f - elapsed;

	mRotation += Vec3f( ( mImpulse.y * elapsed ) * 90.0f, ( mImpulse.x * elapsed ) * 90.0f, 0.0f );

	while ( mRotation.x < -180.0f ) mRotation.x += 360.0f;
	while ( mRotation.x >  180.0f ) mRotation.x -= 360.0f;
	while ( mRotation.y < -180.0f ) mRotation.y += 360.0f;
	while ( mRotation.y >  180.0f ) mRotation.y -= 360.0f;

	if( mRotation.y <= 0.0f && mRotation.y > -90.0f ){
		mImpulse.x += ( 0.0f - mRotation.y ) * elapsed * 0.2f;
	}else if( mRotation.y <= -90.0 && mRotation.y > -180.0f ){
		mImpulse.x += ( -180.0f - mRotation.y ) * elapsed * 0.2f;
	}else if( mRotation.y <= -180.0f && mRotation.y > -270.0f ){
		mImpulse.x += ( -180.0f - mRotation.y ) * elapsed * 0.2f;
	}else if( mRotation.y <= -270.0f && mRotation.y > -360.0f ){
		mImpulse.x += ( -360.0f - mRotation.y ) * elapsed * 0.2f;
	}

	if( mRotation.y >= 0.0f && mRotation.y <= 90.0f ){
		mImpulse.x += ( 0.0f - mRotation.y ) * elapsed * 0.2f;
	}else if( mRotation.y >= 90.0f && mRotation.y < 180.0f ){
		mImpulse.x += ( 180.0f - mRotation.y ) * elapsed * 0.2f;
	}else if( mRotation.y >= 180.0f && mRotation.y < 270.0f ){
		mImpulse.x += ( 180.0f - mRotation.y ) * elapsed * 0.2f;
	}else if( mRotation.y >= 270.0f && mRotation.y < 360.0f ){
		mImpulse.x += ( 360.0f - mRotation.y ) * elapsed * 0.2f;
	}


	if( mRotation.x < 0.0f && mRotation.x >= -180.0f ){
		mImpulse.y += ( 0.0f - mRotation.x ) * elapsed * 0.2f;
	}else if( mRotation.y <= -180.0f && mRotation.x < -360.0f ){
		mImpulse.y += ( -360.0f - mRotation.x ) * elapsed * 0.2f;
	}else if( mRotation.x > 0.0f && mRotation.x <= 180.0f ){
		mImpulse.y +=  ( 0.0f - mRotation.x ) * elapsed * 0.2f;
	}else if( mRotation.x >= 180.0f && mRotation.x < 360.0f ){
		mImpulse.y += ( 360.0f - mRotation.x ) * elapsed * 0.2f;
	}
}

void FlipOver::applyImpulse( ci::Vec2i impulse ){
	impulse.y *= -1;
	mImpulse += (ci::Vec2f) impulse;
	mImpulse = Vec2f( ci::math<float>::clamp( mImpulse.x, -5.0f, 5.0f ), ci::math<float>::clamp( mImpulse.y, -5.0f, 5.0f ) );
}