#include "mugen/game.h"

#include "util/bitmap.h"
#include "util/font.h"
#include "util/token.h"
#include "configuration.h"
#include "menu/menu.h"
#include "menu/menu_global.h"
#include "globals.h"
#include "init.h"
#include "input/keyboard.h"
#include "input/input-manager.h"
#include "input/input-map.h"
#include "util/funcs.h"
#include "factory/font_render.h"
#include "loading.h"

#include "character.h"
#include "mugen_util.h"
#include "mugen_font.h"
#include "mugen_menu.h"
#include "mugen_stage.h"
#include "character_select.h"

namespace PaintownUtil = ::Util;

using namespace Mugen;

Game::Game(GameType type, const std::string & systemFile):
type(type),
systemFile(systemFile),
motifDirectory(Util::getFileDir(systemFile)),
team1(0),
team2(0){
}

Game::~Game(){
}

/* This needs to be changed so that it's grabbed from configuration or whatever */

static InputMap<Mugen::CharacterKeys> getSelect1Keys(){
    InputMap<Mugen::CharacterKeys> input;
    input.set(Keyboard::Key_UP, 0, true, Mugen::Up);
    input.set(Keyboard::Key_DOWN, 0, true, Mugen::Down);
    input.set(Keyboard::Key_RIGHT, 0, true, Mugen::Right);
    input.set(Keyboard::Key_LEFT, 0, true, Mugen::Left);

    input.set(Keyboard::Key_A, 0, true, Mugen::A);
    input.set(Keyboard::Key_S, 0, true, Mugen::B);
    input.set(Keyboard::Key_D, 0, true, Mugen::C);
    input.set(Keyboard::Key_Z, 0, true, Mugen::X);
    input.set(Keyboard::Key_X, 0, true, Mugen::Y);
    input.set(Keyboard::Key_C, 0, true, Mugen::Z);
    input.set(Keyboard::Key_ENTER, 0, true, Mugen::Start);
    return input;
}

static InputMap<Mugen::CharacterKeys> getSelect2Keys(){
    InputMap<Mugen::CharacterKeys> input;
    input.set(Keyboard::Key_H, 0, true, Mugen::Up);
    input.set(Keyboard::Key_Y, 0, true, Mugen::Down);
    input.set(Keyboard::Key_J, 0, true, Mugen::Right);
    input.set(Keyboard::Key_G, 0, true, Mugen::Left);

    input.set(Keyboard::Key_I, 0, true, Mugen::A);
    input.set(Keyboard::Key_O, 0, true, Mugen::B);
    input.set(Keyboard::Key_P, 0, true, Mugen::C);
    input.set(Keyboard::Key_8, 0, true, Mugen::X);
    input.set(Keyboard::Key_9, 0, true, Mugen::Y);
    input.set(Keyboard::Key_0, 0, true, Mugen::Z);
    input.set(Keyboard::Key_L, 0, true, Mugen::Start);
    return input;
}

void Game::run(){
    Bitmap screen(GFX_X, GFX_Y);
    
    int ticker = 0;
#if 0    
    std::string selectFile = Util::probeDef(systemFile, "files", "select");
    Mugen::CharacterSelect select(ticker, selectFile);
    select.load();
    
    /* This will change with select screen */
    Mugen::SelectedChars * gameInfo = 0;
    switch (type){
	default:
	case Arcade:
	    gameInfo = select.run("Arcade" , 1, true, &screen);
	    break;
	case Versus:
	    gameInfo = select.run("Versus Mode" , 2, true, &screen);
	    break;
	case TeamArcade:
	    gameInfo = select.run("Team Arcade" , 1, true, &screen);
	    break;
	case TeamVersus:
	    gameInfo = select.run("Team Versus" , 2, true, &screen);
	    break;
	case TeamCoop:
	    gameInfo = select.run("Team Cooperative" , 1, true, &screen);
	    break;
	case Survival:
	    gameInfo = select.run("Survival" , 1, true, &screen);
	    break;
	case SurvivalCoop:
	    gameInfo = select.run("Survival Cooperative" , 1, true, &screen);
	    break;
	case Training:
	    gameInfo = select.run("Training Mode" , 1, true, &screen);
	    break;
	case Watch:
	    gameInfo = select.run("Watch Mode" , 1, true, &screen);
	    break;
    }
    if (gameInfo == 0){
        return;
    }
    
    runGame(setupStage(gameInfo), screen);
#endif
}
#if 0
void Game::runGame(MugenStage * stage, const Bitmap & screen){
    InputMap<int> gameInput;
    gameInput.set(Keyboard::Key_F1, 10, false, 0);
    gameInput.set(Keyboard::Key_F2, 10, false, 1);
    gameInput.set(Keyboard::Key_F3, 10, false, 2);
    gameInput.set(Keyboard::Key_F4, 10, true, 3);
    gameInput.set(Keyboard::Key_ESC, 0, true, 4);
    
    // Load the stage
    try{
        /* FIXME: replace hard coded numbers */
        Bitmap work(320,240);
        bool quit = false;
        double gameSpeed = 1.0;
        double runCounter = 0;
        double mugenSpeed = 60;

        // Lets reset the stage for good measure
        stage->reset();

        while( !quit ){
            bool draw = false;

            if ( Global::speed_counter > 0 ){
                runCounter += Global::speed_counter * gameSpeed * mugenSpeed / Global::TICS_PER_SECOND;
                while (runCounter > 1){
                    InputManager::poll();
                    stage->logic();
                    runCounter -= 1;
                    draw = true;

                    InputMap<int>::Output out = InputManager::getMap(gameInput);
                    if (out[0]){
                        gameSpeed -= 0.1;
                    }
                    if (out[1]){
                        gameSpeed += 0.1;
                    }
                    if (out[2]){
                        gameSpeed = 1;
                    }
		    if (out[3]){
			stage->toggleDebug();
		    }
		    if (out[4]){
			quit = true;
		    }
                    if (gameSpeed < 0.1){
                        gameSpeed = 0.1;
                    }
                }
                Global::speed_counter = 0;
            }

            if (draw){
                stage->render(&work);
                work.Stretch(screen);

                FontRender * render = FontRender::getInstance();
                render->render(&screen);
    
                screen.BlitToScreen();
            }

            while (Global::speed_counter == 0){
                PaintownUtil::rest(1);
            }
        }
    } catch (const MugenException &ex){
	Global::debug(0) << "Problem with stage: " << stage->getName() << " Problem was: " << ex.getReason() << std::endl;
    }
}

MugenStage * Game::setupStage(Mugen::SelectedChars * gameInfo){
    try{
        /* Need to display VS Screen here */
	
	MugenStage * stage = gameInfo->selectedStage;
        // Load player 1
        gameInfo->team1[0]->load();
        gameInfo->team1[0]->setInput(getPlayer1Input(), getPlayer1Input());
        stage->addp1(gameInfo->team1[0]);

        /* for testing, load kfm as player 2 */
       // stage->addp2(loadKfm());
        stage->load();
        return stage;
    } catch (const MugenException & e){
        throw e;
    }
}
#endif

