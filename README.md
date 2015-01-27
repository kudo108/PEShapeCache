# PEShapeCache
Physics Editor parser for cocos2dx integrated Physics

Original PEShapeCache from here:
https://github.com/jslim89/Cocos2d-x-PhysicsEditor-demo

Modification:
- remove some function, make memory management more clearly
- support scaleContentFactor (the body will scale to match the resources)

# How to use:

```
//create sprite
	auto sprite = Sprite::createWithSpriteFrameName("main_player_1.png");
            
//create body
            auto body = PEShapeCache::getInstance()->getPhysicsBodyByName("main_player_1");
                        
//some properties
            body->setDynamic(true);
            body->setEnable(true);
            body->setGravityEnable(true);
            body->setCollisionBitmask(0x03);    //0011
            body->setCategoryBitmask(0x03);     //0011
            body->setContactTestBitmask(0x03);  //0011
        
//set physics body
            sprite->setPhysicsBody(body);
```

# Contact me:
[1] Skype: kudo_108

[2] Facebook: http://facebook.com/kudo108