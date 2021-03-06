#pragma once

// NOTE: The snake enemy takes up a lot of resources, we can only realistically
// spawn one per level.


#include "collision.hpp"
#include "enemy.hpp"
#include "entity/entity.hpp"
#include "tileMap.hpp"


class LaserExplosion;
class Player;
class Laser;


class SnakeNode : public Enemy {
public:
    SnakeNode(SnakeNode* parent);

    SnakeNode* parent() const;

    void update(Platform& pfrm, Game& game, Microseconds dt);

    const Vec2<TIdx>& tile_coord() const;

    friend class SnakeTail;

protected:
    void destroy();

    enum class State { sleep, active } state_ = State::sleep;

private:
    SnakeNode* parent_;
    Vec2<TIdx> tile_coord_;
    Microseconds destruct_timer_;
};


class SnakeHead : public SnakeNode {
public:
    SnakeHead(const Vec2<Float>& pos, Game& game);

    void update(Platform& pfrm, Game& game, Microseconds dt);

    void on_collision(Platform&, Game&, Laser&)
    {
    }

    void on_collision(Platform&, Game&, LaserExplosion&)
    {
    }

    void on_collision(Platform&, Game&, Player&)
    {
    }

private:
    enum class Dir { up, down, left, right } dir_;
};


class SnakeBody : public SnakeNode {
public:
    SnakeBody(const Vec2<Float>& pos, SnakeNode* parent, Game&, u8 remaining);

    void update(Platform& pfrm, Game& game, Microseconds dt);

    void on_collision(Platform&, Game&, Laser&)
    {
    }

    void on_collision(Platform&, Game&, LaserExplosion&)
    {
    }

    void on_collision(Platform&, Game&, Player&)
    {
    }

private:
    Vec2<TIdx> next_coord_;
};


class SnakeTail : public SnakeBody {
public:
    SnakeTail(const Vec2<Float>& pos, SnakeNode* parent, Game& game);

    void update(Platform& pfrm, Game& game, Microseconds dt);

    void on_collision(Platform& pf, Game& game, Laser&);

    void on_collision(Platform&, Game&, LaserExplosion&);

    void on_collision(Platform&, Game&, Player&)
    {
    }

    void on_death(Platform&, Game&);

private:
    Microseconds sleep_timer_;
    FadeColorAnimation<Microseconds(9865)> fade_color_anim_;
};
