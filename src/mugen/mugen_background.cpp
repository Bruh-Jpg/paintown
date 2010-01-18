#include "util/bitmap.h"
#include "mugen_background.h"
#include <math.h>
#include <ostream>
#include <cstring>
#include <string>
#include <algorithm>
#include "globals.h"
#include "mugen_sprite.h"
#include "mugen_util.h"
#include "mugen_section.h"
#include "mugen_item_content.h"
#include "mugen_item.h"
#include "util/regex.h"
#include "ast/all.h"

//static double pi = 3.14159265;

static const int CONTROLLER_VALUE_NOT_SET = -999999;
static const int DEFAULT_BACKGROUND_ID = -9999;
static const char * DEBUG_CONTEXT = __FILE__;

using namespace std;

static double interpolate(double f1, double f2, double p){
    return (f1 * (1.0 - p)) + (f2 * p);
}

struct Tile {
    int start;
    int total;
};

static Tile getTileData( int location, int length, int spacing, int total ){
    Tile tile;
    if (total == 0){
	tile.start = location;
	tile.total = 1;
	return tile;
    } else if (total > 1){
	tile.start = location;
	tile.total = total;
	return tile;
    } else if (total == 1){
	// Infinite tiling.. just tile on the board
	if (location < 0){
	    // Less than the board itself lets get location up to that point
	    while (location < 0){
		location+=spacing;
	    }
	    // Now backup 1 so we get the wrap effect 
	    location-=spacing;
	} else{
	    // Either Larger than the board or inside seek back to beginning
	    while (location > 0){
		location-=spacing;
	    }
	}
	// Now figure out how many we need to do
	int temp = location;
	// Reuse total
	total = 0;
	while (temp < length){
	    total++;
	    temp+=spacing;
	}
	// Blammo
	tile.start = location;
	tile.total = total;
	return tile;
    }
    tile.start = 0;
    tile.total = 0;
    return tile;
}

static void doParallax(Bitmap &bmp, Bitmap &work, int leftx, int lefty, int xoffset, double top, double bot, int yscalestart, double yscaledelta, double yoffset, bool mask){
    const int height = bmp.getHeight();
    const int width = bmp.getWidth();
    //double z = 1.0 / z1;
    //const double z_add = ((1.0 / z2) - z) / (y2 - y1);


    Global::debug(3) << "background leftx " << leftx << endl;

    for (int localy = 0; localy < height; ++localy ){
        int movex = 0;
	//int width = bmp.getWidth()*z;
        const double range = (double)localy / (double)height;
	const double scale = interpolate(top, bot, range) - top;
	//const double newHeight = height*((yscalestart+(yoffset*yscaledelta))/100);
	//const double yscale = (newHeight/height);
	movex = (int)(leftx + (leftx - xoffset) * scale);
	// bmp.Stretch(work, 0, localy, w, 1, movex, lefty+localy, w, 1);
        bmp.Blit(0, localy, width, 1, movex, lefty + localy, work);
	//z +=  z_add;
	//Global::debug(1) << "Height: " << height << " | yscalestart: " << yscalestart << " | yscaledelta: " << yscaledelta << " | yoffset: " << yoffset << " | New Height: " << newHeight << " | yscale: " << yscale << endl;	
    }
}

