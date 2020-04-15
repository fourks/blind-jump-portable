#pragma once


#include "collision.hpp"
#include "entity/entity.hpp"
#include "graphics/animation.hpp"
#include "enemy.hpp"


class Player;
class Laser;


class Turret : public Enemy {
public:
    Turret(const Vec2<Float>& pos);

    void update(Platform& pfrm, Game& game, Microseconds dt);

    void on_collision(Platform&, Game&, Laser&);
    void on_collision(Platform&, Game&, Player&) {}

private:
    Animation<TextureMap::turret, 5, milliseconds(50)> animation_;
    enum class State { sleep, closed, opening, open, closing } state_;
    FadeColorAnimation<Microseconds(9865)> fade_color_anim_;
    Microseconds timer_;
};
