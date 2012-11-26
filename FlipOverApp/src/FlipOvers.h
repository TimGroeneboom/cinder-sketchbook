#include "FlipOver.h"
#include "cinder/app/AppBasic.h"

typedef boost::shared_ptr<class FlipOvers> FlipOversRef;
class FlipOvers{
public:
	static FlipOversRef createFlipOvers( ci::Vec2i flipOverNum, ci::Vec2f flipOverSize ){
		return FlipOversRef( new FlipOvers( flipOverNum, flipOverSize ) );
	}

	void draw();
	void drawColored();
	void update( float elapsed );

	void mouseDown( ci::app::MouseEvent event );
	void mouseDrag( ci::app::MouseEvent event );

	inline void enableYFlip( bool val = true ){ mEnableYFlip = val; };
	inline bool getYFlipEnabled(){ return mEnableYFlip; };
protected:
	FlipOvers( ci::Vec2i flipOverNum, ci::Vec2f flipOverSize );

	ci::Vec2f					mFlipOverSize;
	ci::Vec2i					mFlipOverNum;

	std::vector< FlipOverRef>	mFlipOvers;
	ci::gl::GlslProg			mShader;

	ci::Vec2i					mMousePos;
	
	bool						mEnableYFlip;
};