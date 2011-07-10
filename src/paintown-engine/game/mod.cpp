#include <string>
#include <vector>
#include "mod.h"
#include "paintown-engine/level/utils.h"
#include "util/file-system.h"
#include "util/tokenreader.h"
#include "util/token.h"
#include "util/load_exception.h"
#include "util/bitmap.h"
#include "openbor/mod.h"
#include "globals.h"

using namespace std;

namespace Paintown{

Mod * Mod::currentMod = NULL;
Mod::Mod(const string & name, const Filesystem::AbsolutePath & path):
name(name){
    try{
        TokenReader reader(path.path());
        Token * head = reader.readToken();

        const Token * name = head->findToken("game/name");
        const Token * token_menu = head->findToken("game/menu");
        if (token_menu == NULL){
            throw LoadException(__FILE__, __LINE__, "No game/menu token found. Add (menu some/path) to the mod file.");
        }
        token_menu->view() >> menu;

        vector<const Token*> token_level = head->findTokens("game/level-set");
        Global::debug(1) << "Found " << token_level.size() << " level sets" << endl;
        for (vector<const Token*>::iterator it = token_level.begin(); it != token_level.end(); it++){
            const Token * set = *it;
            levels.push_back(Level::readLevel(set));
        }

    } catch (const TokenException & e){
        Global::debug(0) << "Error while reading mod " << path.path() << ":" << e.getTrace() << endl;
    }
}
    
Graphics::Bitmap * Mod::createBitmap(const Filesystem::RelativePath & path){
    return new Graphics::Bitmap(Storage::instance().find(path).path());
}

Graphics::Bitmap Mod::makeBitmap(const Filesystem::RelativePath & path){
    Graphics::Bitmap * what = createBitmap(path);
    Graphics::Bitmap out(*what);
    delete what;
    return out;
}
    
Mod::Mod(){
}

Mod::~Mod(){
}
    
vector<Level::LevelInfo> Mod::getLevels(){
    return levels;
}
    
const string Mod::getMenu(){
    return menu;
}

void Mod::loadDefaultMod(){
    loadPaintownMod("paintown");
}

void Mod::setMod(Mod * mod){
    if (currentMod != NULL){
        delete currentMod;
    }
    currentMod = mod;
}

void Mod::loadOpenborMod(const Filesystem::AbsolutePath & path){
    setMod(new OpenborMod(path));
}

void Mod::loadPaintownMod(const string & name){
   string path = name + "/" + name + ".txt"; 
   setMod(new Mod(name, Storage::instance().find(Filesystem::RelativePath(path))));
}
    
Filesystem::AbsolutePath Mod::find(const Filesystem::RelativePath & path){
    Storage::System & filesystem = Storage::instance();
    string totalFailure;
    /* first search in the mod directory */
    try{
        return filesystem.find(Filesystem::RelativePath(name).join(path));
    } catch (const Filesystem::NotFound & fail){
        totalFailure = fail.getTrace();
    }

    /* then search in the regular place */
    try{
        return filesystem.find(path);
    } catch (const Filesystem::NotFound & fail){
        /* if a file can't be found then combine the errors */
        totalFailure += fail.getTrace();
        throw Filesystem::NotFound(__FILE__, __LINE__, fail, totalFailure);
    }
}

Mod * Mod::getCurrentMod(){
    return currentMod;
}

}
