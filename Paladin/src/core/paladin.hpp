//
//  paladin.hpp
//  Paladin
//
//  Created by SATAN_Z on 2019/6/30.
//  Copyright © 2019 Zero. All rights reserved.
//

#ifndef paladin_hpp
#define paladin_hpp

#include "parser/sceneparser.hpp"
#include "tools/parallel.hpp"

PALADIN_BEGIN

class Paladin {
public:
    
    int run() {
        return 0;
    }
    
    void render(const std::string &fn) {
        _basePath = fn.substr(0, fn.find_last_of("/"));
        _sceneParser.loadFromJson(fn);
        parallelCleanup();
    }
    
    static Paladin * getInstance();
    
    const SceneParser * getSceneParser() const {
        return & _sceneParser;
    }
    
    inline string getBasePath() {
        return _basePath;
    }
    
private:
    
    Paladin() {
        
    }
    
    static Paladin * s_paladin;
    
    SceneParser _sceneParser;
    
    string _basePath = "";

};

PALADIN_END

#endif /* paladin_hpp */
