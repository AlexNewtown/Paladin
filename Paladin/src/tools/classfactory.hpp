//
//  classfactory.hpp
//  Paladin
//
//  Created by SATAN_Z on 2019/11/11.
//

#ifndef classfactory_hpp
#define classfactory_hpp

#include "core/header.h"

PALADIN_BEGIN

class ClassFactory {
    
public:
    
    createObject getCreatorByName(const std::string& className);
    
    void registClass(const std::string& className, createObject func);
    
    static ClassFactory * getInstance();
    
private:
    
    ClassFactory(const ClassFactory&) {
        
    }
    
    ClassFactory& operator=(const ClassFactory&) {
        return *this;
    }
    
    std::map<std::string, createObject> _classMap;
    
    ClassFactory() {
        
    };
    
    static ClassFactory * _instance;
};

class RegisterAction {
    
public:
    RegisterAction() {
        
    }
    
    RegisterAction(const std::string &className, createObject creator) {
        COUT << "haha" << std::endl;
        ClassFactory::getInstance()->registClass(className, creator);
    }
};

#define REGISTER(className, creatorName)                            \
RegisterAction g_Register##creatorName(className,(createObject)creatorName);


PALADIN_END

#endif /* classfactory_hpp */
