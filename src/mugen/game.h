#ifndef paintown_mugen_game_h
#define paintown_mugen_game_h

#include <string>

#include "util.h"

class Bitmap;
class MugenStage;

namespace Mugen{

class CharacterSelect;
    
/* Our game definition, this is to facilitate running a game */
class Game {
    public:
	Game(const PlayerType &, const GameType &, const std::string & systemFile);
	virtual ~Game();
	
	//! Runs everything
	virtual void run();
    private:
	void doArcade(const Bitmap &);
        void doTraining(const Bitmap & bmp);
	void doVersus(const Bitmap &);
	void doWatch(const Bitmap &);
	
	//! Player type
	PlayerType playerType;
	//! Game type
	GameType gameType;
	//! System file
	std::string systemFile;
	//! Motif Base Directory
	std::string motifDirectory;
};

}


#endif

