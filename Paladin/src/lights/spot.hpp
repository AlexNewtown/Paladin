//
//  spot.hpp
//  Paladin
//
//  Created by SATAN_Z on 2019/10/18.
//

#ifndef spot_hpp
#define spot_hpp

#include "core/light.hpp"

PALADIN_BEGIN

class SpotLight : public Light {
    
public:
    SpotLight(const Transform * LightToWorld, const MediumInterface &m,
              const Spectrum &I, Float totalWidth, Float falloffStart);
    
    virtual Spectrum sample_Li(const Interaction &ref, const Point2f &u, Vector3f *wi,
                       Float *pdf, VisibilityTester *vis) const override;
    
    virtual Spectrum sample_Li(DirectSamplingRecord *rcd, const Point2f &u,
                               const Scene &) const override;
    
    Float falloff(const Vector3f &w) const;
    
    virtual nloJson toJson() const override {
        return nloJson();
    }
    
    virtual Spectrum sample_Le(const Point2f &u1, const Point2f &u2,
                                Float time, Ray *ray, Normal3f *nLight,
                                Float *pdfPos, Float *pdfDir) const override;
    
    virtual void pdf_Le(const Ray &ray, const Normal3f &nLight,
                        Float *pdfPos, Float *pdfDir) const override;
    
    virtual Spectrum power() const override;
    
    virtual Float pdf_Li(const Interaction &, const Vector3f &) const override;
    
    virtual Float pdf_Li(const DirectSamplingRecord &) const override;
    
private:
    const Point3f _pos;
    const Spectrum _I;
    const Float _cosTotalWidth, _cosFalloffStart;
};

CObject_ptr createSpot(const nloJson &param, const Arguments &lst);

PALADIN_END

#endif /* spot_hpp */
