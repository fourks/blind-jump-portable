#include "gatekeeper.hpp"
#include "boss.hpp"
#include "game.hpp"
#include "number/random.hpp"
#include "entity/enemies/common.hpp"


static const Entity::Health initial_health = 125;
static const int default_shield_radius = 24;
static const int max_shield_radius = 100;


Gatekeeper::Gatekeeper(const Vec2<Float>& position, Game& game)
    : Enemy(initial_health, position, {{32, 50}, {16, 36}}),
      state_(State::idle), timer_(0), charge_timer_(0),
      shield_radius_(default_shield_radius)
{
    sprite_.set_texture_index(7);
    sprite_.set_size(Sprite::Size::w32_h32);
    sprite_.set_origin({16, 16});

    head_.set_texture_index(6);
    head_.set_size(Sprite::Size::w32_h32);
    head_.set_origin({16, 48});

    shadow_.set_texture_index(41);
    shadow_.set_size(Sprite::Size::w32_h32);
    shadow_.set_origin({16, 9});
    shadow_.set_alpha(Sprite::Alpha::translucent);

    sprite_.set_position(position);
    head_.set_position(position);
    shadow_.set_position(position);

    game.enemies().spawn<GatekeeperShield>(position, 0);
    game.enemies().spawn<GatekeeperShield>(position, INT16_MAX / 2);
}


void Gatekeeper::update(Platform& pfrm, Game& game, Microseconds dt)
{
    static constexpr const Microseconds jump_duration = milliseconds(500);
    static constexpr const Float movement_rate = 0.000033f;

    charge_timer_ += dt;

    auto face_left = [this] {
        sprite_.set_flip({1, 0});
        head_.set_flip({1, 0});
    };

    auto face_right = [this] {
        sprite_.set_flip({0, 0});
        head_.set_flip({0, 0});
    };

    auto face_player =
        [this, &player = game.player(), &face_left, &face_right] {
            if (player.get_position().x > position_.x) {
                face_left();
            } else {
                face_right();
            }
        };

    auto dist_from_player = [&](Float dist) {
        return distance(position_, game.player().get_position()) > dist;
    };

    // auto second_form = [&] { return get_health() < 85; };

    switch (state_) {
    case State::idle:
        face_player();

        timer_ += dt;
        if (timer_ > milliseconds(600)) {
            timer_ = 0;

            if (dist_from_player(80)) {
                state_ = State::jump;

                sprite_.set_texture_index(48);
                head_.set_texture_index(42);

            } else {
                if (charge_timer_ > seconds(9)) {
                    charge_timer_ = 0;
                    state_ = State::shield_sweep_out;
                }
            }
        }
        break;

    case State::jump:
        timer_ += dt;
        if (timer_ > milliseconds(90)) {
            timer_ = 0;

            const auto index = head_.get_texture_index();
            if (index < 46) {
                head_.set_texture_index(index + 1);
                sprite_.set_texture_index(sprite_.get_texture_index() + 1);

            } else {
                state_ = State::airborne;

                Vec2<Float> dest;
                Vec2<Float> unit;

                int tries = 0;

                do {

                    if (tries++ > 0) {
                        const s16 dir =
                            ((static_cast<float>(random_choice<359>())) / 360) *
                            INT16_MAX;

                        unit = {(float(cosine(dir)) / INT16_MAX),
                                (float(sine(dir)) / INT16_MAX)};

                    } else {
                        unit =
                            direction(position_, game.player().get_position());
                    }

                    move_vec_ = 5.f * unit;

                    const auto tolerance = milliseconds(90);
                    dest = position_ +
                           move_vec_ *
                               ((jump_duration + tolerance) * movement_rate);

                } while (wall_in_path(unit, position_, game, dest));
            }
        }
        break;

    case State::airborne: {
        timer_ += dt;

        const Float offset = 10 *
                             Float(sine(4 * 3.14 * 0.0027f * timer_ + 180)) /
                             std::numeric_limits<s16>::max();

        position_.x += move_vec_.x * dt * movement_rate;
        position_.y += move_vec_.y * dt * movement_rate;

        sprite_.set_position({position_.x, position_.y - abs(offset)});
        head_.set_position({position_.x, position_.y - abs(offset)});
        shadow_.set_position(position_);

        if (timer_ > milliseconds(300)) {
            sprite_.set_texture_index(53);
            head_.set_texture_index(47);
        }
        if (timer_ > jump_duration) {
            head_.set_position(position_);
            sprite_.set_position(position_);

            head_.set_texture_index(45);
            sprite_.set_texture_index(45 + 6);
            state_ = State::landing;
            game.camera().shake();
        }
        break;
    }

    case State::landing:
        timer_ += dt;
        if (timer_ > milliseconds(90)) {
            timer_ = 0;

            const auto index = head_.get_texture_index();
            if (index > 42) {
                head_.set_texture_index(index - 1);
                sprite_.set_texture_index(sprite_.get_texture_index() - 1);

            } else {
                state_ = State::idle;
                sprite_.set_texture_index(7);
                head_.set_texture_index(6);
            }
        }
        break;

    case State::shield_sweep_out:
        timer_ += dt;
        if (timer_ > milliseconds(20)) {
            timer_ = 0;
            if (shield_radius_ < max_shield_radius) {
                ++shield_radius_;
            } else {
                state_ = State::shield_sweep_in1;
            }
        }
        break;

    case State::shield_sweep_in1:
        timer_ += dt;
        if (timer_ > milliseconds(25)) {
            timer_ = 0;
            if (shield_radius_ > max_shield_radius / 2) {
                --shield_radius_;
            } else {
                state_ = State::shield_sweep_in2;
            }
        }
        break;

    case State::shield_sweep_in2:
        timer_ += dt;
        if (timer_ > milliseconds(40)) {
            timer_ = 0;
            if (shield_radius_ > default_shield_radius) {
                --shield_radius_;
            } else {
                state_ = State::idle;
            }
        }
        break;

    }


    fade_color_anim_.advance(sprite_, dt);
    head_.set_mix(sprite_.get_mix());
}


