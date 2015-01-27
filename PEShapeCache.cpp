//
//  PEShapeCache.cpp
//
//
//  Created by Goodia on 23/01/2015.
//
//

#include "PEShapeCache.h"

#include "chipmunk.h"
#include "chipmunk/CCPhysicsHelper_chipmunk.h"

USING_NS_CC;

typedef enum
{
    SHAPE_POLYGON,
    SHAPE_CIRCLE
} PEShapeType;

class Polygon : public Ref
{
public:
    virtual ~Polygon(){
        CC_SAFE_DELETE_ARRAY(vertices);
    }
    
    Point* vertices;
    int numVertices;
    float area;
    float mass;
    float momentum;
};

class FixtureData : public Ref
{
public:
    PEShapeType fixtureType;
    float mass;
    float elasticity;
    float friction;
    Point surfaceVelocity;
    int collisionType;
    int group;
    int layers;
    float area;
    float momentum;
    bool isSensor;
    //for circles
    Point center;
    float radius;
    //for polygons
    Vector<Polygon*> polygons;
};

class CPBodyDef : public Ref
{
public:
    CPBodyDef() : Ref()
    {}
    
    Point anchorPoint;
    Vector<FixtureData*> fixtures;
    float mass;
    float momentum;
};

static float area(Point *vertices, int numVertices)
{
    float area = 0.0f;
    int r = (numVertices - 1);
    area += vertices[0].x * vertices[r].y - vertices[r].x * vertices[0].y;
    for (int i = 0; i < numVertices - 1; ++i)
    {
        area += vertices[r - i].x * vertices[r - (i + 1)].y - vertices[r - (i + 1)].x * vertices[r - i].y;
    }
    area *= .5f;
    return area;
}

static PEShapeCache *_instance = nullptr;

PEShapeCache::PEShapeCache()
{
}

bool PEShapeCache::init()
{
    return true;
}

PEShapeCache::~PEShapeCache()
{
    removeAllBodys();
}

PEShapeCache *PEShapeCache::getInstance()
{
    if (!_instance)
    {
        _instance = new PEShapeCache();
        _instance->init();
    }
    return _instance;
}

void PEShapeCache::destroyInstance()
{
    CC_SAFE_DELETE(_instance);
    _instance = nullptr;
}

void PEShapeCache::addBodysWithFile(const std::string &plist)
{
    ValueMap dict = FileUtils::getInstance()->getValueMapFromFile(plist);
    CCASSERT(!dict.empty(), "Shape-file not found");
    CCASSERT(dict.size() != 0, "plist file empty or not existing");
    
    ValueMap &metadata = dict["metadata"].asValueMap();
    int format = metadata["format"].asInt();
    CCASSERT(format == 1, "format not supported!");
    
    auto scale = Director::getInstance()->getContentScaleFactor();
    
    ValueMap &bodydict = dict.at("bodies").asValueMap();
    for (auto iter = bodydict.cbegin(); iter != bodydict.cend(); ++iter)
    {
        const ValueMap &bodyData = iter->second.asValueMap();
        std::string bodyName = iter->first;
        
        CPBodyDef *bodyDef = new CPBodyDef();
        bodyDefs.insert(bodyName, bodyDef);
        bodyDef->release();
        
        bodyDef->anchorPoint = PointFromString(bodyData.at("anchorpoint").asString());
        const ValueVector &fixtureList = bodyData.at("fixtures").asValueVector();
        float totalMass = 0.0f;
        float totalBodyMomentum = 0.0f;
        
        for (auto &fixtureitem : fixtureList)
        {
            FixtureData *fd = new FixtureData();
            bodyDef->fixtures.pushBack(fd);
            fd->release();
            
            auto &fixturedata = fixtureitem.asValueMap();
           
            fd->friction = fixturedata.at("friction").asFloat();
            fd->elasticity = fixturedata.at("elasticity").asFloat();
            fd->mass = fixturedata.at("mass").asFloat();
            fd->surfaceVelocity = PointFromString(fixturedata.at("surface_velocity").asString());
            fd->layers = fixturedata.at("layers").asInt();
            fd->group = fixturedata.at("group").asInt();
            fd->collisionType = fixturedata.at("collision_type").asInt();
            fd->isSensor = fixturedata.at("isSensor").asBool();
            
            std::string fixtureType = fixturedata.at("fixture_type").asString();
            float totalArea = 0.0f;
            totalMass += fd->mass;
            if (strcmp("POLYGON", fixtureType.c_str()) == 0)
            {
                const ValueVector &polygonsArray = fixturedata.at("polygons").asValueVector();
                fd->fixtureType = SHAPE_POLYGON;
                for (auto &polygonitem : polygonsArray)
                {
                    Polygon *poly = new Polygon();
                    fd->polygons.pushBack(poly);
                    poly->release();
                    
                    auto &polygonArray = polygonitem.asValueVector();
                    poly->numVertices = polygonArray.size();
                    Point *vertices = new Point[poly->numVertices];
                    int vindex = 0;
                    for (auto &pointString : polygonArray)
                    {
                        Point offsex = PointFromString(pointString.asString());
                        vertices[vindex].x = offsex.x/scale;
                        vertices[vindex].y = offsex.y/scale;
                        vindex++;
                    }
                    poly->vertices = vertices;
                    poly->area = area(vertices, poly->numVertices);
                    totalArea += poly->area;
                }
            }
            else if (strcmp("CIRCLE", fixtureType.c_str()) == 0)
            {
                fd->fixtureType = SHAPE_CIRCLE;
                const ValueMap &circleData = fixturedata.at("circle").asValueMap();
                fd->radius = circleData.at("radius").asFloat()/scale;
                fd->center = PointFromString(circleData.at("position").asString())/scale;
                totalArea += M_PI * fd->radius * fd->radius;
            }
            else
            {
                // unknown type
                CCASSERT(false, "Unknown type");
            }
            fd->area = totalArea;
            // update sub polygon's masses and momentum
            cpFloat totalFixtureMomentum = 0.0f;
            if (totalArea)
            {
                if (fd->fixtureType == SHAPE_CIRCLE)
                {
                    totalFixtureMomentum += cpMomentForCircle(PhysicsHelper::float2cpfloat(fd->mass),
                                                              PhysicsHelper::float2cpfloat(fd->radius),
                                                              PhysicsHelper::float2cpfloat(fd->radius),
                                                              PhysicsHelper::point2cpv(fd->center));
                }
                else
                {
                    for (auto *p : fd->polygons)
                    {
                        // update mass
                        p->mass = (p->area * fd->mass) / fd->area;
                        cpVect *cpvs = new cpVect[p->numVertices];
                        // calculate momentum
                        p->momentum = cpMomentForPoly(PhysicsHelper::float2cpfloat(p->mass),
                                                      p->numVertices,
                                                      PhysicsHelper::points2cpvs(p->vertices, cpvs, p->numVertices),
                                                      PhysicsHelper::point2cpv(Point::ZERO));
                        delete[] cpvs;
                        // calculate total momentum
                        totalFixtureMomentum += p->momentum;
                    }
                }
            }
            
            fd->momentum = PhysicsHelper::cpfloat2float(totalFixtureMomentum);
            totalBodyMomentum = PhysicsHelper::cpfloat2float(totalFixtureMomentum);
        }
        
        // set bodies total mass
        bodyDef->mass = totalMass;
        bodyDef->momentum = totalBodyMomentum;
    }
}

