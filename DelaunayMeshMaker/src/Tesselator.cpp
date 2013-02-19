#include "Tesselator.h"
#include "cinder/app/AppBasic.h"

using namespace ci;
using namespace std;

Tesselator::Tesselator(){
	mRunning		= false;
	mRunningPrev	= false;
}

Tesselator::~Tesselator(){
	if( mThread )
		mThread->join();
}

void Tesselator::interrupt(){
	if( mThread && !mThread->interruption_requested() )
		mThread->interrupt();
}

void Tesselator::process( const std::vector<Vec2f > & borderPoints, const std::vector<Vec2f > & steinerPoints, bool blend, Surface32f surface, float depthOffset ){
	if( mRunning )
		return;

	mRunning		= true;
	mRunningPrev	= mRunning;
	mBlend			= blend;

	mSurfaceCopy		= surface.clone(true);
	mSteinerPointsCopy	= steinerPoints;
	mBorderPointsCopy	= borderPoints;

	mDepthOffset		= depthOffset;

	mThread				= std::shared_ptr<boost::thread>( new boost::thread( boost::bind( &Tesselator::run, this ) ) );
}

std::vector<float > Tesselator::getDepthRandom(){
	boost::mutex::scoped_lock l(mDepthRandomMutex);

	return mDepthRandom;
}

TriMesh Tesselator::getMesh(){
	boost::mutex::scoped_lock l( mMeshMutex );

	return mMesh;
}

void Tesselator::update(){
	// check if thread is done, if so, dispatch signal
	if( !mRunning && mRunningPrev ){
		mRunningPrev = mRunning;
		sDone();
	}
}

void Tesselator::run(){
	boost::mutex::scoped_lock l1(mMeshMutex);
	boost::mutex::scoped_lock l2(mDepthRandomMutex);

	mMesh.clear();
	mDepthRandom.clear();
		
	// clear previous mesh
	mMesh.clear();

	// get the 4 outline points of the screen 
	std::vector<p2t::Point*> p2tBorderPoints;
	for( auto itr = mBorderPointsCopy.begin(); itr != mBorderPointsCopy.end(); ++itr ){
		p2tBorderPoints.push_back( new p2t::Point( (*itr).x, (*itr).y ) );
	}

	// make cdt out of border points
	p2t::CDT * cdt = new p2t::CDT( p2tBorderPoints );

	// add steinerPoints within screen
	std::vector<p2t::Point*> p2tSteinerPoints;
	for( auto itr = mSteinerPointsCopy.begin(); itr != mSteinerPointsCopy.end(); itr++ ){
		p2tSteinerPoints.push_back( new p2t::Point( (*itr).x, (*itr).y ) );
		cdt->AddPoint( p2tSteinerPoints.back()  );
	}

	// succeeded, check if thread was interrupted
	try { boost::this_thread::interruption_point(); }
	catch( boost::thread_interrupted ) { 
		mRunning		= false;
		mRunningPrev	= false;
		return; 
	}

	// this is where the magic happens
	cdt->Triangulate();

	// succeeded, check if thread was interrupted
	try { boost::this_thread::interruption_point(); }
	catch( boost::thread_interrupted ) { 
		mRunning		= false;
		mRunningPrev	= false;
		return; 
	}

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
			color1 = mSurfaceCopy.getPixel( Vec2i( (int)p1->x, (int)p1->y ) );
			color2 = mSurfaceCopy.getPixel( Vec2i( (int)p2->x, (int)p2->y ) );
			color3 = mSurfaceCopy.getPixel( Vec2i( (int)p3->x, (int)p3->y ) );
		}else{
			Vec2i center =	( Vec2i( (int)p1->x, (int)p1->y ) / 3.0f ) +
							( Vec2i( (int)p2->x, (int)p2->y ) / 3.0f ) +
							( Vec2i( (int)p3->x, (int)p3->y ) / 3.0f );

			color1 = mSurfaceCopy.getPixel( center );
			color2 = color1;
			color3 = color2;
		}

		appendVertex( p1, color1 );
		appendVertex( p2, color2 );
		appendVertex( p3, color3 );

		mMesh.appendTriangle( mMesh.getVertices().size() - 3, mMesh.getVertices().size() - 2, mMesh.getVertices().size() - 1 );
	}

	// succeeded, check if thread was interrupted
	try { boost::this_thread::interruption_point(); }
	catch( boost::thread_interrupted ) { 
		mRunning		= false;
		mRunningPrev	= false;
		return; 
	}

	std::vector<Vec3f> verts = mMesh.getVertices();
	std::vector<Vec3f> newVerts;
	for( auto itr = verts.begin(); itr != verts.end(); ++itr ){
		float z = (*itr).z;

		newVerts.push_back( Vec3f( (*itr).x, (*itr).y, (*itr).z * mDepthOffset ) );
	}
	mMesh.getVertices().swap( newVerts );
	mMesh.recalculateNormals();

	// succeeded, check if thread was interrupted
	try { boost::this_thread::interruption_point(); }
	catch( boost::thread_interrupted ) { 
		mRunning		= false;
		mRunningPrev	= false;
		return; 
	}

	// delete allocated memory
	for( size_t i = 0 ; i < p2tBorderPoints.size(); i++ ){
		delete p2tBorderPoints[i];
	}
	for( size_t i = 0 ; i < p2tSteinerPoints.size(); i++ ){
		delete p2tSteinerPoints[i];
	}
	delete cdt;

	// done!
	mRunning = false;
}

void Tesselator::appendVertex( p2t::Point * p, Color & color )
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