//
//  sceneparser.hpp
//  Paladin
//
//  Created by SATAN_Z on 2019/11/11.
//

#ifndef sceneparser_hpp
#define sceneparser_hpp

#include "transformcache.h"
#include <fstream>
#include "core/integrator.hpp"


PALADIN_BEGIN

USING_STD

class SceneParser {
    
public:
    void loadFromJson(const std::string &fn) {
        try {
            std::ifstream fst;
            fst.open(fn.c_str());
            stringstream buffer;
            buffer << fst.rdbuf();
            string str = buffer.str();
            nloJson json = nloJson::parse(str);
            parse(json);
        } catch (const std::exception &exc) {
            cout << exc.what();
            return;
        }
    }
    
    const TransformCache &getTransformCache() const {
        return _transformCache;
    }
    
    void parse(const nloJson &);
    
    void parseShapes(const nloJson &);
    
    Sampler * parseSampler(const nloJson &param);
    
    Camera * parseCamera(const nloJson &, Film *);
    
    Integrator * parseIntegrator(const nloJson &,Sampler * sampler, Camera * camera);
    
    void parseMaterials(const nloJson &);
    
    Filter * parseFilter(const nloJson &);
    
    void parseLights(const nloJson &);
    
    // 解析简单物体，球体，圆柱，圆锥等
    void parseSimpleShape(const nloJson &data, const string &type);
    
    // 解析模型
    void parseModel(const nloJson &data);
    
    shared_ptr<Aggregate> parseAccelerator(const nloJson &);
    
    Film * parseFilm(const nloJson &param, Filter *);
    
private:
    
    void addMaterialToCache(const string &name, const shared_ptr<const Material> &material) {
        // todo
        _materialCache[name] = material;
    }
    
    shared_ptr<const Material> getMaterial(const nloJson &name) {
        if (name.is_null()) {
            return nullptr;
        }
        auto iter = _materialCache.find(name);
        if (iter == _materialCache.end()) {
            return nullptr;
        }
        return _materialCache[name];
    }
    
private:
    
    TransformCache _transformCache;
    
    shared_ptr<Aggregate> _aggregate;
    
    unique_ptr<Integrator> _integrator;
    
    unique_ptr<Scene> _scene;
    
    vector<shared_ptr<Light>> _lights;
    
    vector<shared_ptr<Primitive>> _primitives;
    
    // 先用map储存着，待后续优化todo
    map<string, shared_ptr<const Material>> _materialCache;
    
    Transform _cameraToWorld;
};

PALADIN_END

#endif /* sceneparser_hpp */
