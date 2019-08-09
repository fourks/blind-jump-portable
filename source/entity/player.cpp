#include "player.hpp"
#include "game.hpp"
#include "platform/platform.hpp"
#include "wallCollision.hpp"


// NOTE: The player code was ported over from the original version of
// BlindJump from 2016. This code is kind of a mess.


const Vec2<s32> v_origin{8, 16};
const Vec2<s32> h_origin{16, 16};


Player::Player()
    : Entity(Health(4)), frame_(0), frame_base_(ResourceLoc::player_still_down),
      anim_timer_(0), color_timer_(0), invulnerability_timer_(0), l_speed_(0.f),
      r_speed_(0.f), u_speed_(0.f), d_speed_(0.f),
      health_(5), hitbox_{&position_, {16, 32}, {8, 16}}
{
    sprite_.set_position({104.f, 64.f});
    sprite_.set_size(Sprite::Size::w16_h32);
    sprite_.set_origin({8, 16});
    shadow_.set_origin({8, -9});
    shadow_.set_texture_index(TextureMap::drop_shadow);
    shadow_.set_size(Sprite::Size::w16_h32);
    shadow_.set_alpha(Sprite::Alpha::translucent);
}


void Player::enemy_contact()
{
    if (not Player::is_invulnerable()) {
        debit_health(1);
        sprite_.set_mix({ColorConstant::coquelicot, 255});
        color_timer_ = 0;
        invulnerability_timer_ = 700000;
    }
}


void Player::on_collision(Platform& pf, Game& game, Critter&)
{
    Player::enemy_contact();
}


void Player::on_collision(Platform& pf, Game& game, Dasher&)
{
    Player::enemy_contact();
}


void Player::on_collision(Platform& pf, Game& game, Turret&)
{
    Player::enemy_contact();
}


void Player::on_collision(Platform& pf, Game& game, Probe&)
{
    Player::enemy_contact();
}


void Player::on_collision(Platform& pf, Game& game, Item& item)
{
    switch (item.get_type()) {
    case Item::Type::heart:
        sprite_.set_mix({ColorConstant::spanish_crimson, 255});
        add_health(1);
        break;

    case Item::Type::coin:
        sprite_.set_mix({ColorConstant::electric_blue, 255});
        break;
    }
    color_timer_ = 0;
}


template <Player::ResourceLoc L>
void Player::key_response(bool k1,
                          bool k2,
                          bool k3,
                          bool k4,
                          float& speed,
                          bool collision)
{
    if (k1) {
        if (not k2 and not k3 and not k4 and frame_base_ != L) {
            frame_base_ = L;
        }
        if (not collision) {
            if (k3 || k4) {
                speed = 1.00f;
            } else {
                speed = 1.50f;
            }
        } else {
            speed = 0.f;
        }
    } else {
        speed = 0.f;
    }
}


template <Player::ResourceLoc S, uint8_t maxIndx>
void Player::on_key_released(bool k2, bool k3, bool k4, bool x)
{
    if (not k2 && not k3 && not k4) {
        if (not x) {
            frame_base_ = S;
            frame_ = maxIndx;
        } else {
            switch (frame_base_) {
            case Player::ResourceLoc::player_walk_down:
                frame_base_ = Player::ResourceLoc::player_still_down;
                frame_ = 0;
                break;

            case Player::ResourceLoc::player_walk_up:
                frame_base_ = Player::ResourceLoc::player_still_up;
                frame_ = 0;
                break;

            case Player::ResourceLoc::player_walk_left:
                frame_base_ = Player::ResourceLoc::player_still_left;
                frame_ = 0;
                break;

            case Player::ResourceLoc::player_walk_right:
                frame_base_ = Player::ResourceLoc::player_still_right;
                frame_ = 0;
                break;

            default:
                break;
            }
        }
    }
}


static uint8_t remap_vframe(uint8_t index)
{
    switch (index) {
    case 0:
        return 1;
    case 1:
        return 2;
    case 2:
        return 2;
    case 3:
        return 1;
    case 4:
        return 0;
    case 5:
        return 3;
    case 6:
        return 4;
    case 7:
        return 4;
    case 8:
        return 3;
    case 9:
        return 0;
    default:
        return 0;
    }
}


template <u8 StepSize>
void Player::update_animation(Microseconds dt, u8 max_index, Microseconds count)
{
    anim_timer_ += dt;
    if (anim_timer_ > count) {
        anim_timer_ -= count;
        frame_ += 1;
    }
    if (frame_ > max_index) {
        frame_ = 0;
    }
}


