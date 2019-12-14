//
//  bssrdf.hpp
//  Paladin
//
//  Created by SATAN_Z on 2019/10/21.
//

#ifndef bssrdf_hpp
#define bssrdf_hpp

#include "interaction.hpp"
#include "core/spectrum.hpp"
#include "core/bxdf.hpp"

PALADIN_BEGIN

/**
 * 参考资料
 * http://www.pbr-book.org/3ed-2018/Volume_Scattering/The_BSSRDF.html
 * 之前介绍了BRDF
 * 现在介绍一下BSSRDF
 * 两者有何区别？
 * 我们知道，漫反射是由于光子进入物体表面之后，随机散射到各个方向离开物体表面
 * 入射点跟出射点距离很近，可以简化模型，当做一个点处理，我们可以用BRDF来描述
 * 物体表面反射的分布
 * 但如果由于材质原因（半透明，例如，人的皮肤，蜡烛），入射点与与出射点的距离较远
 * 不能当做一个点来处理，则需要BSSRDF来表示了，废话不多说，先看表达式
 * 
 *     Lo(po,ωo) = ∫[A] ∫[H] S(po, ωo, pi, ωi) Li(pi, ωi) |cosθi| dωidA    1式
 * 
 * BSSRDF的表现形式为 S(po, ωo, pi, ωi)
 * BSSRDF是对边界上（通常是物体表面）
 * 给定的一对点和方向之间的这些散射过程的结果进行建模的总结表示。
 *  
 */
class BSSRDF {
public:
	BSSRDF(const SurfaceInteraction &po, Float eta) 
	: _po(po), 
	_eta(eta) {

	}

	virtual ~BSSRDF() {

	}

	virtual Spectrum S(const SurfaceInteraction &pi, const Vector3f &wi) = 0;

	virtual Spectrum sampleS(const Scene &scene, Float u1, const Point2f &u2,
                              MemoryArena &arena, SurfaceInteraction *si,
                              Float *pdf) const = 0;

protected:
	// 光线出射的位置，包含了出射方向 ωo
	const SurfaceInteraction &_po;
	// 折射率
	Float _eta;
};

// 菲涅尔函数一阶矩
// ∫[0,π/2]Fr(η, cosθi)sinθcosθdθ
Float FresnelMoment1(Float invEta);

// 菲涅尔函数二阶矩
Float FresnelMoment2(Float invEta);

struct BSSRDFTable;

class SeparableBSSRDFAdapter;

/**
 * 
 * BSSRDF的通用性比较強
 * 即使在比较简单的几何图形下求解光照传输的解决方案已经非常的困难了
 * 例如球体，平面等等，而BSSRDF可以附加到任何一个几何体上，导致求解
 * BSSRDF的积分是不现实的，接下来要介绍一种简单的表示
 * 
 * 我们可以做如下近似简化
 * 
 *     S(po, ωo, pi, ωi) ≈ (1 - Fr(cosθo)) Sp(po, pi) Sω(ωi)  2式
 * 
 * 先来分析一下Sω(ωi)函数，该函数可以认为是一个菲涅尔透射的缩放版本
 *
 *     Sω(ωi) = (1 - Fr(cosθi)) / cπ         3式
 *
 * 来看看分子部分，1 - Fr(cosθi)表示光子进入物体表面的部分
 * 分母部分为π是因为理想漫反射的brdf为1/π，c是常数，为了保证 ∫[H] Sω(ωi) cosθi dωi = 1
 * 
 * 将 3式带入 ∫[H] Sω(ω) cosθ dω = 1，可得
 *
 *     1 = ∫[H] ((1 - Fr(η, cosθ)) / cπ) cosθ dω
 *
 *     求解得到 c = 1 - 2 * ∫[0,π/2]Fr(η, cosθ)sinθcosθdθ
 *
 * 式中的积分被称为菲涅尔函数的一阶矩（个人理解这应该是近似函数）
 * 也有其他涉及余弦函数高次幂的矩，并且经常出现在与次表面散射相关的计算中，
 * 更一般的表示方法，菲涅尔的第i阶矩：
 * _
 * Fr(η,i) = ∫[0,π/2] Fr(η, cosθ) sinθ (cosθ)^i dθ
 *
 * pbrt提供了FresnelMoment1()，FresnelMoment2()
 * 两个函数用多项式的方式去拟合菲涅尔的一阶矩跟二阶矩，但有个微妙的地方是
 * 这两个函数接受了eta的倒数
 *
 * 接下来分析Sp(po, pi)项，我们假设表面不仅局部平坦，而且任意等距离的两点的Sp函数相同
 * 
 *                Sp(po, pi) ≈ Sr(|po - pi|)
 *
 * SeparableBSSRDF 就做了如上的近似简化
 * 
 */
