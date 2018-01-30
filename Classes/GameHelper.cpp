//
//  GameHelper.cpp
//  TheMonkeyKing
//
//  Created by wilhan on 18/1/11.
//
//

#include "GameHelper.h"
#include "GameWorld.h"

USING_NS_CC;
using namespace entityx;

void GameHelper::init(b2World *world)
{
    GameHelper::world = world;
}

std::set<Entity> GameHelper::queryEntities(Rect rect)
{
    if(!world)
    {
        return std::set<Entity>();
    }
    
    EntityQueryCallback callback;
    
    b2AABB aabb;
    aabb.lowerBound.Set(rect.getMinX() / PTM_RATIO, rect.getMinY() / PTM_RATIO);
    aabb.upperBound.Set(rect.getMaxX() / PTM_RATIO, rect.getMaxX() / PTM_RATIO);
    
    world->QueryAABB(&callback, aabb);
    
    return callback.entities;
}

b2World* GameHelper::world = nullptr;

/////////////////////////////////////////////////////////
bool GameHelper::EntityQueryCallback::ReportFixture(b2Fixture *fixture)
{
    entityx::Entity entity{*static_cast<entityx::Entity*>(fixture->GetBody()->GetUserData())};
    
    if(entity.valid())
    {
        entities.insert(entity);
    }
    return true;
}