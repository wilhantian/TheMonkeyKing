//
//  GameWorld.cpp
//  TheMonkeyKing
//
//  Created by wilhan on 17/12/22.
//
//

#include "GameWorld.h"

#include "GameHelper.h"

USING_NS_CC;

void RenderSystem::configure(EventManager &events)
{
    events.subscribe<ComponentAddedEvent<SpriteComp>>(*this);
    events.subscribe<ComponentRemovedEvent<SpriteComp>>(*this);
}

void RenderSystem::receive(const ComponentAddedEvent<SpriteComp>& eve)
{
    ComponentHandle<SpriteComp> sprite = eve.component;
    if(sprite)
    {
        //TODO
    }
}

void RenderSystem::receive(const ComponentRemovedEvent<SpriteComp>& eve)
{
    ComponentHandle<SpriteComp> sprite = eve.component;
    if(sprite && sprite->sprite)
    {
        sprite->sprite->removeFromParent();
        sprite->sprite = nullptr;
    }
}

void RenderSystem::update(EntityManager &es, EventManager &events, TimeDelta dt)
{
    es.each<SpriteComp, PositionComp>([=](Entity e, SpriteComp& sprite, PositionComp& pos)
    {
        if(!sprite.sprite)
        {
            return;
        }
        
        // 位置
        sprite.sprite->setPosition(pos.x, pos.y);
        
        // 动画
        auto animate = e.component<AnimateComp>();
        if(animate && !animate->frame.empty())
        {
            sprite.sprite->setTexture(animate->frame);
        }
    });
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void AnimateSystem::configure(EventManager &events)
{
}

void AnimateSystem::update(entityx::EntityManager &es, entityx::EventManager &events, TimeDelta dt)
{
    es.each<AnimateComp, SpriteComp>([=](Entity e, AnimateComp& animate, SpriteComp& sprite)
    {
        animate.onceEnd = false;
        if(animate.paused)
        {
            return;
        }
        
        animate.curDuration += dt;
        
        if(animate.curDuration >= animate.speed)
        {
            animate.curDuration = 0.0f;
            
            if(++animate.index == animate.size)
            {
                animate.index = 0;
                animate.onceEnd = true;
                
                if(!animate.loop)
                {
                    animate.paused = true;
                }
            }
            
            // 改变帧
            char buf[128];
            sprintf(buf, animate.filename.c_str(), animate.index);
            animate.frame = buf;
        }
        else if(animate.initFirstFrame)//立即更新生效
        {
            char buf[128];
            sprintf(buf, animate.filename.c_str(), animate.index);
            animate.frame = buf;
            animate.initFirstFrame = false;
        }
    });
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void PhysicsSystem::configure(EventManager &events)
{
    __world = new b2World(b2Vec2(0, 0));
    __world->SetAllowSleeping(true);
    __world->SetContinuousPhysics(true);
    __world->SetContactListener(this);
    
    __debugDraw = new GLESDebugDraw(PTM_RATIO);
    uint32 flags = 0;
    flags += b2Draw::e_shapeBit;
    flags += b2Draw::e_jointBit;
    flags += b2Draw::e_aabbBit;
    flags += b2Draw::e_pairBit;
    flags += b2Draw::e_centerOfMassBit;
    __debugDraw->SetFlags(flags);
    __world->SetDebugDraw(__debugDraw);
    
    events.subscribe<ComponentAddedEvent<PhysicsComp>>(*this);
    events.subscribe<ComponentRemovedEvent<PhysicsComp>>(*this);
}

void PhysicsSystem::receive(const ComponentAddedEvent<PhysicsComp>& eve)
{
    ComponentHandle<PhysicsComp> physics = eve.component;
    
    if(physics->body)
    {
        physics->self = eve.entity;
        physics->body->SetUserData(&physics->self);
    }
}

void PhysicsSystem::receive(const ComponentRemovedEvent<PhysicsComp>& eve)
{
    ComponentHandle<PhysicsComp> physics = eve.component;
    if(physics->body)
    {
        __world->DestroyBody(physics->body);
    }
}

void PhysicsSystem::update(EntityManager &es, EventManager &events, TimeDelta dt)
{
    int velocityIterations = 8;
    int positionIterations = 1;
    
    es.each<PhysicsComp>([=](Entity e, PhysicsComp& physics)
    {
        // 更新速度
        auto velocity = e.component<VelocityComp>();
        auto position = e.component<PositionComp>();
        if(velocity && position)
        {
            if(physics.body)
            {
                physics.body->SetTransform(b2Vec2(position->x / PTM_RATIO, position->y / PTM_RATIO), physics.body->GetAngle());
                physics.body->SetLinearVelocity(b2Vec2(velocity->x, velocity->y));
            }
        }
    });
    
    // 更新物理世界
    __world->Step(dt, velocityIterations, positionIterations);
    
    // 分发碰撞事件
    for(auto eve : __collisionEvents)
    {
        events.emit(eve);
    }
    __collisionEvents.clear();
    
    es.each<PhysicsComp>([=](Entity e, PhysicsComp& physics)
    {
        // 更新位置
        auto position = e.component<PositionComp>();
        if(position)
        {
            if(physics.body)
            {
                auto bodyPos = physics.body->GetPosition();
                position->x = bodyPos.x * PTM_RATIO;
                position->y = bodyPos.y * PTM_RATIO;
            }
        }
    });
}

void PhysicsSystem::BeginContact(b2Contact *contact)
{
    auto a = contact->GetFixtureA();
    auto b = contact->GetFixtureB();
    auto ea = *(Entity*)a->GetBody()->GetUserData();
    auto eb = *(Entity*)b->GetBody()->GetUserData();
    
    __collisionEvents.push_back(EntityCollisionEvent(ea, eb));
}

void PhysicsSystem::EndContact(b2Contact *contact)
{
}

b2World* PhysicsSystem::getB2World()
{
    return __world;
}

b2Draw* PhysicsSystem::getB2DebugDraw()
{
    return __debugDraw;
}

std::vector<Entity*> PhysicsSystem::queryAABB(Rect rect)
{
    AABBQueryCallback query;
    
    b2AABB aabb;
    aabb.lowerBound.Set(rect.getMinX() / PTM_RATIO, rect.getMinY() / PTM_RATIO);
    aabb.upperBound.Set(rect.getMaxX() / PTM_RATIO, rect.getMaxY() / PTM_RATIO);
    
    __world->QueryAABB(&query, aabb);
    
    return query.entitys;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void ActorSystem::configure(EventManager &event_manager)
{
    event_manager.subscribe<ActorActionChangedEvent>(*this);
    event_manager.subscribe<ActorDirectionChangedEvent>(*this);
    event_manager.subscribe<EntityCollisionEvent>(*this);
    event_manager.subscribe<DiedEvent>(*this);
}

void ActorSystem::receive(const ActorActionChangedEvent& eve)
{
    auto e = eve.e;
    auto actor = e.component<ActorComp>();

    if(actor && actor->action != eve.action)
    {
        actor->action = eve.action;
        
        CCLOG("actor->action = %d", actor->action);
        
        changeActorAnimate(eve.e);
        changeActorVelocity(eve.e);
    }
}

void ActorSystem::receive(const ActorDirectionChangedEvent& eve)
{
    auto e = eve.e;
    auto actor = e.component<ActorComp>();

    if(actor && actor->dir != eve.dir)
    {
        actor->dir = eve.dir;
        
        changeActorAnimate(eve.e);
        changeActorVelocity(eve.e);
    }
}

void ActorSystem::receive(const EntityCollisionEvent& eve)
{
    auto a = eve.a;
    auto b = eve.b;
    auto aa = a.component<TagComp>();
    auto bb = b.component<TagComp>();
    
    CCLOG("%s, %s", aa->tag.c_str(), bb->tag.c_str());
}

void ActorSystem::receive(const DiedEvent& eve)
{
    auto e = eve.e;
    auto actor = e.component<ActorComp>();
    
    if(actor && actor->action != ActionState::Death)
    {
        actor->action = ActionState::Death;
        
        changeActorAnimate(e);
    }
}

void ActorSystem::changeActorAnimate(entityx::Entity e)
{
    auto animate = e.component<AnimateComp>();
    auto actor = e.component<ActorComp>();
    
    if(!animate) return;
    if(!actor) return;
    
    std::string actionName;
    if(actor->action == ActionState::Stand)
    {
        actionName = "stand";
    }
    else if(actor->action == ActionState::Walk)
    {
        actionName = "walk";
    }
    else if(actor->action == ActionState::Melee)
    {
        actionName = "melee";
    }
    else if(actor->action == ActionState::Death)
    {
        actionName = "death";
    }
    else if(actor->action == ActionState::Corpse)
    {
        actionName = "corpse";
    }
    else
    {
        return;
    }
    
    std::string dirName;
    if(actor->dir == Direction::Up)
    {
        dirName = "up";
    }
    else if(actor->dir == Direction::RightUp || actor->dir == Direction::Right || actor->dir == Direction::RightDown)
    {
        dirName = "right";
    }
    else if(actor->dir == Direction::Down)
    {
        dirName = "down";
    }
    else if(actor->dir == Direction::LeftDown || actor->dir == Direction::Left || actor->dir == Direction::LeftUp)
    {
        dirName = "left";
    }
    else
    {
        return;
    }
    
    std::string animateName = actionName + "_" + dirName;
    auto animateInfo = actor->animateMap[animateName];
    
    CCLOG(animateName.c_str());
    
    animate->filename = animateInfo.filename;
    animate->size = animateInfo.size;
    animate->loop = animateInfo.loop;
    animate->speed = animateInfo.speed;
    animate->index = 0;
    animate->paused = false;
    animate->curDuration = 0.0f;
    animate->initFirstFrame = true;
}

void ActorSystem::changeActorVelocity(entityx::Entity e)
{
    auto velocity = e.component<VelocityComp>();
    auto actor = e.component<ActorComp>();
    
    if(!velocity) return;
    if(!actor) return;
    
    Vec2 vector(0.0f, 0.0f);
    
    if(actor->action == ActionState::Walk)
    {
        if(actor->dir == Direction::Up) vector.set(0, 1);
        else if(actor->dir == Direction::RightUp) vector.set(1, 1);
        else if(actor->dir == Direction::Right) vector.set(1, 0);
        else if(actor->dir == Direction::RightDown) vector.set(1, -1);
        else if(actor->dir == Direction::Down) vector.set(0, -1);
        else if(actor->dir == Direction::LeftDown) vector.set(-1, -1);
        else if(actor->dir == Direction::Left) vector.set(-1, 0);
        else if(actor->dir == Direction::LeftUp) vector.set(-1, 1);
    }
    
    vector.normalize();
    vector = vector * actor->walkSpeed;
    velocity->x = vector.x;
    velocity->y = vector.y;
}

void ActorSystem::update(entityx::EntityManager &es, entityx::EventManager &events, TimeDelta dt)
{
    es.each<ActorComp>([=, &events](Entity e, ActorComp& actor)
    {
        auto animate = e.component<AnimateComp>();
        if(animate)
        {
            // 动画播放完成一次
            if(animate->onceEnd)
            {
                if(actor.action == ActionState::Melee)
                {
                    events.emit<MeleeAttackEndEvent>(e);
                    events.emit<ActorActionChangedEvent>(e, ActionState::Stand);
                }
                else if(actor.action == ActionState::Death)
                {
                    events.emit<ActorActionChangedEvent>(e, ActionState::Corpse);
                }
            }
        }
    });
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void HealthSystem::configure(entityx::EventManager &event_manager)
{
    event_manager.subscribe<DamageEvent>(*this);
}

void HealthSystem::receive(const DamageEvent &eve)
{
    auto targer = eve.targer;
    auto health = targer.component<HealthComp>();
    
    if(!health)
    {
        return;
    }
    
    if(health->isDied)
    {
        return;
    }
    
    //减血
    health->hp -= eve.damage;
    
    if(health->hp < 0)
    {
        health->hp = 0;
    }
}

void HealthSystem::receive(const DiedEvent& eve)
{
    auto e = eve.e;
    auto health = e.component<HealthComp>();
    if(health)
    {
        health->hp = 0;
    }
}

void HealthSystem::update(entityx::EntityManager &es, entityx::EventManager &events, TimeDelta dt)
{
    es.each<HealthComp>([=, &events](Entity e, HealthComp& health)
    {
        if(health.hp <= 0)
        {
            if(!health.isDied)
            {
                health.isDied = true;
                
                DiedEvent eve(e);
                events.emit(eve);//派发死亡事件
            }
        }
    });
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void MeleeSystem::configure(entityx::EventManager &event_manager)
{
    event_manager.subscribe<MeleeAttackEvent>(*this);
    event_manager.subscribe<MeleeAttackEndEvent>(*this);
}

void MeleeSystem::receive(const MeleeAttackEvent& eve)
{
    auto e = eve.e;
    
    auto melee = e.component<MeleeComp>();
    auto physics = e.component<PhysicsComp>();
    auto pos = e.component<PositionComp>();
    auto actor = e.component<ActorComp>();
    
    if(!melee) return;
    if(!physics) return;
    if(!pos) return;
    if(!actor) return;
    
    if(melee->isRunning){
        return;
    }
    
    // 改变实体动作为Melee
    ActorActionChangedEvent actionChange(e, ActionState::Melee);
    __actorActionChangedEvents.push_back(actionChange);
    
    melee->isRunning = true;
    
    auto aabb = physics->body->GetFixtureList()->GetAABB(0);
    
    float minX = aabb.lowerBound.x * PTM_RATIO;
    float minY = aabb.lowerBound.y * PTM_RATIO;
    float maxX = aabb.upperBound.x * PTM_RATIO;
    float maxY = aabb.upperBound.y * PTM_RATIO;
    
    float x, y, w, h;
    
    if(actor->dir == Direction::Up)
    {
        x = pos->x - melee->area.width / 2;
        y = maxY;
        w = melee->area.width;
        h = melee->area.height;
    }
    else if(actor->dir == Direction::Right || actor->dir == Direction::RightUp || actor->dir == Direction::RightDown )
    {
        x = maxX;
        y = pos->y - melee->area.width / 2;
        w = melee->area.height;
        h = melee->area.width;
    }
    else if(actor->dir == Direction::Left || actor->dir == Direction::LeftUp || actor->dir == Direction::LeftDown )
    {
        x = minX - melee->area.height;
        y = pos->y - melee->area.width / 2;
        w = melee->area.height;
        h = melee->area.width;
    }
    else if(actor->dir == Direction::Down)
    {
        x = pos->x - melee->area.width / 2;
        y = minY - melee->area.height;
        w = melee->area.width;
        h = melee->area.height;
    }
    else
    {
        return;
    }
    
#if DRAW_MELEE_DEBUG == 1
    drawDebugArea(Rect(x, y, w, h));
#endif
    
    auto entities = GameHelper::queryEntities(Rect(x, y, w, h));
    // TODO
    for(auto t : entities)
    {
        CCLOG("xxx = %d", e.id());
        
        DamageEvent damageEvent(e, t, melee->damage);
        __damageEvents.push_back(damageEvent);
    }
}

void MeleeSystem::receive(const MeleeAttackEndEvent &eve)
{
    auto e = eve.e;
    auto melee =e.component<MeleeComp>();
    
    if(!melee)
    {
        return;
    }
    
    if(melee->isRunning)
    {
        melee->isRunning = false;
    }
}

void MeleeSystem::update(entityx::EntityManager &es, entityx::EventManager &events, TimeDelta dt)
{
    // 派发动作事件
    for(int i=0; i<__actorActionChangedEvents.size(); i++)
    {
        events.emit(__actorActionChangedEvents[i]);
    }
    __actorActionChangedEvents.clear();
    
    // 派发伤害事件
    for(int i=0; i<__damageEvents.size(); i++)
    {
        events.emit(__damageEvents[i]);
    }
    __damageEvents.clear();
}

void MeleeSystem::drawDebugArea(cocos2d::Rect rect)
{
    DrawNode* node = DrawNode::create();
    node->drawSolidRect(rect.origin, rect.origin+rect.size, Color4F(1, 0.5f, 0, 0.5f));
    Director::getInstance()->getRunningScene()->addChild(node);
    
    auto delay = DelayTime::create(0.4f);
    node->runAction(Sequence::create(delay, CallFunc::create([=](){
        node->removeFromParent();
    }), NULL));
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
InputSystem::InputSystem()
{
    __keyboard = EventListenerKeyboard::create();
    __keyboard->onKeyPressed = [=](EventKeyboard::KeyCode code, cocos2d::Event* e)
    {
        __keyDownMap[code] = true;
    };
    __keyboard->onKeyReleased = [=](EventKeyboard::KeyCode code, cocos2d::Event* e)
    {
        __keyDownMap.erase(code);
    };
    
    auto dispatcher = Director::getInstance()->getEventDispatcher();
    dispatcher->addEventListenerWithFixedPriority(__keyboard, 1);
}

InputSystem::~InputSystem()
{
    auto dispatcher = Director::getInstance()->getEventDispatcher();
    dispatcher->removeEventListener(__keyboard);
}

void InputSystem::configure(entityx::EventManager &events)
{
}

void InputSystem::update(entityx::EntityManager &es, entityx::EventManager &events, TimeDelta dt)
{
    es.each<ControllableComp>([=, &events](Entity e, ControllableComp& controllable)
    {
        if(controllable.controller)
        {
            controllable.controller->control(e, events, __keyDownMap);
        }
    });
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
GameWorld::GameWorld()
{
    systems.add<InputSystem>();
    systems.add<MeleeSystem>();
    systems.add<ActorSystem>();
    systems.add<PhysicsSystem>();
    systems.add<AnimateSystem>();
    systems.add<RenderSystem>();
    systems.add<HealthSystem>();
    systems.configure();
}

void GameWorld::update(float dt)
{
    systems.update<InputSystem>(dt);
    systems.update<MeleeSystem>(dt);
    systems.update<ActorSystem>(dt);
    systems.update<PhysicsSystem>(dt);
    systems.update<AnimateSystem>(dt);
    systems.update<RenderSystem>(dt);
    systems.update<HealthSystem>(dt);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
bool AABBQueryCallback::ReportFixture(b2Fixture *fixture)
{
    Entity* e = (Entity*)fixture->GetBody()->GetUserData();
    entitys.push_back(e);
    return true;
}