class SeparableBSSRDF : public BSSRDF {
public:
	SeparableBSSRDF(const SurfaceInteraction &po, Float eta,
					const Material *material, TransportMode mode)
	: BSSRDF(po, eta),
	_sNormal(po.shading.normal),
	_material(material),
	_sTangent(normalize(po.shading.dpdu)),
	_tTangent(cross(_sNormal, _sTangent)),
	_mode(mode){

	}

	// S(po, ωo, pi, ωi) ≈ (1 - Fr(cosθo)) Sp(po, pi) Sω(ωi)
	Spectrum S(const SurfaceInteraction &pi, const Vector3f &wi) {
		// todo 这里的折射率应该要传入，待优化
		Float Ft = FrDielectric(cosTheta(_po.wo), 1, _eta);
		return (1 - Ft) * Sp(pi) * Sw(wi);
	}

	// Sω(ωi) = (1 - Fr(cosθi)) / cπ
	// c = 1 - 2 * ∫[0,π/2]Fr(η, cosθi)sinθcosθdθ
	Spectrum Sw(const Vector3f &w) const {
		Float c = 1 - 2 * FresnelMoment1(1 / _eta);
		return (1 - FrDielectric(cosTheta(w), 1, _eta)) / (c * Pi);
	}

	// Sp(po, pi) ≈ Sr(|po - pi|)
	Spectrum Sp(const SurfaceInteraction &pi) const {
		return Sr(distance(_po.pos, pi.pos));
	}

	Spectrum sample_S(const Scene &scene, Float u1, const Point2f &u2,
                      MemoryArena &arena, SurfaceInteraction *si,
                      Float *pdf) const;

	Spectrum sample_Sp(const Scene &scene, Float u1, const Point2f &u2,
                       MemoryArena &arena, SurfaceInteraction *si,
                       Float *pdf) const;

	Float pdf_Sp(const SurfaceInteraction &si) const;

	virtual Spectrum Sr(Float d) const = 0;

	virtual Float sample_Sr(int ch, Float u) const = 0;

    virtual Float pdf_Sr(int ch, Float r) const = 0;

protected:
	// 着色法线
    const Normal3f _sNormal;
    // 着色切线(s方向，u方向)
    const Vector3f _sTangent;
    // 着色切线(t方向，v方向)
    const Vector3f _tTangent;
    // 材质
    const Material * _material;
    // 光照传输模式
    const TransportMode _mode;
    
    friend class SeparableBSSRDFAdapter;
	
};


