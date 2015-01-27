//
//  PEShapeCache.h
//
//
//  Created by Goodia on 23/01/2015.
//
//

#ifndef __PEShapeCache__
#define __PEShapeCache__

#include "cocos2d.h"

class CPBodyDef;

/** 
 *  a Instance which
 *  1. Load physic body from PhysicsEditor plist file
 *  2. Cache the body info data, so it load only 1 times.
 */
class PEShapeCache 
{
protected:
    virtual bool init();
    
public:
    PEShapeCache(void);
    virtual ~PEShapeCache(void);
    
    static PEShapeCache *getInstance();
    static void destroyInstance();
    
    /**
     *  load bodys from plist (which export from PhysicEditor
     *  @param plist : plist file name
     */
    void addBodysWithFile(const std::string &plist);
   
    /**
     *  get body with name
     *  @param name : name of physics body
     *  @return PhysicsBody
     */
    cocos2d::PhysicsBody *getPhysicsBodyByName(const std::string& name);
   
    /**
     *  remove physics bodys from plist file
     *  @param plist : plist file name
     */
    bool removeBodysWithFile(const std::string &plist);
  
    /**
     *  clear all bodys cache
     */
    bool removeAllBodys();
    
private:
    cocos2d::Map<std::string, CPBodyDef*> bodyDefs;

};

#endif /* defined(__PEShapeCache__) */
