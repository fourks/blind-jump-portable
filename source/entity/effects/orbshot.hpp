#pragma once

#include "graphics/animation.hpp"
#include "projectile.hpp"


class Player;


class OrbShot : public Projectile {
public:
    OrbShot(const Vec2<Float>& position,
            const Vec2<Float>& target,
            Float speed,
            Microseconds duration = seconds(2));

    void update(Platform& pf, Game& game, Microseconds dt);

    void on_collision(Platform& pf, Game&, Player&);

    const HitBox& hitbox() const
    {
        return hitbox_;
    }

private:
    Microseconds timer_;
    HitBox hitbox_;
    Animation<TextureMap::orb1, 2, milliseconds(20)> animation_;
};
