#include "options.h"
#include "menu/menu_option.h"
#include "menu/options.h"
#include "menu/menu-exception.h"
#include "../object/player.h"
#include "../object/buddy_player.h"
#include "../level/utils.h"
#include "mod.h"
#include "menu/menu.h"
#include "configuration.h"
#include "game.h"
#include "util/file-system.h"
#include "util/funcs.h"
#include "util/token.h"
#include "util/tokenreader.h"
#include <sstream>
#include <vector>

#include "openbor/pack-reader.h"

using namespace std;

namespace Paintown{

Level::LevelInfo doLevelMenu(const string dir, const Menu::Context & context){
    vector<Level::LevelInfo> possible = Mod::getCurrentMod()->getLevels();
    if (possible.size() == 0){
        throw LoadException(__FILE__, __LINE__, "No level sets defined!");
    }

    if (possible.size() == 1){
        return possible[0];
    }
    
    // FIXME what is this crap? Do we need it?
    /* don't hardcode 60, base it on the size of the font */
    int count = possible.size() * 60;
    /* what is 250?? */
    if (count > 250){
        count = 250;
    }

    try{
        Menu::Menu temp;
        
        int index = 0;
        for ( unsigned int i = 0; i < possible.size(); i++ ){
            OptionLevel *opt = new OptionLevel(0, &index, i);
            opt->setText(possible[i].getName());
            opt->setInfoText("Select a set of levels to play");
            temp.addOption(opt);
        }
        // Run it
        try {
            temp.run(context);
        } catch (const Menu::MenuException & ex){
        }
        return possible[index];
    } catch (const TokenException & ex){
        // Global::debug(0) << "There was a problem with the token. Error was:\n  " << ex.getReason() << endl;
        throw LoadException(__FILE__, __LINE__, ex, "Could not load levels");
    }
    throw LoadException(__FILE__, __LINE__, "No level chosen!");
}

OptionFactory::OptionFactory(){
}

class OptionAdventure: public MenuOption {
public:
    OptionAdventure(const Token *token):
        MenuOption(token){
            if ( *token != "adventure" ){
                throw LoadException(__FILE__, __LINE__, "Not an adventure");
            }

            readName(token);
        }

    ~OptionAdventure(){
        // Nothing
    }

    void logic(){
    }

    void run(const Menu::Context & context){
        Object * player = NULL;
        try{
            //string level = Game::selectLevelSet( Util::getDataPath() + "/levels" );
            Level::LevelInfo info = doLevelMenu("/levels", context);

            Global::debug(1) << "Selecting players" << endl;
            int remap = 0;
            Filesystem::AbsolutePath path = Mod::getCurrentMod()->selectPlayer("Pick a player", info, remap);

            PlayerFuture future(path, Configuration::getInvincible(), Configuration::getLives(), remap);
            vector<Util::Future<Object *> *> players;
            players.push_back(&future);
            Game::realGame(players, info);
        } catch ( const LoadException & le ){
            Global::debug(0) << "Error while loading: " << le.getTrace() << endl;
        } catch (const Exception::Return & ignore){
            throw Menu::Reload(__FILE__, __LINE__);
        }

        /* player will be null if an exception occurred before selectPlayer was called */
        /*
           if ( player != NULL ){
           delete player;
           }
           */
    }
};

class OptionAdventureCpu: public MenuOption {
public:
    OptionAdventureCpu(const Token *token):
        MenuOption(token){
            if (*token != "adventure-cpu"){
                throw LoadException(__FILE__, __LINE__, "Not an adventure");
            }

            readName(token);
        }

    ~OptionAdventureCpu(){
    }

    void logic(){
    }

    static string nthWord(int i){
        switch (i){
            case 1 : return "first";
            case 2 : return "second";
            case 3 : return "third";
            case 4 : return "fourth";
            case 5 : return "fifth";
            case 6 : return "sixth";
            case 7 : return "seventh";
            case 8 : return "eigth";
            case 9 : return "ninth";
            case 10 : return "tenth";
            default : {
                ostringstream out;
                out << i;
                return out.str();
            }
        }
    }

    void run(const Menu::Context & context){
        int max_buddies = Configuration::getNpcBuddies();

        Object * player = NULL;
        vector<Util::Future<Object*>* > futures;
        vector<Object *> buddies;
        try{
            Level::LevelInfo info = doLevelMenu("/levels", context);

            int remap;
            Filesystem::AbsolutePath path = Mod::getCurrentMod()->selectPlayer("Pick a player", info, remap);
            Util::Future<Object*> * player = new PlayerFuture(path, Configuration::getInvincible(), Configuration::getLives(), remap);
            futures.push_back(player);

            for ( int i = 0; i < max_buddies; i++ ){
                ostringstream out;
                out << "Pick buddy " << nthWord(i+1);
                int remap;
                Filesystem::AbsolutePath path = Mod::getCurrentMod()->selectPlayer(out.str(), info, remap);
                Util::Future<Object*> * buddy = new BuddyFuture(path, player, remap, -(i+2));
                futures.push_back(buddy);
            }

            Game::realGame(futures, info);
        } catch ( const LoadException & le ){
            Global::debug( 0 ) << "Could not load player: " << le.getTrace() << endl;
        } catch (const Exception::Return & ignore){
            throw Menu::Reload(__FILE__, __LINE__);
        }

        for (vector<Util::Future<Object*>*>::iterator it = futures.begin(); it != futures.end(); it++){
            delete *it;
        }
    }
};

class OptionChangeMod: public MenuOption {
public:
    OptionChangeMod(const Token *token):
        MenuOption(token){
            if ( *token != "change-mod" ){
                throw LoadException(__FILE__, __LINE__, "Not a change mod");
            }

            /* TODO: fix */
            setText("Change mod");
        }

