#ifndef mugen_animation_h
#define mugen_animation_h

#include <string>
#include <vector>

class MugenSprite;

/*
Collision Area
*/
class MugenArea{
public:
    MugenArea();
    MugenArea( const MugenArea &copy );
    ~MugenArea();
    int x1,y1,x2,y2;
};

/*
Frame
*/
class MugenFrame{
    public:
	MugenFrame();
	MugenFrame( const MugenFrame &copy );
	virtual ~MugenFrame();

	// We'll keep them, but they probably won't be used
	std::vector< MugenArea > defenseCollision;
	// This is the only one will be worried about
	std::vector< MugenArea > attackCollision;
	// Is this frame a loopstart position?
	bool loopstart;
	// This is the sprite 
	MugenSprite *sprite;
	// Additional Offsets which are in character.air (ie x + sprite.x)
	int xoffset;
	int yoffset;
	// Time in ticks to display -1 for infinite
	int time;
	// Flip horizontal?
	bool flipHorizontal;
	// Flip Vertical
	bool flipVertical;
	/*Color addition (need to decipher their shorthand crap like A1)
	15,4, 0,0, 5, ,A   ;<-- Color addition (flip parameter omitted)
	15,4, 0,0, 5, H, S ;<-- Flips horizontally and does color subtraction

	If you wish to specify alpha values for color addition, use the
	parameter format "AS???D???", where ??? represents the values of the
	source and destination alpha respectively. Values range from 0 (low)
	to 256 (high). For example, "AS64D192" stands for "Add Source_64 to
	Dest_192". Also, "AS256D256" is equivalent to just "A". A shorthand
	for "AS256D128" is "A1".

	15,4, 0,0, 5, ,A1  ;<-- Color addition to 50% darkened dest
	15,4, 0,0, 5, ,AS128D128 ;<-- Mix 50% source with 50% dest
	*/
	std::string colorAdd;
};

/*
 * Holds mugen animations, ie: player.air
 */
class MugenAnimation{
    public:
	MugenAnimation();
	MugenAnimation( const MugenAnimation &copy );
	~MugenAnimation();
	
	// Get next Frame
	const MugenFrame *getNext();
	
	// Reset
	inline void reset(){ position = 0; }
	
	// Add a frame
	void addFrame( MugenFrame * );

        inline const std::vector<MugenFrame*> & getFrames() const {
            return frames;
        }
	
    private:
	
	std::vector< MugenFrame * > frames;
	
	unsigned int loopPosition;
	unsigned int position;
};

#endif
