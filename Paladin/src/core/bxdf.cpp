//
//  bxdf.cpp
//  Paladin
//
//  Created by SATAN_Z on 2019/6/30.
//  Copyright © 2019 Zero. All rights reserved.
//

#include "core/bxdf.hpp"
#include "math/sampling.hpp"
#include "core/interaction.hpp"
#include "math/interpolation.hpp"

PALADIN_BEGIN

Float FrDielectric(Float cosThetaI, Float etaI, Float etaT) {
    cosThetaI = clamp(cosThetaI, -1, 1);

    bool entering = cosThetaI > 0.f;
    // 如果如果入射角大于90° 
    // 则法线方向反了，cosThetaI取绝对值，对换两个折射率
    if (!entering) {
        std::swap(etaI, etaT);
        cosThetaI = std::abs(cosThetaI);
    }
    
    // 用斯涅耳定律计算sinThetaI
    Float sinThetaI = std::sqrt(std::max((Float)0, 1 - cosThetaI * cosThetaI));
    Float sinThetaT = etaI / etaT * sinThetaI;
    
    // 全内部反射情况
    if (sinThetaT >= 1) {
        return 1;
    }
    // 套公式
    Float cosThetaT = std::sqrt(std::max((Float)0, 1 - sinThetaT * sinThetaT));
    Float Rparl = ((etaT * cosThetaI) - (etaI * cosThetaT))
                / ((etaT * cosThetaI) + (etaI * cosThetaT));
    Float Rperp = ((etaI * cosThetaI) - (etaT * cosThetaT))
                / ((etaI * cosThetaI) + (etaT * cosThetaT));
    return (Rparl * Rparl + Rperp * Rperp) / 2;
}

Spectrum FrConductor(Float cosThetaI, const Spectrum &etai,
                     const Spectrum &etat, const Spectrum &kt) {
    cosThetaI = clamp(cosThetaI, -1, 1);
    Spectrum eta = etat / etai;
    Spectrum etak = kt / etai;
    
    Float cosThetaI2 = cosThetaI * cosThetaI;
    Float sinThetaI2 = 1. - cosThetaI2;
    Spectrum eta2 = eta * eta;
    Spectrum etak2 = etak * etak;
    
    Spectrum t0 = eta2 - etak2 - sinThetaI2;
    Spectrum a2plusb2 = Sqrt(t0 * t0 + 4 * eta2 * etak2);
    Spectrum t1 = a2plusb2 + cosThetaI2;
    Spectrum a = Sqrt(0.5f * (a2plusb2 + t0));
    Spectrum t2 = (Float)2 * cosThetaI * a;
    Spectrum Rs = (t1 - t2) / (t1 + t2);
    
    Spectrum t3 = cosThetaI2 * a2plusb2 + sinThetaI2 * sinThetaI2;
    Spectrum t4 = t2 * sinThetaI2;
    Spectrum Rp = Rs * (t3 - t4) / (t3 + t4);
    
    return 0.5 * (Rp + Rs);
}


// BXDF 实现
Spectrum BxDF::sample_f(const Vector3f &wo,
                        Vector3f *wi,
                        const Point2f &sample,
                        Float *pdf,
                        BxDFType *sampledType) const {
    *wi = cosineSampleHemisphere(sample);
    if (wo.z < 0) {
        wi->z *= -1;
    }
    *pdf = pdfDir(wo, *wi);
    return f(wo, *wi);
}

Spectrum BxDF::rho_hd(const Vector3f &wo, int nSamples, const Point2f *samples) const {
    Spectrum ret(0.0f);
    // ρhd(wo) = ∫[hemisphere]f(p,wi,wo)|cosθi|dwi
    // 蒙特卡洛方法采样估计积分值
    for (int i = 0; i < nSamples; ++i) {
        Vector3f wi;
        Float pdf = 0;
        Spectrum f = sample_f(wo, &wi, samples[i], &pdf);
        if (pdf > 0) {
            ret += f * absCosTheta(wi) / pdf;
        }
    }
    return ret / nSamples;
}