/**
 * tabulated BSSRDF，在我理解为制成表格的bssrdf，该形式可以处理各种散射曲线
 * 包括实际测量的bssrdf
 * 
 * 先来介绍一下Sr函数
 * 注意：当材质属性固定时，Sr只是一个1D函数，更一般的来看Sr还有其他四个参数
 * 折射率η，散射各向异性系数g，反射率ρ，衰减系数σt，组合成一个完整的函数
 * Sr(η,g,ρ,σt,r)，不幸的是，这个函数对于离散化而言，维度太高了。
 * 上述参数中，除了r，只有σt是有物理单位的，这个参数量化了单位距离内的吸收率以及散射率
 * 这个参数比较简单，它仅仅控制了bssrdf的空间轮廓比例，为了降低函数的维度，我们可以把它固定为1
 * 并且制作一个无单位版本的bssrdf的外形轮廓。
 *
 * 在运行时，我们查找一个给定消光系数σt，半径r，我们找到一个无单位光学半径r_optical = σt r,
 * 计算更低维度的表格，表达式如下：
 *
 *               Sr(σt,r) = σt^2 Sr(1, σt r)
 * 
 *               Sr(η,g,ρ,σt,r) = σt^2 Sr(η,g,ρ,1,r_optical)
 *
 * 为何？因为Sr在极坐标(r,θ)空间中是一个2D密度函数，需要一个相应的比例因子来解释变量的变化
 *      p(r,θ) = r p(x,y)  (sampling.hpp文件8式)
 *
 * 握草，这个表达式有点难搞，先跳过todo
 *
 * 其实bssrdf的最终解决方案还是查表
 *
 * 我们也将固定折射率η，以及各向异性系数g这两个参数，这意味着使用TabulatedBSSRDF材质的
 * 对象不能用纹理来储存这两个参数
 *
 * 简化之后剩下一个可管理性很强的2d函数
 * 可以通过反射率ρ，光学半径r_optical，实现离散化
 * 用以上两个物理量作为Sr表的两个维度，为了更准确地表示基本函数，也经过了科学家们的测试
 * 光学半径跟反射率的采样间距通常是不均匀的
 * 
 * 反射率ρ，确定了单个散射事件之后的能量剩余量
 * 这与材质整体的反射率不同
 * 材质整体的反射率考虑了所有的散射顺序
 * 为了强调这种差别，我们将把这些不同类型的反照率称为单散射反射率ρ和有效反射率ρeff
 *
 * 我们定义有效反射率如下所示
 *
 * ρeff = ∫[0,2π]∫[0,∞] r Sr(r) dr dφ = 2π ∫[0,∞] r Sr(r) dr   (这里的r指的是光学半径)
 *
 * KdSubsurfaceMaterial材质与Sr采样代码都会频繁的访问ρeff的值
 *
 * ρeff是ρ的严格单调递增函数
 *
 * 给定半径值r和单次散射反照率，函数Sr()实现了对表格配置文件的样条插值查找。
 * 
 * BSSRDFTable的注释中说明了如何初始化
 */
class TabulatedBSSRDF : public SeparableBSSRDF {
    
public:
    TabulatedBSSRDF(const SurfaceInteraction &po, const Material *material,
                    TransportMode mode, Float eta, const Spectrum &sigma_a,
                    const Spectrum &sigma_s, const BSSRDFTable &table)
    : SeparableBSSRDF(po, eta, material, mode),
    table(table) {
        // σt = σa + σs
        // ρ = σs / σt
        _sigma_t = sigma_a + sigma_s;
        for (int ch = 0; ch < Spectrum::nSamples; ++ch) {
            _rho[ch] = _sigma_t[ch] != 0 ? (sigma_s[ch] / _sigma_t[ch]) : 0;
        }
    }
    
    virtual Spectrum Sr(Float r) const override;
    
    virtual Float pdf_Sr(int ch, Float distance) const override;
    
    virtual Float sample_Sr(int ch, Float sample) const override;
    
private:
    // 
    const BSSRDFTable &table;
    // 衰减系数
    Spectrum _sigma_t;
    // 反射率，确定了单个散射事件之后的能量剩余量
    // 这与材质整体的反射率不同
    // 材质整体的反射率考虑了所有的散射顺序
    // 为了强调这种差别，我们将把这些不同类型的反照率称为单散射反射率ρ和有效反射率ρeff
    Spectrum _rho;
};