// mugen background
MugenBackground::MugenBackground( const unsigned long int &ticker ):
type(Normal),
groupNumber(-1),
imageNumber(-1),
actionno(-1),
id(0),
layerno(0),
startx(0),
starty(0),
deltax(1),
deltay(1),
mask(false),
tilex(0),
tiley(0),
tilespacingx(0),
tilespacingy(0),
windowdeltax(0),
windowdeltay(0),
xscaletop(0),
xscalebot(0),
yscalestart(100),
yscaledelta(100),
positionlink(false),
velocityx(0),
velocityy(0),
sinx_amp(0),
sinx_period(0),
sinx_offset(0),
sinx_angle(0),
siny_amp(0),
siny_period(0),
siny_offset(0),
siny_angle(0),
xoffset(0),
yoffset(0),
movex(0),
movey(0),
velx(0),
vely(0),
stageTicker( ticker ),
x(0),
y(0),
visible(true),
enabled(true),
controller_offsetx(0),
controller_offsety(0),
sprite(0),
action(0),
linked(0),
runLink(false){
    // Set window to default
    window.x1 = 0;
    window.y1 = 0;
    window.x2 = 319;
    window.y2 = 239;
}
MugenBackground::MugenBackground( const MugenBackground &copy ):
stageTicker( copy.stageTicker ){
}
MugenBackground::~MugenBackground(){
}
MugenBackground & MugenBackground::operator=( const MugenBackground &copy ){
    
    return *this;
}
    
void MugenBackground::logic( const double x, const double y, const double placementx, const double placementy ){
    if (enabled){
	movex = movey = 0;
	movex += x * deltax;
	movey += y * deltay;
	velx += velocityx;
	vely += velocityy;
	/* how much should sin_angle be incremented by each frame?
	* I think the total angle should be (ticks % sin_period) * 2pi / sin_period
	* M (decimal) is the magnitude in pixels (amp)
	* P (decimal) is the period in game ticks (period) 
	* O (decimal) is the time offset in game ticks (offset)
	* From updates.txt:  M * sine ((ticks+O)/P * 2 * Pi)
	* sinx_amp * sin((stageTicker+sinx_offset)/sinx_period * 2 * pi)) ? useless it seems
	*/
	//sin_angle += 0.00005;
	sinx_angle += 0.00005;
	siny_angle += 0.00005;
	
	if (type == Anim){
            action->logic();
        }
	
	this->x = (int)(placementx + xoffset + movex + velx + controller_offsetx + sinx_amp * sin(sinx_angle*sinx_period + sinx_offset));
	this->y = (int)(placementy + yoffset + movey + vely + controller_offsety + siny_amp * sin(siny_angle*siny_period + siny_offset));
	// this->x = (int)(xoffset + movex + velx + controller_offsetx + sinx_amp * sin(sinx_angle*sinx_period + sinx_offset));
	// this->y = (int)(yoffset + movey + vely + controller_offsety + siny_amp * sin(siny_angle*siny_period + siny_offset));
    }
}
    
void MugenBackground::render( const double windowx, const double windowy, const int totalLength, const int totalHeight, Bitmap *work ){
    if (visible){
	// Set clipping rect
	work->setClipRect(int(windowx + window.x1), int(windowy + window.y1),int(windowx + window.x2),int(windowy + window.y2));
	switch (type){
	    case Normal:{
		// Normal is a sprite
		// Tile it
		const int addw = sprite->getWidth() + tilespacingx;
		const int addh = sprite->getHeight() + tilespacingy;
		Tile tilev = getTileData(y, totalHeight, addh, tiley);
		for (int v = 0; v < tilev.total; ++v){
		    Tile tileh = getTileData(x, totalLength, addw, tilex);
		    for (int h = 0; h < tileh.total; ++h){
			draw( tileh.start, tilev.start, *work);
			tileh.start+=addw;
		    }
		    tilev.start+=addh;
		}
		break;
	    }
	    case Parallax:{
		// This is also a sprite but we must parallax it across the top and bottom to give the illusion of depth
                // Global::debug(0) << "Background at " << x << ", " << y << endl;
		const int addw = sprite->getWidth() + tilespacingx;
		const int addh = sprite->getHeight() + tilespacingy;
		Tile tilev = getTileData(y, totalHeight, addh, tiley);
		for (int v = 0; v < tilev.total; ++v){
		    Tile tileh = getTileData(x, totalLength, addw, tilex);
		    for (int h = 0; h < tileh.total; ++h){
			doParallax( *sprite->getBitmap(), *work, tileh.start, tilev.start, xoffset+((totalLength)/2), xscaletop, xscalebot, yscalestart, yscaledelta, (movey-deltay), mask);
			tileh.start += addw;
		    }
		    tilev.start += addh;
		}
		break;
	    }
	    case Anim:{
		// there is no sprite use our action!
		// Tiling action
		const int addw = tilespacingx;
		const int addh = tilespacingy;
		Tile tilev = getTileData(y, totalHeight, addh, tiley);
		for (int v = 0; v < tilev.total; ++v){
		    Tile tileh = getTileData(x, totalLength, addw, tilex);
		    for (int h = 0; h < tileh.total; ++h){
			action->render( tileh.start, tilev.start, *work);
			tileh.start+=addw;
		    }
		    tilev.start+=addh;
		}
		break;
	    }
	    case Dummy:
		// Do nothing
	    default:
		break;
	}
	// Reset clip
	work->setClipRect(0, 0,work->getWidth(),work->getHeight());
    }
}

