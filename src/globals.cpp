#include "globals.h"
#include <streambuf>
#include "util/funcs.h"
#include "util/bitmap.h"
#include <iostream>

using namespace std;

/*
const int MIN_WORLD_Z = 160;
const int MAX_WORLD_Z = 232;
*/

const int ALLIANCE_NONE = 0;
const int ALLIANCE_PLAYER = 1;
const int ALLIANCE_ENEMY = 2;

const int MIN_RELATIVE_DISTANCE = 10;

const char * NAME_FONT = "CENTURYGOTHIC_PCX";

// static bool global_debug = false;
static int global_debug_level = 0;

const char * Global::DEFAULT_FONT = "/fonts/arial.ttf";

class nullstreambuf_t: public std::streambuf {
public:
	nullstreambuf_t():std::streambuf(){
	}
};

static nullstreambuf_t nullstreambuf;

class nullcout_t: public std::ostream {
public:
	nullcout_t():std::ostream(&nullstreambuf){
	}
};

static nullcout_t nullcout;

ostream & Global::debug( int i ){
	if ( global_debug_level >= i ){
		return std::cout;
	}
	return nullcout;
}

void Global::setDebug( int i ){
	global_debug_level = i;
}

const int Global::getDebug(){
	return global_debug_level;
}

void Global::showTitleScreen(){
	Bitmap::Screen->Blit( Global::titleScreen() );
}

const std::string Global::titleScreen(){
	return Util::getDataPath() + "/menu/paintown.png";
}
