//
//  scene.cpp
//  Paladin
//
//  Created by SATAN_Z on 2019/6/30.
//  Copyright © 2019 Zero. All rights reserved.
//

#include "scene.hpp"
#include "tools/embree_util.hpp"

PALADIN_BEGIN

void Scene::initEnvmap() {
    for (const auto &light : lights) {
        light->preprocess(*this);
        if (light->flags & (int)LightFlags::Infinite) {
            infiniteLights.push_back(light);
        }
    }
}

bool Scene::rayIntersectTr(Ray ray, Sampler &sampler, SurfaceInteraction *isect,
                        Spectrum *Tr) const {
    *Tr = Spectrum(1.f);
    while (true) {
        bool hitSurface = rayIntersect(ray, isect);
        if (ray.medium) {
            *Tr = ray.medium->Tr(ray, sampler);
        }
        
        if (!hitSurface) {
            return false;
        }
        
        if (isect->shape->getMaterial() != nullptr) {
             return true;
        }
        ray = isect->spawnRay(ray.dir);
    }
}

void Scene::initAccel(const nloJson &data, const vector<shared_ptr<const Shape> > &shapes) {
    string type = data.value("type", "embree");
    _shapes = shapes;
    if (type == "embree") {
        InitAccelEmbree(shapes);
    } else {
        InitAccelNative(data, shapes);
    }
}

bool Scene::rayIntersectEmbree(const Ray &ray, SurfaceInteraction *isect) const {
    RTCIntersectContext context;
    rtcInitIntersectContext(&context);
    RTCRayHit rh = EmbreeUtil::toRTCRayHit(ray);
    rtcIntersect1(_rtcScene, &context, &rh);
    if (rh.hit.geomID == RTC_INVALID_GEOMETRY_ID) {
        return false;
    }
    uint32_t gid = rh.hit.geomID;
    uint32_t pid = rh.hit.primID;
   
    auto shape = _shapes.at(gid).get();
    auto prim = shape->getPrimitive(pid);
    
    Vector2f uv(rh.hit.u, rh.hit.v);
    
    prim->fillSurfaceInteraction(ray, uv, isect);
    
    return true;
}

void Scene::InitAccelNative(const nloJson &data, const vector<shared_ptr<const Shape>> &shapes) {
    
    _aggregate = createAccelerator(data, shapes);
    _worldBound = _aggregate->worldBound();
    initEnvmap();
}

void Scene::InitAccelEmbree(const vector<shared_ptr<const Shape>>&shapes) {
    EmbreeUtil::initDevice();
    _rtcScene = rtcNewScene(EmbreeUtil::getDevice());
    int idx = 0;
    for (size_t i = 0; i < shapes.size(); ++i) {
        auto prim = shapes[i];
        RTCGeometry gid = prim->rtcGeometry(this);
        _worldBound = unionSet(_worldBound, prim->worldBound());
        if (gid != nullptr) {
            rtcAttachGeometry(_rtcScene, gid);
        }
    }
    rtcCommitScene(_rtcScene);
}

PALADIN_END
