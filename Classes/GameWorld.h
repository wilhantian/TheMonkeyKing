//
//  GameWorld.h
//  TheMonkeyKing
//
//  Created by wilhan on 17/12/22.
//
//

#ifndef GameWorld_h
#define GameWorld_h

#include "cocos2d.h"
#include "Box2D/Box2D.h"
#include "entityx/entityx.h"

#include "config.h"

#include "GLES-Render.h"

using namespace entityx;

class Controller;

enum class ActionState : int
{
    None,
    
    Stand,//站立
    Walk,//行走
    Melee,//打斗
    
    Death,//死亡
    Corpse//尸体
};

enum class Direction : int
{
    None,
    
    Up,
    RightUp,
    Right,
    RightDown,
    Down,
    LeftDown,
    Left,
    LeftUp
};

struct AnimateInfo
{
//    std::string name;
    std::string filename;
    int size;
    float speed;
    bool loop;
};

///////////////////////////////////////////////////////////
struct EntityCollisionEvent : public Event<EntityCollisionEvent>
{
    EntityCollisionEvent(Entity a, Entity b):a(a), b(b)
    {
    }
    
    Entity a, b;
};

struct ActorActionChangedEvent : public Event<ActorActionChangedEvent>
{
    ActorActionChangedEvent(Entity e, ActionState action):e(e),action(action)
    {
    }
    
    Entity e;
    ActionState action;
};

struct ActorDirectionChangedEvent : public Event<ActorDirectionChangedEvent>
{
    ActorDirectionChangedEvent(Entity e, Direction dir):e(e),dir(dir)
    {
    }
    
    Entity e;
    Direction dir;
};

struct MeleeAttackEvent : public Event<MeleeAttackEvent>
{
    MeleeAttackEvent(Entity e):e(e)
    {
    }
    
    Entity e;
};

struct MeleeAttackEndEvent : public Event<MeleeAttackEndEvent>
{
    MeleeAttackEndEvent(Entity e):e(e)
    {}
    
    Entity e;
};

struct DiedEvent : public Event<DiedEvent>
{
    DiedEvent(Entity e):e(e)
    {
    }
    
    Entity e;
};

struct DamageEvent : public Event<DamageEvent>
{
    DamageEvent(Entity targer, Entity source, int damage):targer(targer),source(source),damage(damage)
    {
    }
    
    Entity targer;
    Entity source;
    int damage;
};

///////////////////////////////////////////////////////////
struct TagComp
{
    TagComp(std::string tag):tag(tag)
    {}
    
    std::string tag;
};

struct PositionComp
{
    PositionComp(float x = 0.0f, float y = 0.0f)
    {
        this->x = x;
        this->y = y;
    }
    
    float x;
    float y;
};

struct SpriteComp
{
    SpriteComp(cocos2d::Sprite* sprite = nullptr):sprite(sprite)
    {
    }
    
    cocos2d::Sprite* sprite;
};

struct AnimateComp
{
    AnimateComp(std::string animateName = "", int size = 0):filename(filename),frame(frame),size(size), curDuration(0.0f),index(0),loop(true),paused(false),speed(0.3f)
    {
    }
    
    bool loop;
    bool paused;
    float speed;
    int size;
    int index;

    std::string filename;
    std::string frame;
    
    float curDuration;
    
    bool initFirstFrame = true;
    bool onceEnd = false;
};

struct VelocityComp
{
    VelocityComp(float x = 0.0f, float y = 0.0f):x(x),y(y){}
    
    float x;
    float y;
};

struct PhysicsComp
{
    PhysicsComp(b2Body* body = nullptr):body(body){}
    
    b2Body* body;
    Entity self;
};

struct ActorComp
{
    ActorComp(ActionState action = ActionState::None, Direction dir = Direction::None):action(action), dir(dir), walkSpeed(2.0f)
    {}
    
    ActionState action;
    Direction dir;
    
    float walkSpeed;
    
    std::map<std::string, AnimateInfo> animateMap;
};

struct ControllableComp
{
    ControllableComp(Controller* controller = nullptr):controller(controller)
    {
    }
    
    Controller* controller;
};

struct MeleeComp
{
    MeleeComp(cocos2d::Size area=cocos2d::Size::ZERO, int damage=1):area(area),damage(damage)
    {
    }
    
    cocos2d::Size area;//攻击范围
    
    bool isRunning = false;
    int damage;
};

struct HealthComp
{
    HealthComp(int maxHp=1):maxHp(maxHp),hp(maxHp)
    {}
    
    int hp;
    int maxHp;
    
    bool isDied = false;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// RenderSystem
class RenderSystem : public System<RenderSystem>, public Receiver<RenderSystem>
{
public:
    void configure(EventManager &event_manager);
    
    void receive(const ComponentAddedEvent<SpriteComp>& eve);
    void receive(const ComponentRemovedEvent<SpriteComp>& eve);
    
    void update(EntityManager &es, EventManager &events, TimeDelta dt);
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// AnimateSystem
class AnimateSystem : public System<AnimateSystem>, public Receiver<AnimateSystem>
{
public:
    void configure(EventManager &event_manager);
    
    void update(EntityManager &es, EventManager &events, TimeDelta dt);
};

// PhysicsSystem
class PhysicsSystem : public System<PhysicsSystem>, public b2ContactListener, public Receiver<PhysicsSystem>
{
public:
    void configure(EventManager &event_manager);
    
