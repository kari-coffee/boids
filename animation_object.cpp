
#include "animation_object.h"

AnimationObject::AnimationObject() : 
    Position(0.0f, 0.0f), Size(1.0f, 1.0f), Velocity(0.0f), Color(1.0f), Rotation(0.0f), Sprite() { }

AnimationObject::AnimationObject(glm::vec2 pos, glm::vec2 size, Texture2D sprite, glm::vec3 color, glm::vec2 velocity, float rotation): 
    Position(pos), Size(size), Velocity(velocity), Color(color), Rotation(rotation), Sprite(sprite) { }

void AnimationObject::Draw(SpriteRenderer &renderer)
{
    renderer.DrawSprite(this->Sprite, this->Position, this->Size, this->Rotation, this->Color);
}