Spectrum BxDF::rho_hh(int nSamples, const Point2f *samplesWo, const Point2f *samplesWi) const {
    Spectrum ret(0.0f);
    // ρhh(wo) = (1/π)∫[hemisphere]∫[hemisphere]f(p,wi,wo)|cosθo * cosθi|dwidwo
    // 蒙特卡洛方法采样估计积分值
    for (int i = 0; i < nSamples; ++i) {
        Vector3f wo, wi;
        wo = uniformSampleHemisphere(samplesWo[i]);
        Float pdfo = uniformHemispherePdf();
        Float pdfi = 0;
        Spectrum r = sample_f(wo, &wi, samplesWi[i], &pdfi);
        if (pdfi > 0) {
            ret += r * absCosTheta(wi) * absCosTheta(wo) / (pdfi * pdfo);
        }
    }
    return ret / (Pi * nSamples);
}

Float BxDF::pdfDir(const Vector3f &wo, const Vector3f &wi) const {
    return sameHemisphere(wo, wi) ? absCosTheta(wi) * InvPi : 0;
}

// FresnelSpecular
Spectrum FresnelSpecular::sample_f(const Vector3f &wo, Vector3f *wi, const Point2f &u,
                  Float *pdf, BxDFType *sampledType) const {
    Float F = FrDielectric(cosTheta(wo), _etaA, _etaB);
    if (u[0] < F) {
        *wi = Vector3f(-wo.x, -wo.y, wo.z);
        if (sampledType)
            *sampledType = BxDFType(BSDF_SPECULAR | BSDF_REFLECTION);
        *pdf = F;
        return F * _R / absCosTheta(*wi);
    } else {
        bool entering = cosTheta(wo) > 0;
        Float etaI = entering ? _etaA : _etaB;
        Float etaT = entering ? _etaB : _etaA;
        
        if (!refract(wo, faceforward(Normal3f(0, 0, 1), wo), etaI / etaT, wi))
            return 0;
        Spectrum ft = _T * (1 - F);
        
        if (_mode == TransportMode::Radiance) {
            ft *= (etaI * etaI) / (etaT * etaT);
        }
        if (sampledType) {
            *sampledType = BxDFType(BSDF_SPECULAR | BSDF_TRANSMISSION);
        }
        *pdf = 1 - F;
        return ft / absCosTheta(*wi);
    }
}

std::string FresnelSpecular::toString() const {
    return std::string("[ FresnelSpecular R: ") + _R.ToString() +
    std::string(" T: ") + _T.ToString() +
    StringPrintf(" etaA: %f etaB: %f ", _etaA, _etaB) +
    std::string(" mode : ") +
    (_mode == TransportMode::Radiance ? std::string("RADIANCE")
     : std::string("IMPORTANCE")) +
    std::string(" ]");
}

// LambertianTransmission
//Spectrum LambertianTransmission::sample_f(const Vector3f &wo, Vector3f *wi, const Point2f &u,
//                          Float *pdf, BxDFType *sampledType) const {
//    *wi = cosineSampleHemisphere(u);
//    if (wo.z > 0) {
//        wi->z *= -1;
//    }
//    *pdf = pdfDir(wo, *wi);
//    return f(wo, *wi);
//}
//
//Float LambertianTransmission::pdfDir(const Vector3f &wo, const Vector3f &wi) const {
//    return sameHemisphere(wo, wi) ? 0 : absCosTheta(wi) * InvPi;
//}
//
//std::string LambertianTransmission::toString() const {
//    return std::string("[ LambertianTransmission T: ") + _T.ToString() +
//    std::string(" ]");
//}