void MugenBackground::setAnimation( MugenAnimation *animation){
    if( actionno != -1 ){
	action = animation;
	// Check tilespacing and mask
	if (tilespacingx == 0){
	    tilespacingx = 1;
	}
	if (tilespacingy == 0){
	    tilespacingy = 1;
	}
	mask = true;
    }
}

void MugenBackground::preload( const int xaxis, const int yaxis ){
    // Do positionlink crap
    if (positionlink && !runLink){
	if (linked){
	    linked->setPositionLink(this);
	}
	runLink = true;
    }
    
    // Set our initial offsets
    xoffset = (xaxis) + startx;
    yoffset = (yaxis) + starty;
    velx = vely = 0;
}

void MugenBackground::draw( const int ourx, const int oury, Bitmap &work ){
    sprite->render(ourx,oury,work,effects);
}

// Lets do our positionlink stuff
void MugenBackground::setPositionLink(MugenBackground *bg){
    if (positionlink){
	if (linked){
	    linked->setPositionLink(bg);
	    return;
	}
    }
    bg->startx += startx;
    bg->starty += starty;
    bg->deltax = deltax;
    bg->deltay = deltay;
    bg->sinx_amp = sinx_amp;
    bg->sinx_offset = sinx_offset;
    bg->sinx_period = sinx_period;
    bg->siny_amp = siny_amp;
    bg->siny_offset = siny_offset;
    bg->siny_period = siny_period;
    bg->velocityx = velocityx;
    bg->velocityy = velocityy;
    
    //Global::debug(1) << "Positionlinked bg: " << bg->name << " set to x: " << bg->startx << " y: " << bg->starty << endl;
}

BackgroundController::BackgroundController():
name(""),
type(Ctrl_Null),
timestart(0),
endtime(0),
looptime(-1),
ownticker(0),
value1(CONTROLLER_VALUE_NOT_SET),
value2(CONTROLLER_VALUE_NOT_SET),
value3(CONTROLLER_VALUE_NOT_SET){
}
BackgroundController::~BackgroundController(){
}