/**
 * 参考章节
 * http://www.pbr-book.org/3ed-2018/Light_Transport_II_Volume_Rendering/Subsurface_Scattering_Using_the_Diffusion_Equation.html#sec:filling-in-bssrdftable
 * Sr函数由两个参数
 * r_optical,ρeff
 * 实际上Sr函数是通过查表来实现，
 * 这是一张二维表，两个维度依次为
 * r1,r2,r3....
 * ρ1,ρ2,ρ3....
 * 二维数组，你懂的，因为二维表是离散的，如果要采样两个元素之间的函数值，则需要通过插值
 * 使用的就是catmull-rom插值
 * 
 * 首先来介绍一下如何初始化这个二维表
 *
 * 在之前已经介绍过，Sr函数已经被我们降低了维度，固定了两个参数
 * 各向异性系数g，折射率η
 * 并把σt跟r整合成了一个参数r_optical，光学半径
 * 
 * 因此Sr函数目前接受两个参数，r_optical,ρeff
 * 但Sr函数值还是会受到各向异性系数g，折射率η这两个参数的影响
 * 怎么实现呢？很简单。
 * 
 * 我们让生成的二维表的元素与各向异性系数g，折射率η有关就好了
 * computeBeamDiffusionBSSRDF 函数接收以上两个输入参数，还有一个用于输出的参数
 *
 * Sr(r_optical)函数是随着r_optical指数衰减 （这还是比较容易理解的）
 * 那么在采样的时候就应该是在函数值比较大的地方，样本间隔应该小一些，
 * 函数值小的地方，样本间隔可以大一些
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 *
 */
struct BSSRDFTable {
    // 反射率采样数量
    const int nRhoSamples;
    // 半径采样数量
    const int nRadiusSamples;
    // 反射率样本列表
    std::unique_ptr<Float[]> rhoSamples;
    // 光学半径样本列表
    std::unique_ptr<Float[]> radiusSamples;
    // Sr函数表
    std::unique_ptr<Float[]> profile;
    // 有效反射率，映射到对应的Sr函数值
    std::unique_ptr<Float[]> rhoEff;
    // Sr函数表的累积分布函数
    std::unique_ptr<Float[]> profileCDF;

    BSSRDFTable(int nRhoSamples, int nRadiusSamples);
    
    inline Float evalProfile(int rhoIndex, int radiusIndex) const {
        return profile[rhoIndex * nRadiusSamples + radiusIndex];
    }
};

class SeparableBSSRDFAdapter : public BxDF {
public:
    // SeparableBSSRDFAdapter Public Methods
    SeparableBSSRDFAdapter(const SeparableBSSRDF *bssrdf)
    : BxDF(BxDFType(BSDF_REFLECTION | BSDF_DIFFUSE)),
    _bssrdf(bssrdf) {
        
    }
    
    Spectrum f(const Vector3f &wo, const Vector3f &wi) const {
        Spectrum f = _bssrdf->Sw(wi);
        if (_bssrdf->_mode == TransportMode::Radiance) {
            f *= _bssrdf->_eta * _bssrdf->_eta;
        }
        return f;
    }
    
    std::string toString() const {
        return "[ SeparableBSSRDFAdapter ]";
    }

private:
    const SeparableBSSRDF * _bssrdf;
};

Float beamDiffusionSS(Float sigma_s, Float sigma_a, Float g, Float eta,
                      Float r);

Float beamDiffusionMS(Float sigma_s, Float sigma_a, Float g, Float eta,
                      Float r);

void computeBeamDiffusionBSSRDF(Float g, Float eta, BSSRDFTable *t);

void subsurfaceFromDiffuse(const BSSRDFTable &table, const Spectrum &rhoEff,
                           const Spectrum &mfp, Spectrum *sigma_a,
                           Spectrum *sigma_s);

PALADIN_END

#endif /* bssrdf_hpp */
