#ifndef ANIMATIONOBJECT_H
#define ANIMATIONOBJECT_H

#include <GLEW/glew.h>
#include <glm/glm.hpp>

#include "texture.h"
#include "sprite_renderer.h"

class AnimationObject
{
public:
    // object state
    glm::vec2   Position, Size, Velocity;
    glm::vec3   Color;
    float       Rotation;
    // render state
    Texture2D   Sprite;	
    // constructor(s)
    AnimationObject();
    AnimationObject(glm::vec2 pos, glm::vec2 size, Texture2D sprite, glm::vec3 color = glm::vec3(1.0f), glm::vec2 velocity = glm::vec2(0.0f, 0.0f), float rotation = 0.0f);
    // draw sprite
    virtual void Draw(SpriteRenderer &renderer);
};

#endif