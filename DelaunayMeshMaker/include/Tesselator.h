#include "cinder/TriMesh.h"
#include "cinder/gl/Vbo.h"
#include "cinder/ImageIo.h"
#include "cinder/Rand.h"
#include "CinderOpenCV.h"

#include "sweep/cdt.h"
#include "poly2tri.h"

#include "boost/thread.hpp"
#include "boost/signals2.hpp"

typedef boost::shared_ptr< p2t::CDT>		CDTRef;
typedef boost::shared_ptr<class Tesselator> TesselatorRef;

class Tesselator{
public:
	static TesselatorRef makeTesselator(){
		return TesselatorRef( new Tesselator() );
	};

	~Tesselator();

	ci::TriMesh getMesh();
	void update();

	// 
	std::vector<float > getDepthRandom(); 

	// shared pointer of thread
	std::shared_ptr<boost::thread>	mThread;

	void process( 
		const std::vector<ci::Vec2f > & borderPoints, 
		const std::vector<ci::Vec2f > & steinerPoints, 
		bool blend, 
		ci::Surface32f surface, 
		float depthOffset );

	// signal dispatched when thread is done
	boost::signals2::signal<void()>	sDone;

	// indicates if thread is running
	inline bool isRunning(){ return mRunningPrev; };
	void interrupt();
protected:
	Tesselator();

	// run is threaded function
	void run();

	// helper function
	void appendVertex( p2t::Point * p, ci::Color & color );

	// data needed by thread
	ci::Surface32f			mSurfaceCopy;
	std::vector<ci::Vec2f > mSteinerPointsCopy;
	std::vector<ci::Vec2f > mBorderPointsCopy;

	// mesh shared resource between thread, stores mesh calculated by tesselator
	boost::mutex	mMeshMutex;
	ci::TriMesh		mMesh;

	// random depth values of each vertex
	boost::mutex		mDepthRandomMutex;
	std::vector<float > mDepthRandom; 
	float				mDepthOffset;

	// bools to indicate thread activity 
	bool	mRunning;
	bool	mRunningPrev;

	// blend determines colors used by each vertex
	bool	mBlend;
};