void Player::soft_update(Platform& pfrm, Game& game, Microseconds dt)
{
    const auto& cmix = sprite_.get_mix();
    if (cmix.amount_ > 0) {
        color_timer_ += dt;
        if (color_timer_ > 15685) {
            color_timer_ -= 15685;
            sprite_.set_mix({cmix.color_, u8(cmix.amount_ - 5)});
        }
    } else {
        sprite_.set_mix({});
    }
}


void Player::update(Platform& pfrm, Game& game, Microseconds dt)
{
    const auto& input = pfrm.keyboard();
    const bool up = input.pressed<Key::up>();
    const bool down = input.pressed<Key::down>();
    const bool left = input.pressed<Key::left>();
    const bool right = input.pressed<Key::right>();

    const auto wc = check_wall_collisions(game.get_tiles(), *this);

    if (invulnerability_timer_ > 0) {
        invulnerability_timer_ -= dt;
    }

    soft_update(pfrm, game, dt);

    key_response<ResourceLoc::player_walk_up>(
        up, down, left, right, u_speed_, wc.up);
    key_response<ResourceLoc::player_walk_down>(
        down, up, left, right, d_speed_, wc.down);
    key_response<ResourceLoc::player_walk_left>(
        left, right, down, up, l_speed_, wc.left);
    key_response<ResourceLoc::player_walk_right>(
        right, left, down, up, r_speed_, wc.right);

    if (input.up_transition<Key::up>()) {
        on_key_released<ResourceLoc::player_still_up, 0>(
            down, left, right, false);
    }
    if (input.up_transition<Key::down>()) {
        on_key_released<ResourceLoc::player_still_down, 0>(
            up, left, right, false);
    }
    if (input.up_transition<Key::left>()) {
        on_key_released<ResourceLoc::player_still_left, 0>(
            up, down, right, false);
    }
    if (input.up_transition<Key::right>()) {
        on_key_released<ResourceLoc::player_still_right, 0>(
            up, down, left, false);
    }

    switch (frame_base_) {
    case ResourceLoc::player_still_up:
    case ResourceLoc::player_still_down:
    case ResourceLoc::player_still_left:
    case ResourceLoc::player_still_right:
        sprite_.set_texture_index(frame_base_);
        break;

    case ResourceLoc::player_walk_up:
        update_animation<1>(dt, 9, 100000);
        sprite_.set_texture_index(frame_base_ + remap_vframe(frame_));
        sprite_.set_size(Sprite::Size::w16_h32);
        sprite_.set_origin(v_origin);
        break;

    case ResourceLoc::player_walk_down:
        update_animation<1>(dt, 9, 100000);
        sprite_.set_texture_index(frame_base_ + remap_vframe(frame_));
        sprite_.set_size(Sprite::Size::w16_h32);
        sprite_.set_origin(v_origin);
        break;

    case ResourceLoc::player_walk_left:
        update_animation<2>(dt, 5, 100000);
        sprite_.set_texture_index(frame_base_ + frame_);
        sprite_.set_size(Sprite::Size::w32_h32);
        sprite_.set_origin(h_origin);
        break;

    case ResourceLoc::player_walk_right:
        update_animation<2>(dt, 5, 100000);
        sprite_.set_texture_index(frame_base_ + frame_);
        sprite_.set_size(Sprite::Size::w32_h32);
        sprite_.set_origin(h_origin);
        break;

    default:
        break;
    }

    static const float MOVEMENT_RATE_CONSTANT = 0.000054f;

    Vec2<Float> new_pos{
        position_.x - ((l_speed_ + -r_speed_) * dt * MOVEMENT_RATE_CONSTANT),
        position_.y - ((u_speed_ + -d_speed_) * dt * MOVEMENT_RATE_CONSTANT)};
    Entity::set_position(new_pos);
    sprite_.set_position(new_pos);
    shadow_.set_position(new_pos);
}


void Player::move(const Vec2<Float>& pos)
{
    Player::set_position(pos);
    sprite_.set_position(pos);
    shadow_.set_position(pos);
    frame_base_ = ResourceLoc::player_still_down;
    sprite_.set_texture_index(frame_base_);
    sprite_.set_size(Sprite::Size::w16_h32);
    sprite_.set_origin(v_origin);
}