void BackgroundController::act(const std::map< int, MugenAnimation * > &animations){
    Global::debug(1, DEBUG_CONTEXT) << "Control Name: " << name << "Control type: " << type << " is running." << endl;
    Global::debug(1, DEBUG_CONTEXT) << "ticker: " << ownticker << " Start time: " << timestart << " End Time: " << endtime << endl;
    // Do we run this?
    if( ownticker >= timestart && ownticker <= endtime ){
	Global::debug(1) << "We have action, total backgrounds: " << backgrounds.size() << endl;
	for (std::vector<MugenBackground *>::iterator i = backgrounds.begin(); i != backgrounds.end(); ++i){
	    MugenBackground *background = *i;
	    Global::debug(1) << "Acting on background: " << background->getName() << " | Type: " << type << endl;
	    switch (type){
		case Ctrl_Visible:
		    if (value1 != CONTROLLER_VALUE_NOT_SET){
			background->setVisible(value1);
		    }
		    break;
		case Ctrl_Enabled:
		    if (value1 != CONTROLLER_VALUE_NOT_SET){
			background->setEnabled(value1);
		    }
		    break;
		case Ctrl_VelSet:
		    if (value1 != CONTROLLER_VALUE_NOT_SET){
			background->velocityx = value1;
			Global::debug(1) << "	Set X velocity to: " << value1 << endl;
		    }
		    if (value2 != CONTROLLER_VALUE_NOT_SET){
			background->velocityy = value2;
			Global::debug(1) << "	Set Y velocity to: " << value2 << endl;
		    }
		    break;
		case Ctrl_VelAdd:
		    if (value1 != CONTROLLER_VALUE_NOT_SET){
			background->velocityx += value1;
		    }
		    if (value2 != CONTROLLER_VALUE_NOT_SET){
			background->velocityy += value2;
		    }
		    break;
		case Ctrl_PosSet:
		    if (value1 != CONTROLLER_VALUE_NOT_SET){
			background->controller_offsetx = value1;
			Global::debug(1) << "	Set X position to: " << value1 << endl;
		    }
		    if (value2 != CONTROLLER_VALUE_NOT_SET){
			background->controller_offsety = value2;
			Global::debug(1) << "	Set Y position to: " << value2 << endl;
		    }
		    break;
		case Ctrl_PosAdd:
		    if (value1 != CONTROLLER_VALUE_NOT_SET){
			background->controller_offsetx += value1;
			Global::debug(1) << "	Add to Position X: " << value1 << endl;
		    }
		    if (value2 != CONTROLLER_VALUE_NOT_SET){
			background->controller_offsety += value2;
			Global::debug(1) << "	Add to Position Y: " << value2 << endl;
		    }
		    break;
		case Ctrl_Animation:{
			std::map< int, MugenAnimation * >::const_iterator iter = animations.find((int)value1);
			if (iter != animations.end()){
			    background->action = iter->second;
			}
		    }
		    break;
		case Ctrl_Sinx:
		    if (value1 != CONTROLLER_VALUE_NOT_SET){
			background->sinx_amp = value1;
		    }
		    if (value2 != CONTROLLER_VALUE_NOT_SET){
			background->sinx_offset = value2;
		    }
		    if (value3 != CONTROLLER_VALUE_NOT_SET){
			background->sinx_period = value3;
		    }
		    break;
		case Ctrl_Siny:
		    if (value1 != CONTROLLER_VALUE_NOT_SET){
			background->siny_amp = value1;
		    }
		    if (value2 != CONTROLLER_VALUE_NOT_SET){
			background->siny_offset = value2;
		    }
		    if (value3 != CONTROLLER_VALUE_NOT_SET){
			background->siny_period = value3;
		    }
		    break;
		case Ctrl_Null:
		default:
		    break;
	    }
	    Global::debug(1) << "Background X: " << background->x << endl;
	}
    }
    ownticker++;
    // Shall we reset?
    if( (looptime != -1) && (ownticker > endtime) ){
	ownticker=0;
    }
}

void BackgroundController::reset(){
    if( looptime == -1){
	// I'm not totally convinced we should be doing this, but it seems to work otherwise....
	//ownticker = 0;
    }
}