void Gatekeeper::on_collision(Platform& pfrm, Game& game, Laser&)
{
    debit_health(1);

    const auto c = current_zone(game).injury_glow_color_;

    sprite_.set_mix({c, 255});
    head_.set_mix({c, 255});

    show_boss_health(pfrm, game, Float(get_health()) / initial_health);
}


void Gatekeeper::on_death(Platform& pfrm, Game& game)
{
    auto loc = position_ + sprite_.get_origin().cast<Float>();
    loc.x -= 16;
    boss_explosion(pfrm, game, loc);
}


GatekeeperShield::GatekeeperShield(const Vec2<Float>& position, int offset)
    : Enemy(Health(30), position, {{14, 32}, {8, 16}}), timer_(0), state_(State::orbit), offset_(offset)
{
    shadow_.set_alpha(Sprite::Alpha::translucent);
    shadow_.set_size(Sprite::Size::w16_h32);
    shadow_.set_texture_index(TextureMap::drop_shadow);
    shadow_.set_origin({8, -11});

    sprite_.set_size(Sprite::Size::w16_h32);
    sprite_.set_texture_index(16);
    sprite_.set_origin({8, 16});
}


void GatekeeperShield::update(Platform& pfrm, Game& game, Microseconds dt)
{
    fade_color_anim_.advance(sprite_, dt);

    switch (state_) {
    case State::orbit: {
        if (game.enemies().get<Gatekeeper>().empty()) {
            timer_ = seconds(2) + milliseconds(random_choice<900>());
            // sprite_.set_texture_index(16);
            state_ = State::detached;
            return;
        }

        const auto& parent_pos =
            (*game.enemies().get<Gatekeeper>().begin())->get_position();

        const auto radius =
            (*game.enemies().get<Gatekeeper>().begin())->shield_radius();

        if (radius not_eq default_shield_radius) {
            timer_ += (dt / 48) * (1.f - smoothstep(0.f, 1.f, Float(radius) / max_shield_radius));
        } else {
            timer_ += dt / 48;
            reload_ = milliseconds(200);
            shot_count_ = 0;
        }

        const auto x_part = (Float(cosine(timer_ + offset_)) / INT16_MAX);
        const auto y_part = (Float(sine(timer_ + offset_)) / INT16_MAX);

        if (abs(radius - max_shield_radius) < 30) {
            sprite_.set_size(Sprite::Size::w32_h32);
            sprite_.set_texture_index(33);

            reload_ -= dt;
            if (reload_ <= 0) {
                reload_ = milliseconds(40 + shot_count_ * 10);

                game.effects().spawn<OrbShot>(position_,
                                              sample<24>(game.player().get_position()),
                                              0.00019f - 0.00002f * shot_count_);

                if (++shot_count_ == 4) {
                    shot_count_ = 0;
                    reload_ = milliseconds(375);
                }

            }

        } else if (abs(y_part) > 0.8) {
            sprite_.set_size(Sprite::Size::w16_h32);
            sprite_.set_texture_index(16);

        } else if (abs(y_part) > 0.6) {
            sprite_.set_size(Sprite::Size::w16_h32);
            sprite_.set_texture_index(17);

        } else if (abs(y_part) > 0.4) {
            sprite_.set_size(Sprite::Size::w16_h32);
            sprite_.set_texture_index(18);

        } else {
            sprite_.set_size(Sprite::Size::w16_h32);
            sprite_.set_texture_index(19);
        }

        // Rather than draw keyframes for the whole rotation animation, which is
        // relatively simple, I animated a quarter of the shield rotation, and
        // mirrored the sprite accordingly.
        if (y_part > 0) {
            if (x_part > 0) {
                sprite_.set_flip({true, false});
            } else {
                sprite_.set_flip({false, false});
            }
        } else {
            if (x_part > 0) {
                sprite_.set_flip({false, false});
            } else {
                sprite_.set_flip({true, false});
            }
        }

        position_ = {parent_pos.x + radius * x_part,
                     (parent_pos.y - 2) + radius * y_part};

        sprite_.set_position(position_);
        shadow_.set_position(position_);

        break;
    }

    case State::detached:
        timer_ -= dt;

        if (timer_ <= 0) {
            this->kill();
        }
        break;
    }

}


void GatekeeperShield::on_collision(Platform& pfrm, Game& game, Laser&)
{
    if (sprite_.get_size() == Sprite::Size::w32_h32) {
        const auto c = current_zone(game).injury_glow_color_;
        sprite_.set_mix({c, 255});
        debit_health(1);

    } else {
        const auto c = current_zone(game).energy_glow_color_;
        sprite_.set_mix({c, 255});

        // if (not game.enemies().get<Gatekeeper>().empty()) {
        //     (*game.enemies().get<Gatekeeper>().begin())->add_charge();
        // }
    }
}


void GatekeeperShield::on_death(Platform& pfrm, Game& game)
{
    pfrm.sleep(5);

    static const Item::Type item_drop_vec[] = {Item::Type::coin,
                                               Item::Type::null};

    on_enemy_destroyed(pfrm, game, position_, 2, item_drop_vec);

}
