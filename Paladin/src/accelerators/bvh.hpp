//
//  bvh.hpp
//  Paladin
//
//  Created by SATAN_Z on 2019/6/30.
//  Copyright © 2019 Zero. All rights reserved.
//

#ifndef bvh_hpp
#define bvh_hpp

#include "core/header.h"
#include "core/aggregate.hpp"
#include "core/shape.hpp"
PALADIN_BEGIN

//代表一个图元的部分信息
struct BVHPrimitiveInfo {
    BVHPrimitiveInfo() {}
    BVHPrimitiveInfo(size_t primitiveNumber, const AABB3f &bounds)
    : primitiveNumber(primitiveNumber),
    bounds(bounds),
    centroid(.5f * bounds.pMin + .5f * bounds.pMax) {}
    size_t primitiveNumber;
    AABB3f bounds;
    Point3f centroid;
};

// bvh可以理解为一个二叉树
// 这是构建二叉树时的节点对象
// 构建完毕之后会转成连续内存的储存方式
struct BVHBuildNode {
    void initLeaf(int first, int n, const AABB3f &b) {
        firstPrimOffset = first;
        nPrimitives = n;
        bounds = b;
        children[0] = children[1] = nullptr;
    }
    void initInterior(int axis, BVHBuildNode *c0, BVHBuildNode *c1) {
        children[0] = c0;
        children[1] = c1;
        bounds = unionSet(c0->bounds, c1->bounds);
        splitAxis = axis;
        nPrimitives = 0;
    }
    AABB3f bounds;
    // 二叉树的两个子节点
    BVHBuildNode *children[2];
    // 分割的坐标轴
    int splitAxis;
    // 第一个片元的偏移量
    int firstPrimOffset;
    // 片元数量
    int nPrimitives;
};

// 莫顿码片元
struct MortonPrimitive {
    int primitiveIndex;
    uint32_t mortonCode;
};

// 用于lbvh
struct LBVHTreelet {
    int startIndex, nPrimitives;
    BVHBuildNode *buildNodes;
};

/*
 在内存中的一个线性BVH节点
 布局如下
     A
    / \
   B   C
  / \
 D   E

 线性顺序为 A B D E C
 一个节点的第一个子节点紧接在该节点的后面
 */
struct LinearBVHNode {
    AABB3f bounds;
    union {
        int primitivesOffset;   //指向图元
        int secondChildOffset;  // 第二个子节点在数组中的偏移量
    };
    uint16_t nPrimitives;  // 图元数量
    uint8_t axis;          // interior node: xyz
    uint8_t pad[1];        // 确保32个字节为一个对象，提高缓存命中率
};


/*
 根据对象划分
 */
class BVHAccel : public Aggregate {
public:
    enum SplitMethod { SAH, HLBVH, Middle, EqualCounts };
    
    BVHAccel(const vector<shared_ptr<const Shape>> &shapes,
             int maxPrimInNode = 1,
             SplitMethod splitMethod = SplitMethod::SAH);
    
    virtual AABB3f worldBound() const override;
    
    virtual nloJson toJson() const override {
        return nloJson();
    }
    
    virtual ~BVHAccel();
    
    virtual bool rayIntersect(const Ray &ray, SurfaceInteraction *isect) const override;
    
    virtual bool rayOccluded(const Ray &ray) const override;
    
private:
    
    BVHBuildNode * recursiveBuild(MemoryArena &arena, std::vector<BVHPrimitiveInfo> &primitiveInfo,
                                  int start, int end, int *totalNodes,
                                  vector<const Primitive *> &orderedPrims);
    
    int flattenBVHTree(BVHBuildNode *node, int *offset);
    
    const int _maxPrimsInNode;
    
    const SplitMethod _splitMethod;
    
    LinearBVHNode *_nodes = nullptr;
    
    vector<shared_ptr<const Shape>> _shapes;
    
    vector<const Primitive *> _prims;
};

shared_ptr<BVHAccel> createBVH(const nloJson &param, const vector<shared_ptr<const Shape>> &shapes);

PALADIN_END

#endif /* bvh_hpp */
