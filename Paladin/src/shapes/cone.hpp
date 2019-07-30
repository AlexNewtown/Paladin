//
//  cone.hpp
//  Paladin
//
//  Created by SATAN_Z on 2019/6/30.
//  Copyright © 2019 Zero. All rights reserved.
//

#ifndef cone_hpp
#define cone_hpp

#include "header.h"
#include "shape.hpp"

PALADIN_BEGIN

class Cone : public Shape {
public:

    Cone(const Transform *o2w, const Transform *w2o, bool reverseOrientation,
         Float height, Float radius, Float phiMax);
    
    virtual void init();
    
    virtual Bounds3f objectBound() const;

    virtual bool intersect(const Ray &ray, Float *tHit, SurfaceInteraction *isect, bool testAlphaTexture) const;

    virtual bool intersectP(const Ray &ray, bool testAlphaTexture) const;

    virtual Float area() const;

    virtual Interaction sampleA(const Point2f &u, Float *pdf) const;
    
protected:

    const Float _radius, _height, _phiMax;
};

PALADIN_END

#endif /* cone_hpp */
