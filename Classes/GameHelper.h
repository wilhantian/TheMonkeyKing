//
//  GameHelper.h
//  TheMonkeyKing
//
//  Created by wilhan on 18/1/11.
//
//

#ifndef GameHelper_h
#define GameHelper_h

#include "cocos2d.h"
#include "Box2D/Box2D.h"
#include "entityx/entityx.h"

#include "config.h"

class GameHelper
{
public:
    static void init(b2World* world);
    
    static std::set<entityx::Entity> queryEntities(cocos2d::Rect rect);
    
public:
    static b2World* world;
    
private:
    class EntityQueryCallback : public b2QueryCallback
    {
    public:
        virtual bool ReportFixture(b2Fixture* fixture);
        
        std::set<entityx::Entity> entities;
    };
};

#endif /* GameHelper_h */