/* our controller handler */
MugenBackgroundController::MugenBackgroundController(const std::string &n):
name(n),
id(DEFAULT_BACKGROUND_ID),
looptime(-1),
ticker(0){
}
MugenBackgroundController::~MugenBackgroundController(){
    // Kill all controllers initiated by the load
    for (std::vector<BackgroundController *>::iterator i = controls.begin(); i != controls.end(); ++i){
	    if(*i)delete *i;
    }
}
void MugenBackgroundController::addControl( BackgroundController *ctrl ){
    controls.push_back(ctrl);
}
void MugenBackgroundController::act(const std::map< int, MugenAnimation * > &animations){
    // Lets act out our controllers
    Global::debug(1) << "Controller Def: " << name << " | Total controls: " << controls.size() << endl;
    for (std::vector<BackgroundController *>::iterator i = controls.begin(); i != controls.end(); ++i){
	    BackgroundController *ctrl = *i;
	    Global::debug(1) << "Acting on Controller: " << ctrl->name << " | timestart: " << ctrl->timestart << " | endtime: " << ctrl->endtime << " | looptime" << ctrl->looptime << " | ticker: " << ctrl->ownticker << endl;
	    ctrl->act(animations);
    }
    ticker++;
    if( (looptime != -1) && (ticker > looptime) ){
	// Reset itself and everybody that needs reseting
	ticker = 0;
	for (std::vector<BackgroundController *>::iterator i = controls.begin(); i != controls.end(); ++i){
	    BackgroundController *ctrl = *i;
	    ctrl->reset();
	}
    }
}

static bool matchRegex(const string & str, const string & regex){
    return Util::matchRegex(str, regex);
}

