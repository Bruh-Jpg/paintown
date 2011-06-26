#ifndef _paintown_player_common_h
#define _paintown_player_common_h

#include "character.h"
#include "util/load_exception.h"

namespace Storage{
    class AbsolutePath;
}

namespace Paintown{

class PlayerCommon: public Character {
public:
    PlayerCommon(const Character& chr);
    PlayerCommon(const char * filename);
    PlayerCommon(const Filesystem::AbsolutePath & str);
    PlayerCommon(const PlayerCommon & pl);
    virtual ~PlayerCommon();

    virtual void increaseScore(int much);

    virtual inline unsigned int getScore() const {
        return score;
    }

    virtual inline void setScore(unsigned int score){
        this->score = score;
    }
	
private:
    unsigned int score;

};

}

#endif