// OrenNayar
Spectrum OrenNayar::f(const Vector3f &wo, const Vector3f &wi) const {
    Float sinThetaI = sinTheta(wi);
    Float sinThetaO = sinTheta(wo);
    // 计算max(0,cos(φi-φo))项
    // 由于三角函数耗时比较高，这里可以用三角恒等变换展开
    // cos(φi-φo) = cosφi * cosφo + sinφi * sinφo
    Float maxCos = 0;
    if (sinThetaI > 1e-4 && sinThetaO > 1e-4) {
        Float sinPhiI = sinPhi(wi), cosPhiI = cosPhi(wi);
        Float sinPhiO = sinPhi(wo), cosPhiO = cosPhi(wo);
        Float dCos = cosPhiI * cosPhiO + sinPhiI * sinPhiO;
        maxCos = std::max((Float)0, dCos);
    }
    
    Float sinAlpha, tanBeta;
    if (absCosTheta(wi) > absCosTheta(wo)) {
        sinAlpha = sinThetaO;
        tanBeta = sinThetaI / absCosTheta(wi);
    } else {
        sinAlpha = sinThetaI;
        tanBeta = sinThetaO / absCosTheta(wo);
    }
    return _R * InvPi * (_A + _B * maxCos * sinAlpha * tanBeta);
}

std::string OrenNayar::toString() const {
    return std::string("[ OrenNayar R: ") + _R.ToString() +
    StringPrintf(" A: %f B: %f ]", _A, _B);
}


// 傅里叶BSDF
Spectrum FourierBSDF::f(const Vector3f &wo, const Vector3f &wi) const {
    // 这里为什么是-wi？
    // 可能外部读取的数据默认是wi是从外部指向入射点的向量
    // 所以要取反
    Float muI = cosTheta(-wi);
    Float muO = cosTheta(wo);
    Float cosPhi = cosDPhi(-wi, wo);
    
    int offsetI, offsetO;
    Float weightsI[4], weightsO[4];
    if (!_bsdfTable.getWeightsAndOffset(muI, &offsetI, weightsI) ||
        !_bsdfTable.getWeightsAndOffset(muO, &offsetO, weightsO)) {
        return Spectrum(0.f);
    }
    
    Float *ak = ALLOCA(Float, _bsdfTable.mMax * _bsdfTable.nChannels);
    memset(ak, 0, _bsdfTable.mMax * _bsdfTable.nChannels * sizeof(Float));
    
    int mMax = 0;
    for (int b = 0; b < 4; ++b) {
        for (int a = 0; a < 4; ++a) {
            Float weight = weightsI[a] * weightsO[b];
            if (weight != 0) {
                int m;
                const Float *ap = _bsdfTable.getAk(offsetI + a, offsetO + b, &m);
                mMax = std::max(mMax, m);
                for (int c = 0; c < _bsdfTable.nChannels; ++c) {
                    for (int k = 0; k < mMax; ++k) {
                        ak[c * _bsdfTable.mMax + k] += weight * ap[c * m + k];
                    }
                }
            }
        }
    }
    
    Float Y = std::max((Float)0, Fourier(ak, mMax, cosPhi));
    // 8.21式中的|μi|
    Float scale = muI != 0 ? (1 / std::abs(muI)) : (Float)0;

    if (_mode == TransportMode::Radiance && muI * muO > 0) {
        // 经过折射之后有缩放
        float eta = muI > 0 ? 1 / _bsdfTable.eta : _bsdfTable.eta;
        scale *= eta * eta;
    }
    
    if (_bsdfTable.nChannels == 1)
        return Spectrum(Y * scale);
    else {
        Float R = Fourier(ak + 1 * _bsdfTable.mMax, mMax, cosPhi);
        Float B = Fourier(ak + 2 * _bsdfTable.mMax, mMax, cosPhi);
        Float G = 1.39829f * Y - 0.100913f * B - 0.297375f * R;
        Float rgb[3] = {R * scale, G * scale, B * scale};
        return Spectrum::FromRGB(rgb).clamp();
    }
}

Spectrum FourierBSDF::sample_f(const Vector3f &wo, Vector3f *wi, const Point2f &u,
                  Float *pdf, BxDFType *sampledType) const {
    return Spectrum(1.f);
}

Float FourierBSDF::pdfDir(const Vector3f &wo, const Vector3f &wi) const {
    return 0;
}

std::string FourierBSDF::toString() const {
    // todo
    return "";
}

bool FourierBSDFTable::getWeightsAndOffset(Float cosTheta, int *offset,
                                       Float weights[4]) const {
    return CatmullRomWeights(nMu, mu, cosTheta, offset, weights);
}

PALADIN_END