MugenBackgroundManager::MugenBackgroundManager(const std::string &baseDir, const vector<Ast::Section *> & section, const unsigned long int &ticker, std::map< unsigned int, std::map< unsigned int, MugenSprite * > > *sprites, const string & baseName):
name(""),
debugbg(false),
clearColor(-1),
spriteFile(""){

    // for linked position in backgrounds
    MugenBackground *prior = 0;
    
    for (vector<Ast::Section*>::const_iterator section_it = section.begin(); section_it != section.end(); section_it++){
        string head = (*section_it)->getName();
	head = Mugen::Util::fixCase(head);
	Global::debug(1) <<  "Header: " << head << " | Extracted name: " << name << endl;
	if (matchRegex(head, ".*" + baseName + "def.*")){
            Ast::Section * section = *section_it;
            for (list<Ast::Attribute*>::const_iterator attribute_it = section->getAttributes().begin(); attribute_it != section->getAttributes().end(); attribute_it++){
                Ast::Attribute * attribute = *attribute_it;
                if (attribute->getKind() == Ast::Attribute::Simple){
                    Ast::AttributeSimple * simple = (Ast::AttributeSimple*) attribute;
                    if (*simple == "spr"){
			*simple >> spriteFile;
			Global::debug(1) << "Reading Sff (sprite) Data..." << endl;
			// Strip it of any directory it might have
			spriteFile = Mugen::Util::stripDir(spriteFile);
			Global::debug(1) << "Sprite File: " << spriteFile << endl;
			Mugen::Util::readSprites(Mugen::Util::getCorrectFileLocation(baseDir, spriteFile), "", this->sprites);
		    } else if (*simple == "debugbg"){
			*simple >> debugbg;
		    } else if (*simple == "bgclearcolor"){
			int r, g, b;
			*simple >> r >> g >> b;
			clearColor = Bitmap::makeColor(r,g,b);
		    } else {
                        throw MugenException("Unhandled option in Background Definition Section: " + simple->toString());
                    }
		}
            }
	// This our background data definitions
        /* probably need a better regex here */
	} else if (matchRegex(head, ".*" + baseName + " ")){
	    MugenBackground *temp;
	    if (!spriteFile.empty()){
		temp = Mugen::Util::getBackground(ticker, *section_it, this->sprites);
	    } else {
		temp = Mugen::Util::getBackground(ticker, *section_it, *sprites);
	    }
	    // Do some fixups and necessary things
	    // lets see where we lay
	    if (temp->layerno == 0){
                backgrounds.push_back(temp);
            } else if (temp->layerno == 1){
                foregrounds.push_back(temp);
            }
	    
	    // If position link lets set to previous item
	    if( temp->positionlink ){
		temp->linked = prior;
		Global::debug(1) << "Set positionlink to id: '" << prior->id << "' Position at x(" << prior->startx << ")y(" << prior->starty << ")" << endl;
	    } 
	    
	    // This is so we can have our positionlink info for the next item if true
	    prior = temp;
	} else if(matchRegex(head, "begin *action")){
            /* This creates the animations it differs from character animation since these are included in the stage.def file with the other defaults */
	    head.replace(0,13,"");
	    int h;
            istringstream out(head);
	    out >> h;
	    if (!spriteFile.empty()){
		animations[h] = Mugen::Util::getAnimation(*section_it, this->sprites);
	    } else {
		animations[h] = Mugen::Util::getAnimation(*section_it, *sprites);
	    }
	} else if (matchRegex(head, ".*bgctrldef")){
	    head.replace(0,10,"");
	    MugenBackgroundController *temp = new MugenBackgroundController(head);
	    Global::debug(1) << "Found background controller definition: " << temp->name << endl;
            class BackgroundControllerWalker: public Ast::Walker {
            public:
                BackgroundControllerWalker(MugenBackgroundManager & manager, MugenBackgroundController * controller, vector<MugenBackground*> & backgrounds, vector<MugenBackground*> & foregrounds):
                    hasId(false),
                    manager(manager),
                    controller(controller),
                    backgrounds(backgrounds),
                    foregrounds(foregrounds){
                }

                bool hasId;
                MugenBackgroundManager & manager;
                MugenBackgroundController * controller;
                vector<MugenBackground*> & backgrounds;
                vector<MugenBackground*> & foregrounds;

                virtual void onAttributeSimple(const Ast::AttributeSimple & simple){
                    if (simple == "eventid"){
		        simple >> controller->id;
                    } else if (simple == "looptime"){
                        simple >> controller->looptime;
                        if (controller->looptime == 0){
                            controller->looptime = -1;
                        }
                    } else if (simple == "ctrlid"){
                        hasId = true;
                        // Max 10
                        try{
                            while (true){
                                int id;
                                simple >> id;
                                manager.getBackgrounds(controller->backgrounds, id);
                            }
                        } catch (const Ast::Exception & e){
                        }
                    } else {
                        string name; // = controller->getName()
                        throw MugenException("Unhandled option in BGCtrlDef " + name + " Section: " + simple.toString());
                    }
                }

                virtual ~BackgroundControllerWalker(){
                    if (!hasId && controller->backgrounds.empty()){
                        controller->backgrounds.insert(controller->backgrounds.end(), backgrounds.begin(), backgrounds.end());
                        controller->backgrounds.insert(controller->backgrounds.end(), foregrounds.begin(), foregrounds.end());
                    }
                }
            };

            {
                BackgroundControllerWalker walker(*this, temp, backgrounds, foregrounds);
                Ast::Section * section = *section_it;
                section->walk(walker);
            }
	    Global::debug(1) << "Controlling total backgrounds: " << temp->backgrounds.size() << endl;
	    controllers.push_back(temp);
	} else if (matchRegex(head, ".*bgctrl")){
	    if (controllers.empty()){
		/* This is a hack to get mugen to do some fancy controlling in a regular
                 * game to accomplish stage fatalities and other tricks
                 */
		Global::debug(1) << "Found a BgCtrl without a parent definition... must be hackery!" << endl;
		continue;
	    }

	    // else we got ourselves some controls... under the last controller added
	    MugenBackgroundController *control = controllers.back();
	    head.replace(0,7,"");
	    BackgroundController *temp = new BackgroundController();
	    temp->name = head;

            class CtrlWalker: public Ast::Walker {
            public:
                CtrlWalker(BackgroundController * controller, MugenBackgroundController * control, MugenBackgroundManager & manager):
                    controller(controller),
                    control(control),
                    manager(manager){
                }

                BackgroundController * controller;
                MugenBackgroundController * control;
                MugenBackgroundManager & manager;
                
                virtual void onAttributeSimple(const Ast::AttributeSimple & simple){
                    if (simple == "type"){
                        string type;
                        simple >> type;
                        type = Mugen::Util::fixCase(type);
                        if (type == "Anim") controller->type = Ctrl_Animation;
                        else if( type == "enabled" ) controller->type = Ctrl_Enabled;
                        else if( type == "null" ) controller->type = Ctrl_Null;
                        else if( type == "posadd" ) controller->type = Ctrl_PosAdd;
                        else if( type == "posset" ) controller->type = Ctrl_PosSet;
                        else if( type == "sinx" ) controller->type = Ctrl_Sinx;
                        else if( type == "siny" ) controller->type = Ctrl_Siny;
                        else if( type == "veladd" ) controller->type = Ctrl_VelAdd;
                        else if( type == "velset" ) controller->type = Ctrl_VelSet;
                        else if( type == "visible" ) controller->type = Ctrl_Visible;
                    } else if (simple == "time"){
                        int start = 0, end =0, loop = 0;
                        try{
                            simple >> start;
                            simple >> end;
                            simple >> loop;
                        } catch (const Ast::Exception & e){
                        }

                        controller->timestart = start;
                        if (end == 0){
                            controller->endtime = start;
                        } else {
                            controller->endtime = end;
                        }

                        if (loop == 0){
                            controller->looptime = -1;
                        } else {
                            controller->looptime = loop;
                        }

                        Global::debug(1) << "start: " << controller->timestart << " | end: " << controller->endtime << " | loop: " << controller->looptime << endl;
                    } else if (simple == "value"){
                        simple >> controller->value1;
                        simple >> controller->value2;		    
                        simple >> controller->value3;
                    } else if (simple == "x"){
                        simple >> controller->value1;
                    } else if (simple == "y"){
                        simple >> controller->value2;
                    } else if (simple == "ctrlid"){
                        try{
                            // Max 10
                            while (true){
                                int id;
                                simple >> id;
                                manager.getBackgrounds(controller->backgrounds, id);
                            }
                        } catch (const Ast::Exception & e){
                        }
                    } else {
                        string name;
                        throw MugenException( "Unhandled option in BGCtrl " + name + " Section: " + simple.toString());
                    }
                }

                virtual ~CtrlWalker(){
                    if (controller->backgrounds.empty()){
                        controller->backgrounds.insert(controller->backgrounds.end(), control->backgrounds.begin(), control->backgrounds.end());
                    }
                }
            };

            {
                CtrlWalker walker(temp, control, *this);
                Ast::Section * section = *section_it;
                section->walk(walker);
            }

	    Global::debug(1) << "Controlling total backgrounds: " << temp->backgrounds.size() << endl;
	    control->addControl(temp);
	}
    }

    Global::debug(1) << "Got total backgrounds: " << backgrounds.size() << " total foregrounds: " << foregrounds.size() << endl;
}

