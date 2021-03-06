//
//  glass.cpp
//  Paladin
//
//  Created by SATAN_Z on 2019/9/24.
//

#include "glass.hpp"
#include "core/interaction.hpp"
#include "core/bxdf.hpp"
#include "core/texture.hpp"
#include "bxdfs/specular.hpp"
#include "materials/bxdfs/bsdf.hpp"
#include "bxdfs/microfacet/transmission.hpp"
#include "bxdfs/microfacet/reflection.hpp"

PALADIN_BEGIN

void GlassMaterial::computeScatteringFunctions(SurfaceInteraction *si,
                                               MemoryArena &arena,
                                               TransportMode mode,
                                               bool allowMultipleLobes) const {
    processNormal(si);
    
    Float eta = _eta->evaluate(*si);
    Float urough = _uRoughness->evaluate(*si);
    Float vrough = _vRoughness->evaluate(*si);
    Spectrum R = _Kr->evaluate(*si).clamp();
    Spectrum T = _Kt->evaluate(*si).clamp();
    
    si->bsdf = ARENA_ALLOC(arena, BSDF)(*si,eta);
    
    if (R.IsBlack() && T.IsBlack()) {
        return;
    }
    
    bool isSpecular = urough == 0 && vrough == 0;

    Float etaMedium = 1.0f;
    
    if (_thin || (isSpecular && allowMultipleLobes)) {
        // 这里的介质折射率不应该固定为1.0f，应该根据介质  todo
        FresnelSpecular * fr = ARENA_ALLOC(arena, FresnelSpecular)(R, T, etaMedium, eta, mode, _thin);
        si->bsdf->add(fr);
    } else {
        if (_remapRoughness) {
            urough = GGXDistribution::RoughnessToAlpha(urough);
            vrough = GGXDistribution::RoughnessToAlpha(vrough);
        }
        
        MicrofacetDistribution * distrib = isSpecular ?
                                            nullptr :
                                            ARENA_ALLOC(arena, GGXDistribution)(urough, vrough);
        if (!R.IsBlack()) {
            Fresnel *fresnel = ARENA_ALLOC(arena, FresnelDielectric)(etaMedium, eta);
            if (isSpecular) {
                BxDF * bxdf = ARENA_ALLOC(arena, SpecularReflection)(R, fresnel);
                si->bsdf->add(bxdf);
            } else {
                BxDF * bxdf = ARENA_ALLOC(arena, MicrofacetReflection)(R, distrib, fresnel);
                si->bsdf->add(bxdf);
            }
        }
        if (!T.IsBlack()) {
            if (isSpecular) {
                BxDF * bxdf = ARENA_ALLOC(arena, SpecularTransmission)(T, etaMedium, eta, mode);
                si->bsdf->add(bxdf);
            } else {
                BxDF * bxdf = ARENA_ALLOC(arena, MicrofacetTransmission)(T, distrib, etaMedium, eta, mode);
                si->bsdf->add(bxdf);
            }
        }
    }
}

//"param" : {
//    "Kr" : {
//        "type" : "constant",
//        "param" : {
//            "colorType" : 0,
//            "color" : [0.1, 0.9, 0.5]
//        }
//    },
//    "Kt" : {
//        "type" : "constant",
//        "param" : {
//            "colorType" : 0,
//            "color" : [0.1, 0.9, 0.5]
//        }
//    },
//    "uRough" : {
//        "type" : "constant",
//        "param" : 0.5
//    },
//    "vRough" : {
//        "type" : "constant",
//        "param" : 0.5
//    },
//    "eta" : {
//        "type" : "constant",
//        "param" : 0.5
//    },
//    "bumpMap" : {
//        "type" : "constant",
//        "param" : 0.5
//    },
//    "remapRough" : false
//}
CObject_ptr createGlass(const nloJson &param, const Arguments &lst) {
    
    nloJson _Kr = param.value("Kr", nloJson::object());
    auto Kr = shared_ptr<Texture<Spectrum>>(createSpectrumTexture(_Kr));
    
    nloJson _Kt = param.value("Kt", nloJson::object());
    auto Kt = shared_ptr<Texture<Spectrum>>(createSpectrumTexture(_Kt));
    
    nloJson _uRough = param.value("uRough", nloJson(0.f));
    auto uRough = shared_ptr<Texture<Float>>(createFloatTexture(_uRough));
    
    nloJson _vRough = param.value("vRough", nloJson(0.f));
    auto vRough = shared_ptr<Texture<Float>>(createFloatTexture(_vRough));
    
    nloJson _eta = param.value("eta", nloJson(1.f));
    auto eta = shared_ptr<Texture<Float>>(createFloatTexture(_eta));
    
    nloJson _normalMap = param.value("normalMap", nloJson());
    auto normalMap = shared_ptr<Texture<Spectrum>>(createSpectrumTexture(_normalMap));
    
    nloJson _bumpMap = param.value("bumpMap", nloJson(0.f));
    auto bumpMap = shared_ptr<Texture<Float>>(createFloatTexture(_bumpMap));
    
    bool remap = param.value("remapRough", false);
    
    bool thin = param.value("thin", true);
    
    auto ret = new GlassMaterial(Kr, Kt, uRough, vRough, eta, normalMap, bumpMap, remap, thin);
    return ret;
}

REGISTER("glass", createGlass)

PALADIN_END

