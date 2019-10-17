//
//  distant.hpp
//  Paladin
//
//  Created by SATAN_Z on 2019/10/18.
//

#ifndef distant_hpp
#define distant_hpp

#include "core/light.hpp"
#include "core/scene.hpp"

PALADIN_BEGIN

class DistantLight : public Light {
public:

    DistantLight(const Transform &LightToWorld, const Spectrum &L,
                 const Vector3f &w);
    
    virtual void Preprocess(const Scene &scene) {
        scene.worldBound().boundingSphere(&_worldCenter, &_worldRadius);
    }
    
    virtual Spectrum sampleLi(const Interaction &ref, const Point2f &u, Vector3f *wi,
                       Float *pdf, VisibilityTester *vis) const;
    
    virtual Spectrum power() const;
    
    virtual Float pdfLi(const Interaction &, const Vector3f &) const;
    
private:
    const Spectrum _L;
    const Vector3f _wLight;
    Point3f _worldCenter;
    Float _worldRadius;
};

PALADIN_END

#endif /* distant_hpp */