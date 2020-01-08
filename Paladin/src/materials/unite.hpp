//
//  unite.hpp
//  Paladin
//
//  Created by SATAN_Z on 2020/1/8.
//

#ifndef unite_hpp
#define unite_hpp

#include "core/header.h"
#include "core/material.hpp"

/**
 *
 * 简单理解为联合材质，代码参考pbrt中的uber，实在是不理解为何用uber
 * 来作为材质命名，所以就取了个新的名字，unite
 * 
 */

PALADIN_BEGIN

class UniteMaterial : public Material {
    
public:
    UniteMaterial(const std::shared_ptr<Texture<Spectrum>> &Kd,
				const std::shared_ptr<Texture<Spectrum>> &Ks,
				const std::shared_ptr<Texture<Spectrum>> &Kr,
				const std::shared_ptr<Texture<Spectrum>> &Kt,
				const std::shared_ptr<Texture<Float>> &roughness,
				const std::shared_ptr<Texture<Float>> &roughnessu,
				const std::shared_ptr<Texture<Float>> &roughnessv,
				const std::shared_ptr<Texture<Spectrum>> &opacity,
				const std::shared_ptr<Texture<Float>> &eta,
				const std::shared_ptr<Texture<Float>> &bumpMap,
				bool remapRoughness)
    : _Kd(Kd),
	_Ks(Ks),
	_Kr(Kr),
	_Kt(Kt),
	_opacity(opacity),
	_roughness(roughness),
	_roughness_u(roughnessu),
	_roughness_v(roughnessv),
	_eta(eta),
	_bumpMap(bumpMap),
    _remapRoughness(remapRoughness) {
        
    }

private:
    std::shared_ptr<Texture<Spectrum>> _Kd, _Ks, _Kr, _Kt, _opacity;
    std::shared_ptr<Texture<Float>> _roughness, _roughness_u,_roughness_v; 
    std::shared_ptr<Texture<Float>> _eta, _bumpMap;
    bool _remapRoughness;
};

CObject_ptr createUniteMaterial(const nloJson &param, const Arguments &lst);


PALADIN_END

#endif /* unite_hpp */