    void receive(const ComponentAddedEvent<PhysicsComp>& eve);
    void receive(const ComponentRemovedEvent<PhysicsComp>& eve);
    
    void update(EntityManager &es, EventManager &events, TimeDelta dt);
    
    virtual void BeginContact(b2Contact* contact);
    virtual void EndContact(b2Contact* contact);
    
    b2World* getB2World();
    b2Draw* getB2DebugDraw();
    
    std::vector<Entity*> queryAABB(cocos2d::Rect rect);
    
private:
    b2World* __world;
    std::vector<EntityCollisionEvent> __collisionEvents;
    GLESDebugDraw* __debugDraw;
};

// ActorSystem
class ActorSystem : public System<ActorSystem>, public Receiver<ActorSystem>
{
public:
    void configure(EventManager &event_manager);
    
    void receive(const ActorActionChangedEvent& eve);
    void receive(const ActorDirectionChangedEvent& eve);
    void receive(const EntityCollisionEvent& eve);
    void receive(const DiedEvent& eve);
    
    void update(EntityManager &es, EventManager &events, TimeDelta dt);
    
    void changeActorAnimate(entityx::Entity e);
    void changeActorVelocity(entityx::Entity e);
};

// MeleeSystem
class MeleeSystem : public System<MeleeSystem>, public Receiver<MeleeSystem>
{
public:
    void configure(EventManager &event_manager);
    
    void receive(const MeleeAttackEvent& eve);
    void receive(const MeleeAttackEndEvent& eve);
    
    void update(EntityManager &es, EventManager &events, TimeDelta dt);
    
private:
    void drawDebugArea(cocos2d::Rect rect);
    
private:
    std::vector<ActorActionChangedEvent> __actorActionChangedEvents;
    std::vector<DamageEvent> __damageEvents;
};

// HealthSystem
class HealthSystem : public System<HealthSystem>, public Receiver<HealthSystem>
{
public:
    void configure(EventManager &event_manager);
    
    void receive(const DamageEvent& eve);
    void receive(const DiedEvent& eve);
    
    void update(EntityManager &es, EventManager &events, TimeDelta dt);
};

// InputSystem
class InputSystem : public System<InputSystem>
{
public:
    InputSystem();
    ~InputSystem();
    
    virtual void configure(EventManager &events);
    
    void update(EntityManager &es, EventManager &events, TimeDelta dt);
    
private:
    cocos2d::EventListenerKeyboard* __keyboard;
    
    std::map<cocos2d::EventKeyboard::KeyCode, bool> __keyDownMap;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// GameWorld
class GameWorld : public EntityX
{
public:
    GameWorld();
    
    void update(float dt);
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
class AABBQueryCallback : public b2QueryCallback
{
public:
    virtual bool ReportFixture(b2Fixture* fixture);
    
public:
    std::vector<Entity*> entitys;
};
///////////////////////////////////////////////////////////
class Controller
{
public:
    virtual void control(Entity e, entityx::EventManager &events, std::map<cocos2d::EventKeyboard::KeyCode, bool> keyDownMap) = 0;
};

class ActorController : public Controller
{
public:
    virtual void control(Entity e, entityx::EventManager &events, std::map<cocos2d::EventKeyboard::KeyCode, bool> keyDownMap)
    {
        auto actor = e.component<ActorComp>();
        
        if(!actor) return;
        
        Direction dir = actor->dir;
        ActionState action = actor->action;
        
        if(action == ActionState::Melee)
        {
            return;
        }
        if(action == ActionState::Death)
        {
            return;
        }
        if(action == ActionState::Corpse)
        {
            return;
        }
        
        // set actor direction and actionstate
        if(keyDownMap[cocos2d::EventKeyboard::KeyCode::KEY_W])
        {
            action = ActionState::Walk;
            if(keyDownMap[cocos2d::EventKeyboard::KeyCode::KEY_A])
            {
                dir = Direction::LeftUp;
            }
            else if(keyDownMap[cocos2d::EventKeyboard::KeyCode::KEY_D])
            {
                dir = Direction::RightUp;
            }
            else
            {
                dir = Direction::Up;
            }
        }
        else if(keyDownMap[cocos2d::EventKeyboard::KeyCode::KEY_S])
        {
            action = ActionState::Walk;
            if(keyDownMap[cocos2d::EventKeyboard::KeyCode::KEY_A])
            {
                dir = Direction::LeftDown;
            }
            else if(keyDownMap[cocos2d::EventKeyboard::KeyCode::KEY_D])
            {
                dir = Direction::RightDown;
            }
            else
            {
                dir = Direction::Down;
            }
        }
        else if(keyDownMap[cocos2d::EventKeyboard::KeyCode::KEY_A])
        {
            action = ActionState::Walk;
            dir = Direction::Left;
        }
        else if(keyDownMap[cocos2d::EventKeyboard::KeyCode::KEY_D])
        {
            action = ActionState::Walk;
            dir = Direction::Right;
        }
        else
        {
            action = ActionState::Stand;
        }
        
        if(keyDownMap[cocos2d::EventKeyboard::KeyCode::KEY_J])
        {
            events.emit<MeleeAttackEvent>(e);
        }
        
        if(actor->dir != dir)
        {
            events.emit<ActorDirectionChangedEvent>(e, dir);
        }
        if(actor->action != action)
        {
            events.emit<ActorActionChangedEvent>(e, action);
        }
    }
};

#endif /* GameWorld_h */