PhysicsBody *PEShapeCache::getPhysicsBodyByName(const std::string& name)
{
    CPBodyDef *bd = bodyDefs.at(name);
    CCASSERT(bd != nullptr, "Body not found");
    PhysicsBody *body = PhysicsBody::create(bd->mass, bd->momentum);
    body->setPositionOffset(bd->anchorPoint);
    for (auto fd : bd->fixtures)
    {
        if (fd->fixtureType == SHAPE_CIRCLE)
        {
            auto shape = PhysicsShapeCircle::create(fd->radius,
                                                    PhysicsMaterial(0.0f, fd->elasticity, fd->friction),
                                                    fd->center);
            shape->setGroup(fd->group);
            shape->setCategoryBitmask(fd->collisionType);
            body->addShape(shape);
        }
        else if (fd->fixtureType == SHAPE_POLYGON)
        {
            for (auto polygon : fd->polygons)
            {
                auto shape = PhysicsShapePolygon::create(polygon->vertices,
                                                         polygon->numVertices,
                                                         PhysicsMaterial(0.0f, fd->elasticity, fd->friction),
                                                         fd->center);
                shape->setGroup(fd->group);
                shape->setCategoryBitmask(fd->collisionType);
                body->addShape(shape);
            }
        }
    }
    
    return body;
}

bool PEShapeCache::removeBodysWithFile(const std::string &plist)
{
    ValueMap dict = FileUtils::getInstance()->getValueMapFromFile(plist);
    CCASSERT(!dict.empty(), "Shape-file not found");
    CCASSERT(dict.size() != 0, "plist file empty or not existing");
    
    ValueMap &metadata = dict["metadata"].asValueMap();
    int format = metadata["format"].asInt();
    CCASSERT(format == 1, "format not supported!");
    
    ValueMap &bodydict = dict.at("bodies").asValueMap();
    for (auto iter = bodydict.cbegin(); iter != bodydict.cend(); ++iter)
    {
        std::string bodyName = iter->first;
        auto bdIte = bodyDefs.find(bodyName);
        if(bdIte != bodyDefs.end()){
            bodyDefs.erase(bdIte);
        }
    }
    
    return true;
}

bool PEShapeCache::removeAllBodys()
{
    CCLOG("%s"," PEShapeCache removeAllbodys");
    bodyDefs.clear();
    return true;
}
