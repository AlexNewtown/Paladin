//
//  meshparser.hpp
//  Paladin
//
//  Created by SATAN_Z on 2020/3/1.
//

#ifndef meshparser_hpp
#define meshparser_hpp

#include "shapes/trianglemesh.hpp"

PALADIN_BEGIN

struct SurfaceData {
    shared_ptr<const Material> material;
    Float emission[3];
};


/**
 * 为了支持unity场景导出的文件
 * 文件中有mesh格式的数据，包含顶点列表，索引列表
 * 该类负责直接解析mesh数据，创建primitive列表
 */
class TriangleParser {
    
public:
    
    void load(const nloJson &data);
    
    vector<shared_ptr<Primitive>> getPrimitiveLst();
    
private:
    // 顶点列表
    vector<Point3f> _points;
    // 边向量列表
    vector<Vector3f> _edges;
    // 法线列表
    vector<Normal3f> _normals;
    // uv坐标列表
    vector<Point2f> _UVs;
    // 面索引列表
    vector<int> _faceIndices;
    
    // 顶点索引列表
    vector<int> _verts;
    // 材质列表
    vector<SurfaceData> _materialLst;
    // 材质索引列表
    vector<int> _matIndices;
    
};

PALADIN_END

#endif /* meshparser_hpp */