MugenBackgroundManager::~MugenBackgroundManager(){
    if (!spriteFile.empty()){
	// Get rid of sprites
	for( std::map< unsigned int, std::map< unsigned int, MugenSprite * > >::iterator i = sprites.begin() ; i != sprites.end() ; ++i ){
	    for( std::map< unsigned int, MugenSprite * >::iterator j = i->second.begin() ; j != i->second.end() ; ++j ){
		if( j->second )delete j->second;
	    }
	}
    }
    // Get rid of animation lists;
    for( std::map< int, MugenAnimation * >::iterator i = animations.begin() ; i != animations.end() ; ++i ){
	if( i->second )delete i->second;
    }
    
    // Get rid of background lists;
    for( std::vector< MugenBackground * >::iterator i = backgrounds.begin() ; i != backgrounds.end() ; ++i ){
	if( (*i) )delete (*i);
    }
    
    // Get rid of foreground lists;
    for( std::vector< MugenBackground * >::iterator i = foregrounds.begin() ; i != foregrounds.end() ; ++i ){
	if( (*i) )delete (*i);
    }
    
    // Get rid of control lists;
    for( std::vector< MugenBackgroundController * >::iterator i = controllers.begin() ; i != controllers.end() ; ++i ){
	if( (*i) )delete (*i);
    }
}
void MugenBackgroundManager::logic( const double x, const double y, const double placementx, const double placementy ){
    
    // Backgrounds
    for( vector< MugenBackground *>::iterator i = backgrounds.begin(); i != backgrounds.end(); ++i ){
	(*i)->logic( x, y, placementx, placementy );
    }
    
     // Foregrounds
    for( vector< MugenBackground *>::iterator i = foregrounds.begin(); i != foregrounds.end(); ++i ){
	(*i)->logic( x, y, placementx, placementy );
    }
    
    // Controllers
    for( vector< MugenBackgroundController *>::iterator i = controllers.begin(); i != controllers.end(); ++i ){
	(*i)->act(animations);
    }
    
}
void MugenBackgroundManager::renderBack( const double windowx, const double windowy, const int totalLength, const int totalHeight, Bitmap *work ){
    if ( clearColor != -1)work->fill(clearColor);
	// debug overrides it
    if ( debugbg )work->fill( Bitmap::makeColor(255,0,255) );
    for( vector< MugenBackground *>::iterator i = backgrounds.begin(); i != backgrounds.end(); ++i ){
	(*i)->render( windowx, windowy, totalLength, totalHeight, work );
    }
    
}
void MugenBackgroundManager::renderFront( const double windowx, const double windowy, const int totalLength, const int totalHeight, Bitmap *work ){
    for( vector< MugenBackground *>::iterator i = foregrounds.begin(); i != foregrounds.end(); ++i ){
	(*i)->render( windowx, windowy, totalLength, totalHeight, work );
    }
}

