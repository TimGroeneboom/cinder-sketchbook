#include "FlipOvers.h"

using namespace ci;

#define MAX_RADIUS Vec2f( 100.0f, 100.0f )

FlipOvers::FlipOvers( Vec2i flipOverNum, Vec2f  flipOverSize ){
	mFlipOverNum	= flipOverNum;
	mFlipOverSize	= flipOverSize;
	mEnableYFlip	= false;

	mShader = gl::GlslProg( app::loadAsset("phong_vert.glsl"), app::loadAsset("phong_frag.glsl") );

	for( int x = 0; x < mFlipOverNum.x; x++ ){
		for( int y = 0; y < mFlipOverNum.y; y++ ){
			FlipOverRef flipOver = FlipOver::createFlipOver( Vec2f( (float)x, (float)y ) * mFlipOverSize, mFlipOverSize );
			flipOver->mColor =  ( ( x * y )  )	/  ( (float)( mFlipOverNum.x * mFlipOverNum.y ) ) * 0.5f;
			mFlipOvers.push_back( flipOver );
		}
	}
}

void FlipOvers::update( float elapsed ){
	for( auto itr = mFlipOvers.begin(); itr != mFlipOvers.end(); itr++ )
		(*itr)->update( elapsed );
}

void FlipOvers::mouseDrag( ci::app::MouseEvent event ){
	ci::Vec2i mDiff = event.getPos() - mMousePos;
	mMousePos = event.getPos();

	for( auto itr = mFlipOvers.begin(); itr != mFlipOvers.end(); itr++ ){
		float dist = ( mMousePos - (*itr)->mPosition ).length();
		if( dist < MAX_RADIUS.length() ){
			if( mEnableYFlip )
				(*itr)->applyImpulse( mDiff );
			else
				(*itr)->applyImpulse( Vec2f( (float)mDiff.x, 0.0f ) );
		}
	}
}

void FlipOvers::mouseDown( ci::app::MouseEvent event ){
	mMousePos = event.getPos();
}

void FlipOvers::draw(){
	mShader.bind();
	mShader.uniform( "size", Vec2f( 1.0f, 1.0f ) / mFlipOverSize );
	mShader.uniform( "scale", Vec2f( 1.0f, 1.0f ) / mFlipOverNum );

	gl::translate( Vec3f( mFlipOverSize / 2, 0 )  );

	for( auto itr = mFlipOvers.begin(); itr != mFlipOvers.end(); itr++ ){
		mShader.uniform( "offset", (*itr)->mPosition / ( mFlipOverSize * (Vec2f)mFlipOverNum ) );
		(*itr)->draw( mShader );
	}
	
	mShader.unbind();
}

void FlipOvers::drawColored(){
	gl::translate( Vec3f( mFlipOverSize / 2, 0 )  );

	for( auto itr = mFlipOvers.begin(); itr != mFlipOvers.end(); itr++ ){
		//mShader.uniform( "offset", (*itr)->mPosition / ( mFlipOverSize * (Vec2f)mFlipOverNum ) );
		(*itr)->drawColored();
	}
}