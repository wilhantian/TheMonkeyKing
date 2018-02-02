#include "HelloWorldScene.h"
#include "SimpleAudioEngine.h"

#include "entityx/entityx.h"
#include "GameHelper.h"

USING_NS_CC;

Scene* HelloWorld::createScene()
{
    // 'scene' is an autorelease object
    auto scene = Scene::create();
    
    // 'layer' is an autorelease object
    auto layer = HelloWorld::create();

    // add layer as a child to scene
    scene->addChild(layer);

    // return the scene
    return scene;
}

void setActorAniamte(std::map<std::string, AnimateInfo>& animateMap, std::string actorname, int size, float speed)
{
    // 行走
    std::vector<std::string> actions;
    actions.push_back("walk_up");
    actions.push_back("walk_down");
    actions.push_back("walk_left");
    actions.push_back("walk_righ");
    actions.push_back("stand_up");
    actions.push_back("stand_down");
    actions.push_back("stand_left");
    actions.push_back("stand_right");
    actions.push_back("melee_up");
    actions.push_back("melee_down");
    actions.push_back("melee_left");
    actions.push_back("melee_right");
    actions.push_back("death_up");
    actions.push_back("death_down");
    actions.push_back("death_left");
    actions.push_back("death_right");
    actions.push_back("corpse_up");
    actions.push_back("corpse_down");
    actions.push_back("corpse_left");
    actions.push_back("corpse_right");
    
    for(int i=0; i<actions.size(); i++)
    {
        AnimateInfo info;
        info.filename = "animate/" + actorname + "/" + actions[i] + "_%02d.png";
        info.loop = true;
        info.size = size;
        info.speed = speed;
        animateMap[actions[i]] = info;
    }
}

// on "init" you need to initialize your instance
bool HelloWorld::init()
{
    gw = new GameWorld();
    auto physicsSystem = gw->systems.system<PhysicsSystem>();
    GameHelper::init(physicsSystem->getB2World());
    
    {
        auto sprite = Sprite::create("CloseSelected.png");
        this->addChild(sprite);
        
        auto e = gw->entities.create();
        e.assign<SpriteComp>(sprite);
        e.assign<PositionComp>(400, 200);
        e.assign<VelocityComp>(0, 0);
        e.assign<HealthComp>(3);
        
        b2BodyDef def;
        def.type = b2BodyType::b2_dynamicBody;
        auto body = physicsSystem->getB2World()->CreateBody(&def);
        b2PolygonShape shape;
        shape.SetAsBox(40 / 2 / PTM_RATIO, 40 / 2 / PTM_RATIO);
        body->CreateFixture(&shape, 1);
        
        e.assign<TagComp>("entity-A");
        e.assign<PhysicsComp>(body);
    }
    
    {
        auto sprite = Sprite::create("CloseSelected.png");
        this->addChild(sprite);
        
        auto e = gw->entities.create();
        e.assign<SpriteComp>(sprite);
        e.assign<PositionComp>(100, 80);
        e.assign<VelocityComp>(0.5, 0.22);
        e.assign<AnimateComp>("anim%02d.png", 2);
        e.assign<HealthComp>(4);
        
        b2BodyDef def;
        def.type = b2BodyType::b2_dynamicBody;
        auto body = physicsSystem->getB2World()->CreateBody(&def);
        b2PolygonShape shape;
        shape.SetAsBox(40.0f/2/PTM_RATIO, 40.0f/2/PTM_RATIO);
        body->CreateFixture(&shape, 1);
        
        e.assign<TagComp>("entity-B");
        e.assign<PhysicsComp>(body);
        
        // 测试Actor
        e.assign<ActorComp>(ActionState::Walk, Direction::Left);
        
        // 设置Actor动画
        auto actor = e.component<ActorComp>();
        setActorAniamte(actor->animateMap, "hero", 2, 1.3f);
        
        // 控制组件
        e.assign<ControllableComp>(new ActorController());
        
        e.assign<MeleeComp>(Size(40, 20));
    }
    
    schedule([=](float dt)
    {
        gw->update(dt);
    }, "x");
    
    return true;
}

void HelloWorld::draw(Renderer *renderer, const Mat4 &transform, uint32_t flags)
{
    Layer::draw(renderer, transform, flags);
    
    _customCmd.init(_globalZOrder, transform, flags);
    _customCmd.func = CC_CALLBACK_0(HelloWorld::onDraw, this, transform, flags);
    renderer->addCommand(&_customCmd);
}

void HelloWorld::onDraw(const Mat4 &transform, uint32_t flags)
{
    Director* director = Director::getInstance();
    CCASSERT(nullptr != director, "Director is null when setting matrix stack");
    director->pushMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW);
    director->loadMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW, transform);
    
    GL::enableVertexAttribs( cocos2d::GL::VERTEX_ATTRIB_FLAG_POSITION );
    gw->systems.system<PhysicsSystem>()->getB2World()->DrawDebugData();
    CHECK_GL_ERROR_DEBUG();
    
    director->popMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW);
}


