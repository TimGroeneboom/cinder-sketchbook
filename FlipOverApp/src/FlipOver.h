#pragma once

#include "cinder/Cinder.h"
#include "cinder/Vector.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Vbo.h"

typedef boost::shared_ptr<class FlipOver> FlipOverRef;
class FlipOver
{

public:
	static FlipOverRef createFlipOver( ci::Vec2f position, ci::Vec2f size ){
		return FlipOverRef( new FlipOver( position, size ) );
	}
protected:
	FlipOver( ci::Vec2f position, ci::Vec2f size );

	ci::Vec2f mPosition;
	ci::Vec2f mSize;
	ci::Vec3f mRotation;
	ci::Vec2f mImpulse;

	static ci::gl::VboMesh	mMesh;
	static ci::gl::GlslProg	mColorPickShader;
private:
	// can be called from Papers
	void draw( ci::gl::GlslProg shader );
	void update( float elapsed );
	void applyImpulse( ci::Vec2i impulse );
	void drawColored();

	friend class FlipOvers;

	float	mColor;
};