void MugenBackgroundManager::preload( const int startx, const int starty ){
    // Set up the animations for those that have action numbers assigned (not -1 )
    // Also do their preload
    for( std::vector< MugenBackground * >::iterator i = backgrounds.begin(); i != backgrounds.end(); ++i ){
	MugenBackground *background = *i;
	    if( background->getActionNumber() != -1 ){
		background->setAnimation( animations[ background->getActionNumber() ] );
	    }
	    // now load
	    background->preload( startx, starty );
    }
    for( std::vector< MugenBackground * >::iterator i = foregrounds.begin(); i != foregrounds.end(); ++i ){
	MugenBackground *background = *i;
	    if( background->getActionNumber() != -1 ){
		background->setAnimation( animations[ background->getActionNumber() ] );
	    }
	    // now load
	    background->preload( startx, starty );
    }
}

void MugenBackgroundManager::reset( const int startx, const int starty, bool resetBG ){
    for( std::vector< MugenBackground * >::iterator i = backgrounds.begin(); i != backgrounds.end(); ++i ){
	// reset just reloads it to default
	MugenBackground *background = *i;
	if (resetBG){
	    background->preload( startx, starty );
	}
    }

    for( std::vector< MugenBackground * >::iterator i = foregrounds.begin(); i != foregrounds.end(); ++i ){
	// reset
	MugenBackground *background = *i;
	if (resetBG){
	    background->preload( startx, starty );
	}
    }
}

MugenBackground *MugenBackgroundManager::getBackground( int ID ){
    for( std::vector< MugenBackground * >::iterator i = backgrounds.begin(); i != backgrounds.end(); ++i ){
	if( (*i)->id == ID )return (*i);
    }
    for( std::vector< MugenBackground * >::iterator i = foregrounds.begin(); i != foregrounds.end(); ++i ){
	if( (*i)->id == ID )return (*i);
    }
    
    return 0;
}
void MugenBackgroundManager::getBackgrounds( std::vector<MugenBackground *> &bgs, int ID ){
    for( std::vector< MugenBackground * >::iterator i = backgrounds.begin(); i != backgrounds.end(); ++i ){
	if( (*i)->id == ID ){
	    bgs.push_back( *i );
	}
    }
    for( std::vector< MugenBackground * >::iterator i = foregrounds.begin(); i != foregrounds.end(); ++i ){
	if( (*i)->id == ID ){
	    bgs.push_back( *i );
	}
    }
}
