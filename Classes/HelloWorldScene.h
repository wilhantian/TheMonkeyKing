#ifndef __HELLOWORLD_SCENE_H__
#define __HELLOWORLD_SCENE_H__

#include "cocos2d.h"

#include "GameWorld.h"

class HelloWorld : public cocos2d::Layer
{
public:
    static cocos2d::Scene* createScene();

    virtual bool init();
    
    void draw(cocos2d::Renderer *renderer, const cocos2d::Mat4 &transform, uint32_t flags);
    void onDraw(const cocos2d::Mat4 &transform, uint32_t flags);
    
    // implement the "static create()" method manually
    CREATE_FUNC(HelloWorld);
    
    GameWorld* gw;
    
    cocos2d::CustomCommand _customCmd;
};

#endif // __HELLOWORLD_SCENE_H__