    ~OptionChangeMod(){
        // Nothing
    }

    void logic(){
    }

    static bool isModFile(const std::string & path){
        try{
            TokenReader reader(path);
            Global::debug(1) << "Checking for a mod in " << path << endl;
            if (*reader.readToken() == "game"){
                return true;
            }
        } catch (const TokenException & e){
            return false;
        }
        return false;
    }

    static bool isOpenBorPackfile(Filesystem::AbsolutePath path){
        try{
            Bor::PackReader reader(path);
            return true;
        } catch (const Bor::PackError & error){
            Global::debug(0) << "Error reading pak file: " << error.getTrace() << endl;
            return false;
        }
    }

    struct ModType{
        enum Kind{
            Paintown,
            Openbor
        };

        ModType(const Filesystem::AbsolutePath & path, Kind type):
            path(path),
            type(type){
            }

        Filesystem::AbsolutePath path; 
        Kind type;
    };

    static vector<ModType> findMods(){
        vector<ModType> mods;

        vector<Filesystem::AbsolutePath> directories = Filesystem::findDirectories(Filesystem::RelativePath("."));
        for (vector<Filesystem::AbsolutePath>::iterator dir = directories.begin(); dir != directories.end(); dir++){
            string file = (*dir).path() + "/" + Filesystem::cleanse(*dir).path() + ".txt";
            if (isModFile(file)){
                mods.push_back(ModType(Filesystem::AbsolutePath(file), ModType::Paintown));
            }
        }

        try{
            vector<Filesystem::AbsolutePath> pakFiles = Filesystem::getFiles(Filesystem::find(Filesystem::RelativePath("paks")), "*", true);
            for (vector<Filesystem::AbsolutePath>::iterator it = pakFiles.begin(); it != pakFiles.end(); it++){
                const Filesystem::AbsolutePath & path = *it;
                if (isOpenBorPackfile(path)){
                    Global::debug(0) << "Found openbor pakfile " << path.path() << endl;
                    mods.push_back(ModType(path, ModType::Openbor));
                }
            }
        } catch (const Filesystem::NotFound & n){
            Global::debug(0) << "Could not find any pak files: " << n.getTrace() << endl;
        }

        return mods;
    }

    static string modNamePaintown(const Filesystem::AbsolutePath & path){
        try{
            TokenReader reader(path.path());
            Global::debug(1) << "Checking for a mod in " << path.path() << endl;
            const Token * name_token = reader.readToken()->findToken("game/name");
            if (name_token != NULL){
                string name;
                name_token->view() >> name;
                return name;
            }
            return Filesystem::cleanse(path).path();
        } catch (const TokenException & e){
            return Filesystem::cleanse(path).path();
        }
    }

    static string modNameOpenbor(const Filesystem::AbsolutePath & path){
        return "OpenBor " + Filesystem::cleanse(path).path();
    }

    static string modName(const ModType & mod){
        switch (mod.type){
            case ModType::Paintown : return modNamePaintown(mod.path);
            case ModType::Openbor : return modNameOpenbor(mod.path);
            default : return "unknown!!";
        }
    }

    static void changeMod(const ModType & mod){
        switch (mod.type){
            case ModType::Paintown : {
                string path = mod.path.path();
                size_t slash = path.rfind('/');
                size_t txt = path.rfind(".txt");
                if (slash != string::npos && txt != string::npos){
                    string name = path.substr(slash + 1, path.length() - slash - 5);
                    Configuration::setCurrentGame(name);
                    Paintown::Mod::loadPaintownMod(name);
                } else {
                    Global::debug(0) << "Could not change mod to " << path << endl;
                }
                break;
            }
            case ModType::Openbor : {
                Paintown::Mod::loadOpenborMod(mod.path);
                break;
            }
        }
    }

    void run(const Menu::Context & context){
        try{
            int select = 0;
            Menu::Menu menu;
            vector<ModType> mods = findMods();
            map<int, ModType*> modMap;
            int index = 0;
            std::vector<OptionLevel *> options;
            for (vector<ModType>::iterator it = mods.begin(); it != mods.end(); it++){
                OptionLevel *opt = new OptionLevel(0, &select, index);
                string name = modName(*it);
                modMap[index] = &(*it);
                opt->setText(name);
                opt->setInfoText("Choose this mod");
                if (name.compare(Util::upcase(Configuration::getCurrentGame())) == 0){
                    options.insert(options.begin(),opt);
                } else {
                    options.push_back(opt);
                }
                index += 1;
            }

            for (std::vector<OptionLevel *>::iterator it = options.begin(); it != options.end(); ++it){
                menu.addOption(*it);
            }

            if (index == 0){
                Global::debug(0) << "No mods found!" << endl;
                return;
            }

            try{
                menu.run(context);
            } catch (const Exception::Return & ignore){
                throw Menu::Reload(__FILE__, __LINE__);
            } catch (const Menu::MenuException & ignore){
            }

            changeMod(*modMap[select]);

            // Reload the menu
            throw ReloadMenuException();

        } catch (const LoadException & le){
            Global::debug(0) << "Could not load menu/change-mod.txt: " << le.getTrace() << endl;
        } catch (const Filesystem::NotFound & e){
            Global::debug(0) << "Could not load menu/change-mod.txt: " << e.getTrace() << endl;
        }
    }

};

MenuOption * OptionFactory::getOption(const Token *token) const {
    const Token * child;
    token->view() >> child;

    if (*child == "adventure"){
        return new OptionAdventure(child);
    } else if (*child == "adventure-cpu"){
        return new OptionAdventureCpu(child);
    } else if (*child == "change-mod"){
        return new OptionChangeMod(child);
    }

    return Menu::OptionFactory::getOption(token);
}

OptionFactory::~OptionFactory(){

}

}
