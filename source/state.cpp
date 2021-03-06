#include "state.hpp"
#include "bitvector.hpp"
#include "conf.hpp"
#include "game.hpp"
#include "graphics/overlay.hpp"
#include "localization.hpp"
#include "number/random.hpp"
#include "string.hpp"


void State::enter(Platform&, Game&, State&)
{
}
void State::exit(Platform&, Game&, State&)
{
}
StatePtr State::update(Platform&, Game&, Microseconds)
{
    return null_state();
}


bool within_view_frustum(const Platform::Screen& screen,
                         const Vec2<Float>& pos);


static Bitmatrix<TileMap::width, TileMap::height> visited;


class OverworldState : public State {
public:
    OverworldState(bool camera_tracking) : camera_tracking_(camera_tracking)
    {
    }
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;


    std::optional<Text> notification_text;
    NotificationStr notification_str;
    Microseconds notification_text_timer = 0;
    enum class NotificationStatus {
        flash,
        flash_animate,
        wait,
        display,
        exit,
        hidden
    } notification_status = NotificationStatus::hidden;

private:
    const bool camera_tracking_;
    Microseconds camera_snap_timer_ = 0;
};


class ActiveState : public OverworldState {
public:
    ActiveState(bool camera_tracking = true) : OverworldState(camera_tracking)
    {
    }
    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

    std::optional<BossHealthBar> boss_health_bar_;

private:
    void repaint_icons(Platform& pfrm);

    std::optional<Text> health_;
    std::optional<Text> score_;
    std::optional<SmallIcon> heart_icon_;
    std::optional<SmallIcon> coin_icon_;
    std::optional<HorizontalFlashAnimation> health_anim_;
    std::optional<HorizontalFlashAnimation> score_anim_;
    bool pixelated_ = false;
};


void show_boss_health(Platform& pfrm, Game& game, Float percentage)
{
    if (auto state = dynamic_cast<ActiveState*>(game.state())) {
        if (not state->boss_health_bar_) {
            state->boss_health_bar_.emplace(
                pfrm, 6, OverlayCoord{u8(pfrm.screen().size().x / 8 - 2), 1});
        }

        state->boss_health_bar_->set_health(percentage);
    }
}


void hide_boss_health(Game& game)
{
    if (auto state = dynamic_cast<ActiveState*>(game.state())) {
        state->boss_health_bar_.reset();
    }
}


void push_notification(Platform& pfrm,
                       Game& game,
                       const NotificationStr& string)
{
    pfrm.sleep(3);

    if (auto state = dynamic_cast<OverworldState*>(game.state())) {
        state->notification_status = OverworldState::NotificationStatus::flash;
        state->notification_str = string;
    }
}


class FadeInState : public OverworldState {
public:
    FadeInState() : OverworldState(false)
    {
    }
    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    Microseconds counter_ = 0;
};


class WarpInState : public OverworldState {
public:
    WarpInState() : OverworldState(true)
    {
    }
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    Microseconds counter_ = 0;
    bool shook_ = false;
};


class PreFadePauseState : public OverworldState {
public:
    PreFadePauseState() : OverworldState(false)
    {
    }
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;
};


class GlowFadeState : public OverworldState {
public:
    GlowFadeState() : OverworldState(false)
    {
    }
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    Microseconds counter_ = 0;
};


class FadeOutState : public OverworldState {
public:
    FadeOutState() : OverworldState(false)
    {
    }
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    Microseconds counter_ = 0;
};


class RespawnWaitState : public State {
public:
    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    Microseconds counter_ = 0;
};


class DeathFadeState : public OverworldState {
public:
    DeathFadeState() : OverworldState(false)
    {
    }
    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

    Microseconds counter_ = 0;
};


class DeathContinueState : public State {
public:
    DeathContinueState()
    {
    }
    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    std::optional<Text> score_;
    std::optional<Text> highscore_;
    std::optional<Text> level_;
    std::optional<Text> items_collected_;
    Microseconds counter_ = 0;
    Microseconds counter2_ = 0;
};


class InventoryState : public State {
public:
    InventoryState(bool fade_in);

    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

    static int page_;
    static Vec2<u8> selector_coord_;

private:
    static constexpr const Microseconds fade_duration_ = milliseconds(400);

    void update_arrow_icons(Platform& pfrm);
    void update_item_description(Platform& pfrm, Game& game);
    void clear_items();
    void display_items(Platform& pfrm, Game& game);

    std::optional<Border> selector_;
    std::optional<SmallIcon> left_icon_;
    std::optional<SmallIcon> right_icon_;
    std::optional<Text> page_text_;
    std::optional<Text> item_description_;
    std::optional<Text> item_description2_;
    std::optional<Text> label_;
    std::optional<MediumIcon> item_icons_[Inventory::cols][Inventory::rows];

    Microseconds selector_timer_ = 0;
    Microseconds fade_timer_ = 0;
    bool selector_shaded_ = false;
};


// FIXME: this shouldn't be global...
static std::optional<Platform::Keyboard::RestoreState> restore_keystates;


class NotebookState : public State {
public:
    // NOTE: The NotebookState class does not store a local copy of the text
    // string! Do not pass in pointers to a local buffer, only static strings!
    NotebookState(const char* text);

    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;

    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    void repaint_page(Platform& pfrm);
    void repaint_margin(Platform& pfrm);

    std::optional<TextView> text_;
    std::optional<Text> page_number_;
    const char* str_;
    int page_;
};


class ImageViewState : public State {
public:
    ImageViewState(const char* image_name, ColorConstant background_color);

    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;

    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    const char* image_name_;
    ColorConstant background_color_;
};


static const std::array<LocaleString, 4> legend_strings = {
    LocaleString::map_legend_1,
    LocaleString::map_legend_2,
    LocaleString::map_legend_3,
    LocaleString::map_legend_4};


class MapSystemState : public State {
public:
    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;

    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    Microseconds timer_ = 0;
    enum class AnimState {
        map_enter,
        map_decorate,
        wp_text,
        legend,
        wait
    } anim_state_ = AnimState::map_enter;
    std::optional<Text> level_text_;
    int last_column_ = -1;
    std::array<std::optional<Text>, legend_strings.size()> legend_text_;
    std::optional<Border> legend_border_;
    Microseconds map_enter_duration_;
};


class IntroCreditsState : public State {
public:
    IntroCreditsState(const char* str) : str_(str)
    {
    }

    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;

    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

    virtual StatePtr next_state(Platform& pfrm, Game& game);

private:
    void center(Platform& pfrm);

    const char* str_;
    std::optional<Text> text_;
    Microseconds timer_ = 0;
};


class IntroLegalMessage : public IntroCreditsState {
public:
    IntroLegalMessage()
        : IntroCreditsState(locale_string(LocaleString::intro_text_1))
    {
    }

    StatePtr next_state(Platform& pfrm, Game& game) override;
};


class EndingCreditsState : public State {
public:
    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;

    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    Microseconds timer_ = 0;
    Buffer<Text, 12> lines_; // IMPORTANT: If you're porting this code to a
                             // platform with a taller screen size, you may need
                             // to increase the line capacity here.
    int scroll_ = 0;
    int next_ = 0;
    int next_y_ = 0;
};


class NewLevelState : public State {
public:
    NewLevelState(Level next_level) : timer_(0), next_level_(next_level)
    {
    }

    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;

    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    std::optional<Text> text_[2];
    Microseconds timer_;
    OverlayCoord pos_;
    Level next_level_;
};


class GoodbyeState : public State {
public:
    void enter(Platform& pfrm, Game& game, State& prev_state) override;

    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    Microseconds wait_timer_ = 0;
    std::optional<Text> text_;
};


class PauseScreenState : public State {
public:
    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;
    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    void draw_cursor(Platform& pfrm);

    Microseconds fade_timer_ = 0;
    Microseconds log_timer_ = 0;
    int cursor_loc_ = 0;
    int anim_index_ = 0;
    Microseconds anim_timer_ = 0;
    std::optional<Text> resume_text_;
    std::optional<Text> save_and_quit_text_;
};


// This is a hidden game state intended for debugging. The user can enter
// various numeric codes, which trigger state changes within the game
// (e.g. jumping to a boss fight/level, spawing specific enemies, setting the
// random seed, etc.)
class CommandCodeState : public State {
public:
    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;

    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    bool handle_command_code(Platform& pfrm, Game& game);

    void push_char(Platform& pfrm, Game& game, char c);

    void update_selector(Platform& pfrm, Microseconds dt);

    StringBuffer<10> input_;
    std::optional<Text> input_text_;
    std::optional<Text> numbers_;
    std::optional<Border> entry_box_;
    std::optional<Border> selector_;
    Microseconds selector_timer_ = 0;
    bool selector_shaded_ = false;
    u8 selector_index_ = 0;
    Level next_level_;
};


class LogfileViewerState : public State {
public:
    void enter(Platform& pfrm, Game& game, State& prev_state) override;
    void exit(Platform& pfrm, Game& game, State& next_state) override;

    StatePtr update(Platform& pfrm, Game& game, Microseconds delta) override;

private:
    void repaint(Platform& pfrm, int offset);
    int offset_ = 0;
};


static void state_deleter(State* s);

StatePtr null_state()
{
    return {nullptr, state_deleter};
}


template <typename... States> class StatePool {
public:
    template <typename TState, typename... Args> StatePtr create(Args&&... args)
    {
        static_assert(std::disjunction<std::is_same<TState, States>...>(),
                      "State missing from state pool");

        if (auto mem = pool_.get()) {
            new (mem) TState(std::forward<Args>(args)...);

            return {reinterpret_cast<TState*>(mem), state_deleter};
        } else {

            return null_state();
        }
    }

    Pool<std::max({sizeof(States)...}),
         // We should only need memory for two states at any given time: the
         // current state, and the next state.
         3,
         std::max({alignof(States)...})>
        pool_;
};
static StatePool<ActiveState,
                 FadeInState,
                 WarpInState,
                 PreFadePauseState,
                 GlowFadeState,
                 FadeOutState,
                 GoodbyeState,
                 DeathFadeState,
                 InventoryState,
                 NotebookState,
                 ImageViewState,
                 NewLevelState,
                 CommandCodeState,
                 PauseScreenState,
                 MapSystemState,
                 IntroCreditsState,
                 IntroLegalMessage,
                 DeathContinueState,
                 RespawnWaitState,
                 LogfileViewerState,
                 EndingCreditsState>
    state_pool_;


static void state_deleter(State* s)
{
    if (s) {
        s->~State();
        state_pool_.pool_.post(reinterpret_cast<byte*>(s));
    }
}


////////////////////////////////////////////////////////////////////////////////
// OverworldState
////////////////////////////////////////////////////////////////////////////////


static Microseconds enemy_lethargy_timer;


void OverworldState::exit(Platform& pfrm, Game&, State&)
{
    notification_text.reset();

    // In case we're in the middle of an entry/exit animation for the
    // notification bar.
    for (int i = 0; i < 32; ++i) {
        pfrm.set_tile(Layer::overlay, i, 0, 0);
    }

    notification_status = NotificationStatus::hidden;
}


StatePtr OverworldState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    animate_starfield(pfrm, delta);

    Player& player = game.player();

    auto update_policy = [&](auto& entity_buf) {
        for (auto it = entity_buf.begin(); it not_eq entity_buf.end();) {
            if (not(*it)->alive()) {
                (*it)->on_death(pfrm, game);
                it = entity_buf.erase(it);
            } else {
                (*it)->update(pfrm, game, delta);
                ++it;
            }
        }
    };

    game.effects().transform(update_policy);
    game.details().transform(update_policy);

    auto enemy_timestep = delta;
    if (enemy_lethargy_timer > 0) {
        enemy_lethargy_timer -= delta;
        enemy_timestep /= 2;
    }

    if (pfrm.keyboard().up_transition<Key::action_1>()) {
        camera_snap_timer_ = seconds(2) + milliseconds(250);
    }

    if (camera_snap_timer_ > 0) {
        if (pfrm.keyboard().down_transition<Key::action_2>()) {
            camera_snap_timer_ = 0;
        }
        camera_snap_timer_ -= delta;
    }

    bool enemies_remaining = false;
    bool enemies_destroyed = false;
    bool enemies_visible = false;
    game.enemies().transform([&](auto& entity_buf) {
        for (auto it = entity_buf.begin(); it not_eq entity_buf.end();) {
            if (not(*it)->alive()) {
                (*it)->on_death(pfrm, game);
                it = entity_buf.erase(it);
                enemies_destroyed = true;
            } else {
                enemies_remaining = true;

                (*it)->update(pfrm, game, enemy_timestep);

                if (camera_tracking_ and
                    (pfrm.keyboard().pressed<Key::action_1>() or
                     camera_snap_timer_ > 0)) {
                    // NOTE: snake body segments do not make much sense to
                    // center the camera on, so exclude them. Same for various
                    // other enemies...
                    using T = typename std::remove_reference<decltype(
                        entity_buf)>::type;

                    using VT = typename T::ValueType::element_type;

                    if constexpr (not std::is_same<VT, SnakeBody>() and
                                  not std::is_same<VT, SnakeHead>() and
                                  not std::is_same<VT, GatekeeperShield>()) {
                        if ((*it)->visible()) {
                            enemies_visible = true;
                            game.camera().push_ballast((*it)->get_position());
                        }
                    }
                }
                ++it;
            }
        }
    });

    if (not enemies_visible) {
        camera_snap_timer_ = 0;
    }

    if (not enemies_remaining and enemies_destroyed) {

        NotificationStr str;
        str += locale_string(LocaleString::level_clear);

        push_notification(pfrm, game, str);
    }


    switch (notification_status) {
    case NotificationStatus::hidden:
        break;

    case NotificationStatus::flash:
        for (int x = 0; x < 32; ++x) {
            pfrm.set_tile(Layer::overlay, x, 0, 108);
        }
        notification_status = NotificationStatus::flash_animate;
        notification_text_timer = -1 * milliseconds(5);
        break;

    case NotificationStatus::flash_animate:
        notification_text_timer += delta;
        if (notification_text_timer > milliseconds(10)) {
            notification_text_timer = 0;

            const auto current_tile = pfrm.get_tile(Layer::overlay, 0, 0);
            if (current_tile < 110) {
                for (int x = 0; x < 32; ++x) {
                    pfrm.set_tile(Layer::overlay, x, 0, current_tile + 1);
                }
            } else {
                notification_status = NotificationStatus::wait;
                notification_text_timer = milliseconds(80);
            }
        }
        break;

    case NotificationStatus::wait:
        notification_text_timer -= delta;
        if (notification_text_timer <= 0) {
            notification_text_timer = seconds(3);

            notification_text.emplace(pfrm, OverlayCoord{0, 0});

            const auto margin =
                centered_text_margins(pfrm, notification_str.length());

            left_text_margin(*notification_text, margin);
            notification_text->append(notification_str.c_str());
            right_text_margin(*notification_text, margin);

            notification_status = NotificationStatus::display;
        }
        break;

    case NotificationStatus::display:
        if (notification_text_timer > 0) {
            notification_text_timer -= delta;

        } else {
            notification_text_timer = 0;
            notification_status = NotificationStatus::exit;

            for (int x = 0; x < 32; ++x) {
                pfrm.set_tile(Layer::overlay, x, 0, 112);
            }
        }
        break;

    case NotificationStatus::exit: {
        notification_text_timer += delta;
        if (notification_text_timer > milliseconds(34)) {
            notification_text_timer = 0;

            const auto tile = pfrm.get_tile(Layer::overlay, 0, 0);
            if (tile < 120) {
                for (int x = 0; x < 32; ++x) {
                    pfrm.set_tile(Layer::overlay, x, 0, tile + 1);
                }
            } else {
                notification_status = NotificationStatus::hidden;
                notification_text.reset();
            }
        }
        break;
    }
    }

    game.camera().update(pfrm, delta, player.get_position());


    check_collisions(pfrm, game, player, game.details().get<Item>());
    check_collisions(pfrm, game, player, game.effects().get<OrbShot>());

    if (not is_boss_level(game.level())) {
        check_collisions(pfrm, game, player, game.enemies().get<Drone>());
        check_collisions(pfrm, game, player, game.enemies().get<Turret>());
        check_collisions(pfrm, game, player, game.enemies().get<Dasher>());
        check_collisions(pfrm, game, player, game.enemies().get<SnakeHead>());
        check_collisions(pfrm, game, player, game.enemies().get<SnakeBody>());
        check_collisions(pfrm, game, player, game.enemies().get<Theif>());
        check_collisions(pfrm,
                         game,
                         game.effects().get<Laser>(),
                         game.enemies().get<Drone>());
        check_collisions(pfrm,
                         game,
                         game.effects().get<Laser>(),
                         game.enemies().get<Theif>());
        check_collisions(pfrm,
                         game,
                         game.effects().get<Laser>(),
                         game.enemies().get<Dasher>());
        check_collisions(pfrm,
                         game,
                         game.effects().get<Laser>(),
                         game.enemies().get<Turret>());
        check_collisions(pfrm,
                         game,
                         game.effects().get<Laser>(),
                         game.enemies().get<SnakeBody>());
        check_collisions(pfrm,
                         game,
                         game.effects().get<Laser>(),
                         game.enemies().get<SnakeHead>());
        check_collisions(pfrm,
                         game,
                         game.effects().get<Laser>(),
                         game.enemies().get<SnakeTail>());
        check_collisions(pfrm,
                         game,
                         game.effects().get<Laser>(),
                         game.enemies().get<Scarecrow>());
    } else {
        check_collisions(
            pfrm, game, player, game.enemies().get<TheFirstExplorer>());
        check_collisions(
            pfrm, game, player, game.effects().get<FirstExplorerBigLaser>());
        check_collisions(
            pfrm, game, player, game.effects().get<FirstExplorerSmallLaser>());
        check_collisions(pfrm, game, player, game.enemies().get<Gatekeeper>());
        check_collisions(
            pfrm, game, player, game.enemies().get<GatekeeperShield>());
        check_collisions(pfrm,
                         game,
                         game.effects().get<Laser>(),
                         game.enemies().get<TheFirstExplorer>());
        check_collisions(pfrm,
                         game,
                         game.effects().get<Laser>(),
                         game.enemies().get<Gatekeeper>());
        check_collisions(pfrm,
                         game,
                         game.effects().get<Laser>(),
                         game.enemies().get<GatekeeperShield>());
    }

    return null_state();
}


////////////////////////////////////////////////////////////////////////////////
// ActiveState
////////////////////////////////////////////////////////////////////////////////


void ActiveState::repaint_icons(Platform& pfrm)
{
    auto screen_tiles = calc_screen_tiles(pfrm);

    heart_icon_.emplace(pfrm, 145, OverlayCoord{1, u8(screen_tiles.y - 3)});
    coin_icon_.emplace(pfrm, 146, OverlayCoord{1, u8(screen_tiles.y - 2)});
}


void ActiveState::enter(Platform& pfrm, Game& game, State&)
{
    if (restore_keystates) {
        pfrm.keyboard().restore_state(*restore_keystates);
        restore_keystates.reset();
    }

    auto screen_tiles = calc_screen_tiles(pfrm);

    health_.emplace(pfrm, OverlayCoord{2, u8(screen_tiles.y - 3)});
    score_.emplace(pfrm, OverlayCoord{2, u8(screen_tiles.y - 2)});
    health_->assign(game.player().get_health());
    score_->assign(game.score());

    repaint_icons(pfrm);

    health_anim_.emplace(pfrm, OverlayCoord{1, u8(screen_tiles.y - 3)});
    score_anim_.emplace(pfrm, OverlayCoord{1, u8(screen_tiles.y - 2)});
}


void ActiveState::exit(Platform& pfrm, Game& game, State& next_state)
{
    OverworldState::exit(pfrm, game, next_state);

    health_.reset();
    score_.reset();
    heart_icon_.reset();
    coin_icon_.reset();

    pfrm.screen().pixelate(0);
}


StatePtr ActiveState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    game.player().update(pfrm, game, delta);

    const Vec2<TIdx> tile_coords =
        to_tile_coord(game.player().get_position().cast<s32>());
    visited.set(tile_coords.x, tile_coords.y, true);

    const auto& player_spr = game.player().get_sprite();
    if (auto amt = player_spr.get_mix().amount_) {
        if (player_spr.get_mix().color_ ==
            current_zone(game).injury_glow_color_) {
            pfrm.screen().pixelate(amt / 8, false);
            pixelated_ = true;
        }
    } else if (pixelated_) {
        pfrm.screen().pixelate(0);
        pixelated_ = false;
    }

    const auto last_health = game.player().get_health();
    const auto last_score = game.score();

    OverworldState::update(pfrm, game, delta);

    if (last_health not_eq game.player().get_health()) {
        auto screen_tiles = calc_screen_tiles(pfrm);
        health_anim_.emplace(pfrm, OverlayCoord{1, u8(screen_tiles.y - 3)});
        health_anim_->init(integer_text_length(game.player().get_health()) + 1);
        // health_->assign(game.player().get_health());
    }

    if (last_score not_eq game.score()) {
        auto screen_tiles = calc_screen_tiles(pfrm);
        score_anim_.emplace(pfrm, OverlayCoord{1, u8(screen_tiles.y - 2)});
        score_anim_->init(integer_text_length(game.score()) + 1);
        // score_->assign(game.score());
    }

    if (health_anim_) {
        health_anim_->update(delta);
        if (health_anim_->done()) {
            health_anim_.reset();
            repaint_icons(pfrm);
            health_->assign(game.player().get_health());
        }
    }

    if (score_anim_) {
        score_anim_->update(delta);
        if (score_anim_->done()) {
            score_anim_.reset();
            repaint_icons(pfrm);
            score_->assign(game.score());
        }
    }

    if (game.player().get_health() == 0) {
        pfrm.sleep(5);
        pfrm.speaker().stop_music();
        // TODO: add a unique explosion sound effect
        pfrm.speaker().play_sound(
            "explosion1", 3, game.player().get_position());
        big_explosion(pfrm, game, game.player().get_position());

        return state_pool_.create<DeathFadeState>();
    }

    if (pfrm.keyboard().down_transition<Key::alt_2>()) {

        // We don't update entities, e.g. the player entity, while in the
        // inventory state, so the player will not receive the up_transition
        // keystate unless we push the keystates, and restore after exiting the
        // inventory screen.
        restore_keystates = pfrm.keyboard().dump_state();

        pfrm.speaker().play_sound("openbag", 2);

        return state_pool_.create<InventoryState>(true);
    }

    if (pfrm.keyboard().down_transition<Key::start>() and
        not is_boss_level(game.level())) {

        restore_keystates = pfrm.keyboard().dump_state();

        return state_pool_.create<PauseScreenState>();
    }

    if (game.transporter().visible()) {

        const auto& t_pos =
            game.transporter().get_position() - Vec2<Float>{0, 22};

        const auto dist =
            apply_gravity_well(t_pos, game.player(), 64, 34, 1.3f, 34);

        if (dist < 6) {
            game.player().move(t_pos);
            pfrm.speaker().play_sound("bell", 5);
            return state_pool_.create<PreFadePauseState>();
        }
    }

    return null_state();
}


////////////////////////////////////////////////////////////////////////////////
// FadeInState
////////////////////////////////////////////////////////////////////////////////


void FadeInState::enter(Platform& pfrm, Game& game, State&)
{
    game.player().set_visible(false);
    game.camera().set_speed(0.75);
}


void FadeInState::exit(Platform& pfrm, Game& game, State&)
{
    game.camera().set_speed(1.f);
}


StatePtr FadeInState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    game.player().soft_update(pfrm, game, delta);

    OverworldState::update(pfrm, game, delta);

    counter_ += delta;

    constexpr auto fade_duration = milliseconds(800);
    if (counter_ > fade_duration) {
        pfrm.screen().fade(1.f, current_zone(game).energy_glow_color_);
        pfrm.sleep(2);
        game.player().set_visible(true);
        return state_pool_.create<WarpInState>();
    } else {
        const auto amount = 1.f - smoothstep(0.f, fade_duration, counter_);
        pfrm.screen().fade(amount);
        return null_state();
    }
}


////////////////////////////////////////////////////////////////////////////////
// WarpInState
////////////////////////////////////////////////////////////////////////////////


StatePtr WarpInState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    game.player().soft_update(pfrm, game, delta);

    OverworldState::update(pfrm, game, delta);

    counter_ += delta;

    if (counter_ > milliseconds(200) and not shook_) {
        game.camera().shake();
        shook_ = true;
    }

    constexpr auto fade_duration = milliseconds(950);
    if (counter_ > fade_duration) {
        shook_ = false;
        pfrm.screen().fade(0.f, current_zone(game).energy_glow_color_);
        pfrm.screen().pixelate(0);
        return state_pool_.create<ActiveState>();
    } else {
        const auto amount = 1.f - smoothstep(0.f, fade_duration, counter_);
        pfrm.screen().fade(amount, current_zone(game).energy_glow_color_);
        if (amount > 0.5f) {
            pfrm.screen().pixelate((amount - 0.5f) * 60);
        }
        return null_state();
    }
}


////////////////////////////////////////////////////////////////////////////////
// PreFadePauseState
////////////////////////////////////////////////////////////////////////////////


StatePtr
PreFadePauseState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    game.camera().set_speed(0.75f);

    game.player().soft_update(pfrm, game, delta);

    OverworldState::update(pfrm, game, delta);

    if (manhattan_length(pfrm.screen().get_view().get_center() +
                             pfrm.screen().get_view().get_size() / 2.f,
                         game.player().get_position()) < 18) {
        game.camera().set_speed(1.f);
        return state_pool_.create<GlowFadeState>();
    } else {
        return null_state();
    }
}


////////////////////////////////////////////////////////////////////////////////
// GlowFadeState
////////////////////////////////////////////////////////////////////////////////


StatePtr GlowFadeState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    game.player().soft_update(pfrm, game, delta);

    OverworldState::update(pfrm, game, delta);

    counter_ += delta;

    constexpr auto fade_duration = milliseconds(950);
    if (counter_ > fade_duration) {
        pfrm.screen().fade(1.f, current_zone(game).energy_glow_color_);
        pfrm.screen().pixelate(0);
        return state_pool_.create<FadeOutState>();
    } else {
        const auto amount = smoothstep(0.f, fade_duration, counter_);
        pfrm.screen().fade(amount, current_zone(game).energy_glow_color_);
        if (amount > 0.25f) {
            pfrm.screen().pixelate((amount - 0.25f) * 60);
        }
        return null_state();
    }
}


////////////////////////////////////////////////////////////////////////////////
// FadeOutState
////////////////////////////////////////////////////////////////////////////////


StatePtr FadeOutState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    game.player().soft_update(pfrm, game, delta);

    OverworldState::update(pfrm, game, delta);

    counter_ += delta;

    constexpr auto fade_duration = milliseconds(670);
    if (counter_ > fade_duration) {
        pfrm.screen().fade(1.f);

        Level next_level = game.level() + 1;

        // backdoor for debugging purposes.
        if (pfrm.keyboard().all_pressed<Key::alt_1, Key::alt_2, Key::start>()) {
            return state_pool_.create<CommandCodeState>();
        }

        // For now, to determine whether the game's complete, scan through a
        // bunch of levels. If there are no more bosses remaining, the game is
        // complete.
        bool bosses_remaining = false;
        for (Level l = next_level; l < next_level + 1000; ++l) {
            if (is_boss_level(l)) {
                bosses_remaining = true;
                break;
            }
        }

        auto zone = zone_info(next_level);
        auto last_zone = zone_info(game.level());

        if (not bosses_remaining and not(zone == last_zone)) {
            pfrm.sleep(120);
            return state_pool_.create<EndingCreditsState>();
        }

        return state_pool_.create<NewLevelState>(next_level);

    } else {
        pfrm.screen().fade(smoothstep(0.f, fade_duration, counter_),
                           ColorConstant::rich_black,
                           current_zone(game).energy_glow_color_);

        return null_state();
    }
}


////////////////////////////////////////////////////////////////////////////////
// DeathFadeState
////////////////////////////////////////////////////////////////////////////////


void DeathFadeState::enter(Platform& pfrm, Game& game, State& prev_state)
{
    enemy_lethargy_timer = 0;

    for (auto& score : reversed(game.highscores())) {
        if (score < game.score()) {
            score = game.score();
            break;
        }
    }
    std::sort(game.highscores().rbegin(), game.highscores().rend());

    rng::get();

    game.persistent_data().seed_ = rng::global_state;
    game.inventory().remove_non_persistent();

    PersistentData& data = game.persistent_data().reset(pfrm);
    pfrm.write_save_data(&data, sizeof data);
}


StatePtr DeathFadeState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    OverworldState::update(pfrm, game, delta);

    counter_ += delta;

    constexpr auto fade_duration = seconds(2);
    if (counter_ > fade_duration) {
        pfrm.screen().pixelate(0);

        auto screen_tiles = calc_screen_tiles(pfrm);

        const auto image_width = 18;

        draw_image(pfrm,
                   450,
                   (screen_tiles.x - image_width) / 2,
                   3,
                   image_width,
                   3,
                   Layer::overlay);

        return state_pool_.create<DeathContinueState>();
    } else {
        const auto amount = smoothstep(0.f, fade_duration, counter_);
        pfrm.screen().fade(amount, current_zone(game).injury_glow_color_);
        pfrm.screen().pixelate(amount * 100);
        return null_state();
    }
}


////////////////////////////////////////////////////////////////////////////////
// RespawnState
////////////////////////////////////////////////////////////////////////////////


void RespawnWaitState::enter(Platform& pfrm, Game& game, State&)
{
    pfrm.speaker().play_sound("bell", 5);
}


StatePtr
RespawnWaitState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    counter_ += delta;

    if (counter_ > milliseconds(1900)) {
        return state_pool_.create<NewLevelState>(Level{0});
    }

    return null_state();
}


////////////////////////////////////////////////////////////////////////////////
// DeathContinueState
////////////////////////////////////////////////////////////////////////////////


void DeathContinueState::enter(Platform& pfrm, Game& game, State&)
{
    game.player().set_visible(false);
}


void DeathContinueState::exit(Platform& pfrm, Game& game, State&)
{
    score_.reset();
    highscore_.reset();
    level_.reset();
    items_collected_.reset();

    pfrm.fill_overlay(0);
}


StatePtr
DeathContinueState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    counter_ += delta;

    constexpr auto fade_duration = seconds(1);
    if (counter_ > fade_duration) {
        counter_ -= delta;
        pfrm.screen().fade(1.f,
                           ColorConstant::rich_black,
                           current_zone(game).injury_glow_color_);

        constexpr auto stats_time = milliseconds(500);

        if (counter2_ < stats_time) {
            const bool show_stats = counter2_ + delta > stats_time;

            counter2_ += delta;

            if (show_stats) {
                score_.emplace(pfrm, Vec2<u8>{1, 8});
                highscore_.emplace(pfrm, Vec2<u8>{1, 10});
                level_.emplace(pfrm, Vec2<u8>{1, 12});
                items_collected_.emplace(pfrm, Vec2<u8>{1, 14});

                const auto screen_tiles = calc_screen_tiles(pfrm);

                auto print_metric = [&](Text& target,
                                        const char* str,
                                        int num,
                                        const char* suffix = "") {
                    target.append(str);

                    const auto iters =
                        screen_tiles.x -
                        (utf8::len(str) + 2 + integer_text_length(num) +
                         str_len(suffix));
                    for (u32 i = 0; i < iters; ++i) {
                        target.append(
                            locale_string(LocaleString::punctuation_period));
                    }

                    target.append(num);
                    target.append(suffix);
                };

                print_metric(
                    *score_, locale_string(LocaleString::score), game.score());
                print_metric(*highscore_,
                             locale_string(LocaleString::high_score),
                             game.highscores()[0]);
                print_metric(*level_,
                             locale_string(LocaleString::waypoints),
                             game.level());
                print_metric(
                    *items_collected_,
                    locale_string(LocaleString::items_collected_prefix),
                    100 * items_collected_percentage(game.inventory()),
                    locale_string(LocaleString::items_collected_suffix));
            }
        }

        if (score_) {
            if (pfrm.keyboard().pressed<Key::action_1>() or
                pfrm.keyboard().pressed<Key::action_2>()) {

                game.score() = 0;
                game.player().revive(pfrm);

                return state_pool_.create<RespawnWaitState>();
            }
        }
        return null_state();

    } else {
        pfrm.screen().fade(smoothstep(0.f, fade_duration, counter_),
                           ColorConstant::rich_black,
                           current_zone(game).injury_glow_color_);
        return null_state();
    }
}


////////////////////////////////////////////////////////////////////////////////
// InventoryState
////////////////////////////////////////////////////////////////////////////////


// These are static, because it's simply more convenient for the player when the inventory page and selector are persistent across opening/closing the inventory page.
int InventoryState::page_{0};
Vec2<u8> InventoryState::selector_coord_{0, 0};


struct InventoryItemHandler {
    Item::Type type_;
    int icon_;
    StatePtr (*callback_)(Platform& pfrm, Game& game);
    LocaleString description_;
    bool single_use_ = false;
};


static void consume_selected_item(Game& game)
{
    game.inventory().remove_item(InventoryState::page_,
                                 InventoryState::selector_coord_.x,
                                 InventoryState::selector_coord_.y);
}


constexpr int item_icon(Item::Type item)
{
    int icon_base = 177;

    return icon_base +
           (static_cast<int>(item) - (static_cast<int>(Item::Type::coin) + 1)) *
               4;
}


// Just so we don't have to type so much stuff. Some items will invariably use
// icons from existing items, so we still want the flexibility of setting icon
// values manually.
#define STANDARD_ITEM_HANDLER(TYPE)                                            \
    Item::Type::TYPE, item_icon(Item::Type::TYPE)


constexpr static const InventoryItemHandler inventory_handlers[] = {
    {Item::Type::null,
     0,
     [](Platform&, Game&) { return null_state(); },
     LocaleString::empty_inventory_str},
    {STANDARD_ITEM_HANDLER(old_poster_1),
     [](Platform&, Game&) {
         static const auto str = "old_poster_flattened";
         return state_pool_.create<ImageViewState>(str,
                                                   ColorConstant::steel_blue);
     },
     LocaleString::old_poster_title},
    {STANDARD_ITEM_HANDLER(surveyor_logbook),
     [](Platform&, Game&) {
         return state_pool_.create<NotebookState>(
             locale_string(LocaleString::logbook_str_1));
     },
     LocaleString::surveyor_logbook_title},
    {STANDARD_ITEM_HANDLER(blaster),
     [](Platform&, Game&) { return null_state(); },
     LocaleString::blaster_title},
    {STANDARD_ITEM_HANDLER(accelerator),
     [](Platform&, Game& game) {
         game.player().weapon().accelerate(3, milliseconds(150));
         return null_state();
     },
     LocaleString::accelerator_title,
     true},
    {STANDARD_ITEM_HANDLER(lethargy),
     [](Platform&, Game& game) {
         enemy_lethargy_timer = seconds(18);
         return null_state();
     },
     LocaleString::lethargy_title,
     true},
    {STANDARD_ITEM_HANDLER(map_system),
     [](Platform&, Game&) { return state_pool_.create<MapSystemState>(); },
     LocaleString::map_system_title},
    {STANDARD_ITEM_HANDLER(explosive_rounds_2),
     [](Platform&, Game& game) {
         game.player().weapon().add_explosive_rounds(2);
         return null_state();
     },
     LocaleString::explosive_rounds_title,
     true},
    {STANDARD_ITEM_HANDLER(seed_packet),
     [](Platform&, Game&) {
         static const auto str = "seed_packet_flattened";
         return state_pool_.create<ImageViewState>(str,
                                                   ColorConstant::steel_blue);
     },
     LocaleString::seed_packet_title},
    {STANDARD_ITEM_HANDLER(engineer_notebook),
     [](Platform&, Game&) {
         return state_pool_.create<NotebookState>(
             locale_string(LocaleString::engineer_notebook_str));
     },
     LocaleString::engineer_notebook_title}};


static const InventoryItemHandler* inventory_item_handler(Item::Type type)
{
    for (auto& handler : inventory_handlers) {
        if (handler.type_ == type) {
            return &handler;
        }
    }
    return nullptr;
}


const char* item_description(Item::Type type)
{
    if (auto handler = inventory_item_handler(type)) {
        return locale_string(handler->description_);

    } else {
        return nullptr;
    }
}


StatePtr InventoryState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    if (pfrm.keyboard().down_transition<Key::alt_2>()) {
        return state_pool_.create<ActiveState>();
    }

    selector_timer_ += delta;

    constexpr auto fade_duration = milliseconds(400);
    if (fade_timer_ < fade_duration) {
        fade_timer_ += delta;

        if (fade_timer_ >= fade_duration) {
            page_text_.emplace(pfrm, OverlayCoord{1, 1});
            page_text_->assign(page_ + 1);
            update_item_description(pfrm, game);
            display_items(pfrm, game);
        }

        pfrm.screen().fade(smoothstep(0.f, fade_duration, fade_timer_));
    }

    if (fade_timer_ > fade_duration / 2) {

        if (pfrm.keyboard().down_transition<Key::left>()) {
            if (selector_coord_.x > 0) {
                selector_coord_.x -= 1;
            } else {
                if (page_ > 0) {
                    page_ -= 1;
                    selector_coord_.x = 4;
                    if (page_text_) {
                        page_text_->assign(page_ + 1);
                    }
                    update_arrow_icons(pfrm);
                    display_items(pfrm, game);
                }
            }
            update_item_description(pfrm, game);

        } else if (pfrm.keyboard().down_transition<Key::right>()) {
            if (selector_coord_.x < 4) {
                selector_coord_.x += 1;
            } else {
                if (page_ < Inventory::pages - 1) {
                    page_ += 1;
                    selector_coord_.x = 0;
                    if (page_text_) {
                        page_text_->assign(page_ + 1);
                    }
                    update_arrow_icons(pfrm);
                    display_items(pfrm, game);
                }
            }
            update_item_description(pfrm, game);

        } else if (pfrm.keyboard().down_transition<Key::down>()) {
            if (selector_coord_.y < 1) {
                selector_coord_.y += 1;
            }
            update_item_description(pfrm, game);

        } else if (pfrm.keyboard().down_transition<Key::up>()) {
            if (selector_coord_.y > 0) {
                selector_coord_.y -= 1;
            }
            update_item_description(pfrm, game);
        }
    }

    if (pfrm.keyboard().down_transition<Key::action_2>()) {

        const auto item = game.inventory().get_item(
            page_, selector_coord_.x, selector_coord_.y);

        if (auto handler = inventory_item_handler(item)) {
            if (handler->single_use_) {
                consume_selected_item(game);
            }
            if (auto new_state = handler->callback_(pfrm, game)) {
                return new_state;
            } else {
                update_item_description(pfrm, game);
                display_items(pfrm, game);
            }
        }
    }

    if (selector_timer_ > milliseconds(75)) {
        selector_timer_ = 0;
        const OverlayCoord pos{static_cast<u8>(3 + selector_coord_.x * 5),
                               static_cast<u8>(3 + selector_coord_.y * 5)};
        if (selector_shaded_) {
            selector_.emplace(pfrm, OverlayCoord{4, 4}, pos, false, 8);
            selector_shaded_ = false;
        } else {
            selector_.emplace(pfrm, OverlayCoord{4, 4}, pos, false, 16);
            selector_shaded_ = true;
        }
    }

    return null_state();
}


InventoryState::InventoryState(bool fade_in)
{
    if (not fade_in) {
        fade_timer_ = fade_duration_ - Microseconds{1};
    }
}


static void draw_dot_grid(Platform& pfrm)
{
    for (int i = 0; i < 6; ++i) {
        pfrm.set_tile(Layer::overlay, 2 + i * 5, 2, 176);
        pfrm.set_tile(Layer::overlay, 2 + i * 5, 7, 176);
        pfrm.set_tile(Layer::overlay, 2 + i * 5, 12, 176);
    }
}


void InventoryState::enter(Platform& pfrm, Game& game, State&)
{
    update_arrow_icons(pfrm);
    draw_dot_grid(pfrm);
}


void InventoryState::exit(Platform& pfrm, Game& game, State& next_state)
{
    // Un-fading and re-fading when switching from the inventory view to an item
    // view creates tearing. Only unfade the screen when switching back to the
    // active state.
    if (dynamic_cast<ActiveState*>(&next_state)) {
        pfrm.screen().fade(0.f);
    }

    selector_.reset();
    left_icon_.reset();
    right_icon_.reset();
    page_text_.reset();
    item_description_.reset();
    item_description2_.reset();
    label_.reset();

    pfrm.fill_overlay(0);

    clear_items();
}


void InventoryState::update_arrow_icons(Platform& pfrm)
{
    switch (page_) {
    case 0:
        right_icon_.emplace(pfrm, 172, OverlayCoord{28, 7});
        left_icon_.emplace(pfrm, 175, OverlayCoord{1, 7});
        break;

    case Inventory::pages - 1:
        right_icon_.emplace(pfrm, 174, OverlayCoord{28, 7});
        left_icon_.emplace(pfrm, 173, OverlayCoord{1, 7});
        break;

    default:
        right_icon_.emplace(pfrm, 172, OverlayCoord{28, 7});
        left_icon_.emplace(pfrm, 173, OverlayCoord{1, 7});
        break;
    }
}


void InventoryState::update_item_description(Platform& pfrm, Game& game)

{
    const auto item =
        game.inventory().get_item(page_, selector_coord_.x, selector_coord_.y);

    constexpr static const OverlayCoord text_loc{3, 15};

    if (auto handler = inventory_item_handler(item)) {
        item_description_.emplace(
            pfrm, locale_string(handler->description_), text_loc);
        item_description_->append(".");

        if (handler->single_use_) {
            item_description2_.emplace(
                pfrm,
                locale_string(LocaleString::single_use_warning),
                OverlayCoord{text_loc.x, text_loc.y + 2});
        } else {
            item_description2_.reset();
        }
    }
}


void InventoryState::clear_items()
{
    for (auto& items : item_icons_) {
        for (auto& item : items) {
            item.reset();
        }
    }
}


void InventoryState::display_items(Platform& pfrm, Game& game)
{
    clear_items();

    auto screen_tiles = calc_screen_tiles(pfrm);

    const char* label_str = locale_string(LocaleString::items);
    label_.emplace(
        pfrm, OverlayCoord{u8(screen_tiles.x - (utf8::len(label_str) + 1)), 1});
    label_->assign(label_str);

    for (int i = 0; i < 5; ++i) {
        for (int j = 0; j < 2; ++j) {

            const auto item = game.inventory().get_item(page_, i, j);

            const OverlayCoord coord{static_cast<u8>(4 + i * 5),
                                     static_cast<u8>(4 + j * 5)};

            if (item not_eq Item::Type::null) {
                if (auto handler = inventory_item_handler(item)) {
                    item_icons_[i][j].emplace(pfrm, handler->icon_, coord);
                }
            }
        }
    }
}


////////////////////////////////////////////////////////////////////////////////
// NotebookState
////////////////////////////////////////////////////////////////////////////////

constexpr u16 notebook_margin_tile = 39 + 26 + 6;

NotebookState::NotebookState(const char* text) : str_(text), page_(0)
{
}


void NotebookState::enter(Platform& pfrm, Game&, State&)
{
    pfrm.speaker().play_sound("open_book", 0);

    pfrm.load_overlay_texture("overlay_journal");
    pfrm.screen().fade(1.f, ColorConstant::aged_paper);

    auto screen_tiles = calc_screen_tiles(pfrm);
    text_.emplace(pfrm);
    text_->assign(str_,
                  {1, 2},
                  OverlayCoord{u8(screen_tiles.x - 2), u8(screen_tiles.y - 4)});
    page_number_.emplace(pfrm, OverlayCoord{0, u8(screen_tiles.y - 1)});

    repaint_page(pfrm);
}


void NotebookState::repaint_margin(Platform& pfrm)
{
    auto screen_tiles = calc_screen_tiles(pfrm);

    for (int x = 0; x < screen_tiles.x; ++x) {
        for (int y = 0; y < screen_tiles.y; ++y) {
            if (x == 0 or y == 0 or y == 1 or x == screen_tiles.x - 1 or
                y == screen_tiles.y - 2 or y == screen_tiles.y - 1) {
                pfrm.set_tile(Layer::overlay, x, y, notebook_margin_tile);
            }
        }
    }
}


void NotebookState::repaint_page(Platform& pfrm)
{
    const auto size = text_->size();

    page_number_->erase();
    repaint_margin(pfrm);
    page_number_->assign(page_ + 1);
    text_->assign(str_, {1, 2}, size, page_ * (size.y / 2));
}


void NotebookState::exit(Platform& pfrm, Game&, State&)
{
    pfrm.fill_overlay(0); // The TextView destructor cleans up anyway, but we
                          // have ways of clearing the screen faster than the
                          // TextView implementation is able to. The TextView
                          // class needs to loop through each glyph and
                          // individually zero them out, which can create
                          // tearing in the display. The fill_overlay() function
                          // doesn't need to work around sub-regions of the
                          // screen, so it can use faster methods, like a single
                          // memset, or special BIOS calls (depending on the
                          // platform) to clear out the screen.

    pfrm.screen().fade(1.f);

    text_.reset();
    pfrm.load_overlay_texture("overlay");
}


StatePtr NotebookState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    if (pfrm.keyboard().down_transition<Key::action_2>()) {
        return state_pool_.create<InventoryState>(false);
    }

    if (pfrm.keyboard().down_transition<Key::down>()) {
        if (text_->parsed() not_eq utf8::len(str_)) {
            page_ += 1;
            repaint_page(pfrm);
            pfrm.speaker().play_sound("open_book", 0);
        }

    } else if (pfrm.keyboard().down_transition<Key::up>()) {
        if (page_ > 0) {
            page_ -= 1;
            repaint_page(pfrm);
            pfrm.speaker().play_sound("open_book", 0);
        }
    }

    return null_state();
}

////////////////////////////////////////////////////////////////////////////////
// ImageViewState
////////////////////////////////////////////////////////////////////////////////


ImageViewState::ImageViewState(const char* image_name,
                               ColorConstant background_color)
    : image_name_(image_name), background_color_(background_color)
{
}


StatePtr ImageViewState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    if (pfrm.keyboard().down_transition<Key::action_2>()) {
        return state_pool_.create<InventoryState>(false);
    }

    return null_state();
}


void ImageViewState::enter(Platform& pfrm, Game& game, State&)
{
    pfrm.enable_glyph_mode(false);

    pfrm.screen().fade(1.f, background_color_);
    pfrm.load_overlay_texture(image_name_);

    const auto screen_tiles = calc_screen_tiles(pfrm);

    draw_image(
        pfrm, 1, 1, 1, screen_tiles.x - 2, screen_tiles.y - 3, Layer::overlay);
}


void ImageViewState::exit(Platform& pfrm, Game& game, State&)
{
    pfrm.fill_overlay(0);

    pfrm.screen().fade(1.f);

    pfrm.enable_glyph_mode(true);
    pfrm.load_overlay_texture("overlay");
}


////////////////////////////////////////////////////////////////////////////////
// MapSystemState
////////////////////////////////////////////////////////////////////////////////


void MapSystemState::enter(Platform& pfrm, Game& game, State&)
{
    pfrm.screen().fade(1.f);

    const auto dname = pfrm.device_name();
    map_enter_duration_ = milliseconds(
        Conf(pfrm).expect<Conf::Integer>(dname.c_str(), "minimap_enter_time"));
}


void MapSystemState::exit(Platform& pfrm, Game& game, State&)
{
    pfrm.screen().fade(1.f);
    pfrm.fill_overlay(0);
    level_text_.reset();
    for (auto& text : legend_text_) {
        text.reset();
    }
    legend_border_.reset();
}


StatePtr MapSystemState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    auto screen_tiles = calc_screen_tiles(pfrm);

    auto set_tile = [&](s8 x, s8 y, int icon, bool dodge = true) {
        const auto tile = pfrm.get_tile(Layer::overlay, x + 1, y);
        if (dodge and (tile == 133 or tile == 132)) {
            // ...
        } else {
            pfrm.set_tile(Layer::overlay, x + 1, y, icon);
        }
    };

    auto render_map_icon = [&](Entity& entity, s16 icon) {
        auto t = to_tile_coord(entity.get_position().cast<s32>());
        if (is_walkable__fast(game.tiles().get_tile(t.x, t.y))) {
            set_tile(t.x, t.y, icon);
        }
    };

    switch (anim_state_) {
    case AnimState::map_enter: {
        timer_ += delta;
        const auto current_column =
            std::min(TileMap::width,
                     interpolate(TileMap::width,
                                 (decltype(TileMap::width))0,
                                 Float(timer_) / map_enter_duration_));
        game.tiles().for_each([&](Tile t, s8 x, s8 y) {
            if (x > current_column or x <= last_column_) {
                return;
            }
            bool visited_nearby = false;
            static const int offset = 3;
            for (int x2 = std::max(0, x - (offset + 1));
                 x2 < std::min((int)TileMap::width, x + offset + 1);
                 ++x2) {
                for (int y2 = std::max(0, y - offset);
                     y2 < std::min((int)TileMap::height, y + offset);
                     ++y2) {
                    if (visited.get(x2, y2)) {
                        visited_nearby = true;
                    }
                }
            }
            if (not visited_nearby) {
                set_tile(x, y, is_walkable__fast(t) ? 132 : 133, false);
            } else if (is_walkable__fast(t)) {
                set_tile(x, y, 143, false);
            } else {
                if (is_walkable__fast(game.tiles().get_tile(x, y - 1))) {
                    set_tile(x, y, 140);
                } else {
                    set_tile(x, y, 144, false);
                }
            }
        });
        last_column_ = current_column;

        if (current_column == TileMap::width) {
            timer_ = 0;
            anim_state_ = AnimState::map_decorate;
        }
        break;
    }

    case AnimState::map_decorate: {
        timer_ += delta;
        if (timer_ > milliseconds(32)) {
            game.enemies().transform([&](auto& buf) {
                for (auto& entity : buf) {
                    render_map_icon(*entity, 139);
                }
            });

            render_map_icon(game.transporter(), 141);
            for (auto& chest : game.details().get<ItemChest>()) {
                if (chest->state() not_eq ItemChest::State::opened) {
                    render_map_icon(*chest, 138);
                }
            }

            auto player_tile =
                to_tile_coord(game.player().get_position().cast<s32>());
            //u32 integer_text_length(int n);
            if (not is_walkable__fast(
                    game.tiles().get_tile(player_tile.x, player_tile.y))) {
                // Player movement isn't constrained to tiles exactly, and sometimes the
                // player's map icon displays as inside of a wall.
                if (is_walkable__fast(game.tiles().get_tile(player_tile.x + 1,
                                                            player_tile.y))) {
                    player_tile.x += 1;
                } else if (is_walkable__fast(game.tiles().get_tile(
                               player_tile.x, player_tile.y + 1))) {
                    player_tile.y += 1;
                }
            }
            set_tile(player_tile.x, player_tile.y, 142);

            timer_ = 0;
            anim_state_ = AnimState::wp_text;
        }
        break;
    }

    case AnimState::wp_text:
        timer_ += delta;
        if (timer_ > milliseconds(32)) {
            timer_ = 0;
            anim_state_ = AnimState::legend;

            const char* level_str = locale_string(LocaleString::waypoint_text);

            level_text_.emplace(
                pfrm,
                OverlayCoord{
                    u8(screen_tiles.x - (1 + utf8::len(level_str) +
                                         integer_text_length(game.level()))),
                    1});
            level_text_->assign(level_str);
            level_text_->append(game.level());
        }
        break;

    case AnimState::legend:
        timer_ += delta;
        if (timer_ > milliseconds(32)) {
            timer_ = 0;
            anim_state_ = AnimState::wait;

            set_tile(TileMap::width + 2, 11, 137, false); // you
            set_tile(TileMap::width + 2, 13, 135, false); // enemy
            set_tile(TileMap::width + 2, 15, 136, false); // transporter
            set_tile(TileMap::width + 2, 17, 134, false); // item

            legend_border_.emplace(pfrm,
                                   OverlayCoord{11, 9},
                                   OverlayCoord{TileMap::width + 2, 10},
                                   false,
                                   8);

            for (size_t i = 0; i < legend_strings.size(); ++i) {
                const u8 y = 11 + (i * 2);
                legend_text_[i].emplace(pfrm,
                                        locale_string(legend_strings[i]),
                                        OverlayCoord{TileMap::width + 5, y});
            }
        }
        break;

    case AnimState::wait:
        if (pfrm.keyboard().down_transition<Key::action_2>()) {
            return state_pool_.create<InventoryState>(false);
        }
        break;
    }

    return null_state();
}


////////////////////////////////////////////////////////////////////////////////
// NewLevelState
////////////////////////////////////////////////////////////////////////////////


void NewLevelState::enter(Platform& pfrm, Game& game, State&)
{
    visited.clear();

    pfrm.sleep(15);

    pfrm.screen().fade(1.f);

    const auto s_tiles = calc_screen_tiles(pfrm);

    auto zone = zone_info(next_level_);
    auto last_zone = zone_info(next_level_ - 1);
    if (not(zone == last_zone) or next_level_ == 0) {

        pos_ = OverlayCoord{1, u8(s_tiles.y * 0.3f)};
        text_[0].emplace(pfrm, pos_);

        pos_.y += 2;

        text_[1].emplace(pfrm, pos_);

        const auto l1str = locale_string(zone.title_line_1);
        const auto margin = centered_text_margins(pfrm, utf8::len(l1str));
        left_text_margin(*text_[0], std::max(0, int{margin} - 1));

        text_[0]->append(l1str);

        const auto l2str = locale_string(zone.title_line_2);
        const auto margin2 = centered_text_margins(pfrm, utf8::len(l2str));
        left_text_margin(*text_[1], std::max(0, int{margin2} - 1));

        text_[1]->append(l2str);

        pfrm.sleep(5);

    } else {
        text_[0].emplace(pfrm, OverlayCoord{1, u8(s_tiles.y - 2)});
        text_[0]->append(locale_string(LocaleString::waypoint_text));
        text_[0]->append(next_level_);
        pfrm.sleep(60);
    }
}


static bool startup = true;


StatePtr NewLevelState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    auto zone = zone_info(next_level_);
    auto last_zone = zone_info(next_level_ - 1);

    auto sound_sync_hack = [&pfrm] {
        // FIXME!!!!!! Mysteriously, there's a weird audio glitch, where the
        // sound effects, but not the music, get all glitched out until two
        // sounds are played consecutively. I've spent hours trying to figure
        // out what's going wrong, and I haven't solved this one yet, so for
        // now, just play a couple quiet sounds.
        pfrm.speaker().play_sound("footstep1", 0);
        pfrm.speaker().play_sound("footstep2", 0);
    };


    if (not(zone == last_zone) or next_level_ == 0) {

        timer_ += delta;

        const auto max_j =
            (int)utf8::len(locale_string(zone.title_line_2)) / 2 + 1;
        const auto max_i = max_j * 8;

        const int i = ease_out(timer_, 0, max_i, seconds(1));

        auto repaint = [&pfrm, this](int max_i) {
            while (true) {
                int i = 0, j = 0;
                auto center = calc_screen_tiles(pfrm).x / 2 - 1;

                while (true) {
                    const int y_off = 3;

                    if (max_i > i + 7 + j * 8) {
                        pfrm.set_tile(
                            Layer::overlay, center - j, pos_.y - y_off, 107);
                        pfrm.set_tile(
                            Layer::overlay, center - j, pos_.y + 2, 107);

                        pfrm.set_tile(Layer::overlay,
                                      center + 1 + j,
                                      pos_.y - y_off,
                                      107);
                        pfrm.set_tile(
                            Layer::overlay, center + 1 + j, pos_.y + 2, 107);

                        i = 0;
                        j += 1;
                        continue;
                    }

                    if (j * 8 + i > max_i) {
                        return;
                    }

                    pfrm.set_tile(
                        Layer::overlay, center - j, pos_.y - y_off, 93 + i);
                    pfrm.set_tile(
                        Layer::overlay, center - j, pos_.y + 2, 93 + i);

                    pfrm.set_tile(Layer::overlay,
                                  center + 1 + j,
                                  pos_.y - y_off,
                                  100 + i);
                    pfrm.set_tile(
                        Layer::overlay, center + 1 + j, pos_.y + 2, 100 + i);

                    i++;

                    if (i == 8) {
                        pfrm.set_tile(
                            Layer::overlay, center - j, pos_.y - y_off, 107);
                        pfrm.set_tile(
                            Layer::overlay, center - j, pos_.y + 2, 107);

                        pfrm.set_tile(Layer::overlay,
                                      center + 1 + j,
                                      pos_.y - y_off,
                                      107);
                        pfrm.set_tile(
                            Layer::overlay, center + 1 + j, pos_.y + 2, 107);

                        i = 0;
                        j++;
                    }

                    if (j * 8 + i >= max_i) {
                        return;
                    }
                }
            }
        };

        repaint(std::min(max_i, i));

        if (timer_ > seconds(1)) {
            pfrm.sleep(50);

            pfrm.speaker().play_music(
                zone.music_name_, true, zone.music_offset_);

            if (startup) {
                sound_sync_hack();
            }

            startup = false;
            return state_pool_.create<FadeInState>();
        }

    } else {
        // If we're loading from a save state, we need to start the music, which
        // normally starts when we enter a new zone.
        if (startup) {
            pfrm.speaker().play_music(
                zone.music_name_, true, zone.music_offset_);
            sound_sync_hack();
        }
        startup = false;
        return state_pool_.create<FadeInState>();
    }

    return null_state();
}


void NewLevelState::exit(Platform& pfrm, Game& game, State&)
{
    game.next_level(pfrm, next_level_);

    // Because generating a level takes quite a bit of time (relative to a
    // normal game update step), and because we aren't really running any game
    // logic during level generation, makes sense to zero-out the delta clock,
    // otherwise the game will go into the next update cycle in the new state
    // with a huge delta time.
    DeltaClock::instance().reset();

    text_[0].reset();
    text_[1].reset();

    pfrm.fill_overlay(0);
}


////////////////////////////////////////////////////////////////////////////////
// IntroCreditsState
////////////////////////////////////////////////////////////////////////////////


void IntroCreditsState::center(Platform& pfrm)
{
    // Because the overlay uses 8x8 tiles, to truely center something, you
    // sometimes need to translate the whole layer.
    if (text_ and text_->len() % 2 == 0) {
        pfrm.set_overlay_origin(-4, 0);
    }
}


void IntroCreditsState::enter(Platform& pfrm, Game& game, State&)
{
    pfrm.enable_glyph_mode(true);

    auto pos = (pfrm.screen().size() / u32(8)).cast<u8>();

    // Center horizontally, and place text vertically in top third of screen
    const auto len = utf8::len(str_);
    pos.x -= len + 1;
    pos.x /= 2;
    pos.y *= 0.35f;

    if (len % 2 == 0) {
        pfrm.set_overlay_origin(-4, 0);
    }
    text_.emplace(pfrm, str_, pos);

    center(pfrm);

    pfrm.screen().fade(1.f);
}


void IntroCreditsState::exit(Platform& pfrm, Game& game, State&)
{
    text_.reset();
    pfrm.set_overlay_origin(0, 0);
}


StatePtr IntroLegalMessage::next_state(Platform& pfrm, Game& game)
{
    return state_pool_.create<IntroCreditsState>(
        locale_string(LocaleString::intro_text_2));
}


StatePtr IntroCreditsState::next_state(Platform& pfrm, Game& game)
{
    // backdoor for debugging purposes.
    if (pfrm.keyboard().all_pressed<Key::alt_1, Key::alt_2, Key::start>()) {
        return state_pool_.create<CommandCodeState>();
    }

    if (pfrm.keyboard().pressed<Key::start>()) {
        return state_pool_.create<EndingCreditsState>();
    }

    return state_pool_.create<NewLevelState>(game.level());
}


StatePtr
IntroCreditsState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    center(pfrm);

    timer_ += delta;

    const auto skip = pfrm.keyboard().down_transition<Key::action_2>();

    if (timer_ > seconds(2) + milliseconds(500) or skip) {
        text_.reset();

        if (not skip) {
            pfrm.sleep(70);
        }

        return next_state(pfrm, game);

    } else {
        return null_state();
    }
}


StatePtr State::initial()
{
    return state_pool_.create<IntroLegalMessage>();
}


////////////////////////////////////////////////////////////////////////////////
//
// LogfileViewerState
//
////////////////////////////////////////////////////////////////////////////////


void LogfileViewerState::enter(Platform& pfrm, Game& game, State& prev_state)
{
    repaint(pfrm, 0);
}


void LogfileViewerState::exit(Platform& pfrm, Game& game, State& next_state)
{
    pfrm.screen().fade(0.f);
    pfrm.fill_overlay(0);
}


StatePtr
LogfileViewerState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    auto screen_tiles = calc_screen_tiles(pfrm);

    if (pfrm.keyboard().down_transition<Key::select>()) {
        return state_pool_.create<ActiveState>();
    } else if (pfrm.keyboard().down_transition<Key::down>()) {
        offset_ += screen_tiles.x;
        repaint(pfrm, offset_);
    } else if (pfrm.keyboard().down_transition<Key::up>()) {
        if (offset_ > 0) {
            offset_ -= screen_tiles.x;
            repaint(pfrm, offset_);
        }
    }

    return null_state();
}


Platform::TextureCpMapper locale_texture_map();


void LogfileViewerState::repaint(Platform& pfrm, int offset)
{
    constexpr int buffer_size = 600;
    u8 buffer[buffer_size];

    pfrm.logger().read(buffer, offset, buffer_size);

    auto screen_tiles = calc_screen_tiles(pfrm);

    for (int j = 0; j < screen_tiles.y; ++j) {
        for (int i = 0; i < screen_tiles.x; ++i) {
            const int index = i + j * screen_tiles.x;
            if (index < buffer_size) {
                const auto t =
                    pfrm.map_glyph(buffer[index], locale_texture_map());
                pfrm.set_tile(Layer::overlay, i, j, t);
            }
        }
    }
}


////////////////////////////////////////////////////////////////////////////////
// CommandCodeState FIXME
//
// This code broke after the transition from ascii to unicode. May need some
// more work. But it's not essential to the game in any way.
//
////////////////////////////////////////////////////////////////////////////////


void CommandCodeState::push_char(Platform& pfrm, Game& game, char c)
{
    input_.push_back(c);

    auto screen_tiles = calc_screen_tiles(pfrm);

    input_text_.emplace(pfrm,
                        OverlayCoord{u8(centered_text_margins(pfrm, 10)),
                                     u8(screen_tiles.y / 2 - 2)});

    input_text_->assign(input_.c_str());

    if (input_.full()) {

        // Why am I even bothering with the way that the cheat system's UI
        // will look!? No one will ever even see it... but I'll know how
        // good/bad it looks, and I guess that's a reason...
        while (not selector_shaded_) {
            update_selector(pfrm, milliseconds(20));
        }

        pfrm.sleep(25);

        input_text_->erase();

        if (handle_command_code(pfrm, game)) {
            input_text_->append(" ACCEPTED");

        } else {
            input_text_->append(" REJECTED");
        }

        pfrm.sleep(25);

        input_text_.reset();

        input_.clear();
    }
}


void CommandCodeState::update_selector(Platform& pfrm, Microseconds dt)
{
    selector_timer_ += dt;
    if (selector_timer_ > milliseconds(75)) {
        selector_timer_ = 0;

        auto screen_tiles = calc_screen_tiles(pfrm);

        const auto margin = centered_text_margins(pfrm, 20) - 1;

        const OverlayCoord selector_pos{u8(margin + selector_index_ * 2),
                                        u8(screen_tiles.y - 4)};

        if (selector_shaded_) {
            selector_.emplace(pfrm, OverlayCoord{3, 3}, selector_pos, false, 8);
            selector_shaded_ = false;
        } else {
            selector_.emplace(
                pfrm, OverlayCoord{3, 3}, selector_pos, false, 16);
            selector_shaded_ = true;
        }
    }
}


StatePtr
CommandCodeState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    update_selector(pfrm, delta);

    if (pfrm.keyboard().down_transition<Key::left>()) {
        if (selector_index_ > 0) {
            selector_index_ -= 1;
        } else {
            selector_index_ = 9;
        }

    } else if (pfrm.keyboard().down_transition<Key::right>()) {
        if (selector_index_ < 9) {
            selector_index_ += 1;
        } else {
            selector_index_ = 0;
        }

    } else if (pfrm.keyboard().down_transition<Key::down>()) {
        if (not input_.empty()) {
            while (not input_.full()) {
                input_.push_back(*(input_.end() - 1));
            }

            push_char(pfrm, game, *(input_.end() - 1));
        }
    } else if (pfrm.keyboard().down_transition<Key::action_2>()) {
        push_char(pfrm, game, "0123456789"[selector_index_]);

    } else if (pfrm.keyboard().down_transition<Key::action_1>()) {
        if (input_.empty()) {
            return state_pool_.create<NewLevelState>(next_level_);

        } else {
            input_.pop_back();
            input_text_->assign(input_.c_str());
        }
    }

    return null_state();
}


[[noreturn]] static void factory_reset(Platform& pfrm)
{
    PersistentData data;
    pfrm.write_save_data(&data, sizeof data);
    pfrm.fatal();
}


[[noreturn]] static void debug_boss_level(Platform& pfrm, Level level)
{
    PersistentData data;
    data.level_ = level;
    data.player_health_ = 6;
    pfrm.write_save_data(&data, sizeof data);
    pfrm.fatal();
}


bool CommandCodeState::handle_command_code(Platform& pfrm, Game& game)
{
    // The command interface interprets the leading three digits as a decimal
    // opcode. Each operation will parse the rest of the trailing digits
    // uniquely, if at all.
    const char* input_str = input_.c_str();

    auto to_num = [](char c) { return int{c} - 48; };

    auto opcode = to_num(input_str[0]) * 100 + to_num(input_str[1]) * 10 +
                  to_num(input_str[2]);

    auto single_parameter = [&]() {
        return to_num(input_str[3]) * 1000000 + to_num(input_str[4]) * 100000 +
               to_num(input_str[5]) * 10000 + to_num(input_str[6]) * 1000 +
               to_num(input_str[7]) * 100 + to_num(input_str[8]) * 10 +
               to_num(input_str[9]);
    };

    switch (opcode) {
    case 222: // Jump to next zone
        for (Level level = next_level_; level < 1000; ++level) {
            auto current_zone = zone_info(level);
            auto next_zone = zone_info(level + 1);

            if (not(current_zone == next_zone)) {
                next_level_ = level + 1;
                return true;
            }
        }
        return false;

    case 999: // Jump to the next boss
        for (Level level = next_level_; level < 1000; ++level) {
            if (is_boss_level(level)) {
                next_level_ = level;
                return true;
            }
        }
        return false;

    // For testing boss levels:
    case 1:
        debug_boss_level(pfrm, boss_0_level);
    case 2:
        debug_boss_level(pfrm, boss_1_level);
    case 3:
        debug_boss_level(pfrm, boss_2_level);

    case 100: // Add player health. The main reason that this command doesn't
              // accept a parameter, is that someone could accidentally overflow
              // the health variable and cause bugs.
        game.player().add_health(100);
        return true;

    case 105: // Maybe you want to add some health, but not affect the gameplay
              // too much.
        game.player().add_health(5);
        return true;

    case 200: { // Get item
        const auto item = single_parameter();
        if (item >= static_cast<int>(Item::Type::count) or
            item <= static_cast<int>(Item::Type::coin)) {
            return false;
        }
        game.inventory().push_item(pfrm, game, static_cast<Item::Type>(item));
        return true;
    }

    case 300: // Get all items
        for (int i = static_cast<int>(Item::Type::coin) + 1;
             i < static_cast<int>(Item::Type::count);
             ++i) {

            const auto item = static_cast<Item::Type>(i);

            if (not game.inventory().has_item(item)) {
                game.inventory().push_item(pfrm, game, item);
            }
        }
        return true;

    case 404: // Drop all items
        while (game.inventory().get_item(0, 0, 0) not_eq Item::Type::null) {
            game.inventory().remove_item(0, 0, 0);
        }
        return true;


    default:
        return false;

    case 592:
        factory_reset(pfrm);
    }
}


void CommandCodeState::enter(Platform& pfrm, Game& game, State&)
{
    auto screen_tiles = calc_screen_tiles(pfrm);

    const auto margin = centered_text_margins(pfrm, 20) - 1;

    numbers_.emplace(pfrm,
                     OverlayCoord{u8(margin + 1), u8(screen_tiles.y - 3)});

    for (int i = 0; i < 10; ++i) {
        numbers_->append(i);
        numbers_->append(" ");
    }

    entry_box_.emplace(pfrm,
                       OverlayCoord{12, 3},
                       OverlayCoord{u8(centered_text_margins(pfrm, 12)),
                                    u8(screen_tiles.y / 2 - 3)},
                       false,
                       16);

    next_level_ = game.level() + 1;
}


void CommandCodeState::exit(Platform& pfrm, Game& game, State&)
{
    input_text_.reset();
    entry_box_.reset();
    selector_.reset();
    numbers_.reset();
    pfrm.fill_overlay(0);
}


/////////////////////////////////////////////////////////////////////////////////
// EndingCreditsState
////////////////////////////////////////////////////////////////////////////////


void EndingCreditsState::enter(Platform& pfrm, Game& game, State&)
{
    auto screen_tiles = calc_screen_tiles(pfrm);

    next_y_ = screen_tiles.y + 2;

    game.on_timeout(milliseconds(500), [](Platform& pfrm, Game&) {
        pfrm.speaker().play_music("clair_de_lune", false, 0);
    });
}


void EndingCreditsState::exit(Platform& pfrm, Game& game, State&)
{
    lines_.clear();
    pfrm.set_overlay_origin(0, 0);
    pfrm.speaker().stop_music();
}


// NOTE: '%' characters in the credits will be filled with N '.' characters,
// such that the surrounding text is aligned to either edge of the screen.
//
// FIXME: localize the credits? Nah...
static const std::array<const char*, 32> credits_lines = {
    "Artwork and Source Code by",
    "Evan Bowman",
    "",
    "",
    "Music",
    "Hiraeth%Scott Buckley",
    "Omega%Scott Buckley",
    "Computations%Scott Buckley",
    "September%Kai Engel",
    "Clair De Lune%Chad Crouch",
    "",
    "",
    "Playtesting",
    "Benjamin Casler",
    "",
    "",
    "Special Thanks",
    "My Family",
    "Jasper Vijn (Tonc)",
    "The DevkitARM Project"
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "THE END"};


StatePtr
EndingCreditsState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    timer_ += delta;

    if (timer_ > milliseconds([&] {
            if (pfrm.keyboard().pressed<Key::action_2>()) {
                return 15;
            } else {
                return 60;
            }
        }())) {

        timer_ = 0;

        pfrm.set_overlay_origin(0, scroll_++);

        constexpr int tile_height = 8;
        const bool scrolled_two_lines = scroll_ % (tile_height * 2) == 0;

        if (scrolled_two_lines) {

            auto screen_tiles = calc_screen_tiles(pfrm);

            if (scroll_ > ((screen_tiles.y + 2) * tile_height) and
                not lines_.empty()) {
                lines_.erase(lines_.begin());
            }

            if (next_ < int{credits_lines.size() - 1}) {
                const u8 y =
                    next_y_ % 32; // The overlay tile layer is 32x32 tiles.
                next_y_ += 2;

                const auto str = credits_lines[next_++];


                bool contains_fill_char = false;
                utf8::scan(
                    [&contains_fill_char,
                     &pfrm](const utf8::Codepoint& cp, const char*, int) {
                        if (cp == '%') {
                            if (contains_fill_char) {
                                error(pfrm, "two fill chars not allowed");
                                pfrm.fatal();
                            }
                            contains_fill_char = true;
                        }
                    },
                    str,
                    str_len(str));

                if (utf8::len(str) > u32(screen_tiles.x - 2)) {
                    error(pfrm, "credits text too large");
                    pfrm.fatal();
                }

                if (contains_fill_char) {
                    const auto fill =
                        screen_tiles.x - ((utf8::len(str) - 1) + 2);
                    lines_.emplace_back(pfrm, OverlayCoord{1, y});
                    utf8::scan(
                        [this,
                         fill](const utf8::Codepoint&, const char* raw, int) {
                            if (str_len(raw) == 1 and raw[0] == '%') {
                                for (size_t i = 0; i < fill; ++i) {
                                    lines_.back().append(".");
                                }
                            } else {
                                lines_.back().append(raw);
                            }
                        },
                        str,
                        str_len(str));
                } else {
                    const auto len = utf8::len(str);
                    const u8 left_margin = (screen_tiles.x - len) / 2;
                    lines_.emplace_back(pfrm, OverlayCoord{left_margin, y});
                    lines_.back().assign(str);
                }


            } else if (lines_.size() == screen_tiles.y / 4) {
                pfrm.sleep(160);
                factory_reset(pfrm);
            }
        }
    }

    return null_state();
}


/////////////////////////////////////////////////////////////////////////////////
// PauseScreenState
////////////////////////////////////////////////////////////////////////////////


void PauseScreenState::draw_cursor(Platform& pfrm)
{
    // draw_dot_grid(pfrm);

    if (not resume_text_ or not save_and_quit_text_) {
        return;
    }

    auto draw_cursor = [&pfrm](Text* target, int tile1, int tile2) {
        const auto pos = target->coord();
        pfrm.set_tile(Layer::overlay, pos.x - 2, pos.y, tile1);
        pfrm.set_tile(Layer::overlay, pos.x + target->len() + 1, pos.y, tile2);
    };

    auto [left, right] = [&]() -> Vec2<int> {
        switch (anim_index_) {
        default:
        case 0:
            return {147, 148};
        case 1:
            return {149, 150};
        }
    }();

    switch (cursor_loc_) {
    default:
    case 0:
        draw_cursor(&(*resume_text_), left, right);
        draw_cursor(&(*save_and_quit_text_), 0, 0);
        break;

    case 1:
        draw_cursor(&(*resume_text_), 0, 0);
        draw_cursor(&(*save_and_quit_text_), left, right);
        break;
    }
}


void PauseScreenState::enter(Platform& pfrm, Game& game, State& prev_state)
{
    // ...
}


StatePtr
PauseScreenState::update(Platform& pfrm, Game& game, Microseconds delta)
{
    constexpr auto fade_duration = milliseconds(400);
    if (fade_timer_ < fade_duration) {
        fade_timer_ += delta;

        if (fade_timer_ >= fade_duration) {
            const auto screen_tiles = calc_screen_tiles(pfrm);

            const auto resume_text_len =
                utf8::len(locale_string(LocaleString::menu_resume));

            const auto snq_text_len =
                utf8::len(locale_string(LocaleString::menu_save_and_quit));

            const u8 resume_x_loc = (screen_tiles.x - resume_text_len) / 2;
            const u8 snq_x_loc = (screen_tiles.x - snq_text_len) / 2;

            const u8 y = screen_tiles.y / 2;

            resume_text_.emplace(pfrm, OverlayCoord{resume_x_loc, u8(y - 2)});
            save_and_quit_text_.emplace(pfrm, OverlayCoord{snq_x_loc, u8(y)});

            resume_text_->assign(locale_string(LocaleString::menu_resume));
            save_and_quit_text_->assign(
                locale_string(LocaleString::menu_save_and_quit));

            draw_cursor(pfrm);
        }

        pfrm.screen().fade(smoothstep(0.f, fade_duration, fade_timer_));
    } else {
        anim_timer_ += delta;
        if (anim_timer_ > milliseconds(75)) {
            anim_timer_ = 0;
            if (++anim_index_ > 1) {
                anim_index_ = 0;
            }
            draw_cursor(pfrm);
        }

        if (pfrm.keyboard().down_transition<Key::down>() and cursor_loc_ < 1) {
            cursor_loc_ += 1;
            draw_cursor(pfrm);
        } else if (pfrm.keyboard().down_transition<Key::up>() and
                   cursor_loc_ > 0) {
            cursor_loc_ -= 1;
            draw_cursor(pfrm);
        } else if (pfrm.keyboard().down_transition<Key::action_2>()) {
            switch (cursor_loc_) {
            case 0:
                return state_pool_.create<ActiveState>();
            case 1:
                return state_pool_.create<GoodbyeState>();
            }
        } else if (pfrm.keyboard().down_transition<Key::start>()) {
            return state_pool_.create<ActiveState>();
        }
    }

    if (pfrm.keyboard().pressed<Key::select>()) {
        log_timer_ += delta;
        if (log_timer_ > seconds(1)) {
            return state_pool_.create<LogfileViewerState>();
        }
    } else {
        log_timer_ = 0;
    }

    return null_state();
}


void PauseScreenState::exit(Platform& pfrm, Game& game, State& next_state)
{
    resume_text_.reset();
    save_and_quit_text_.reset();

    pfrm.fill_overlay(0);

    if (dynamic_cast<ActiveState*>(&next_state)) {
        pfrm.screen().fade(0.f);
    }
}


/////////////////////////////////////////////////////////////////////////////////
// GoodbyeState
////////////////////////////////////////////////////////////////////////////////


void GoodbyeState::enter(Platform& pfrm, Game& game, State& prev_state)
{
    pfrm.write_save_data(&game.persistent_data(),
                         sizeof game.persistent_data());

    pfrm.speaker().stop_music();

    const auto s_tiles = calc_screen_tiles(pfrm);
    text_.emplace(pfrm, OverlayCoord{1, u8(s_tiles.y - 2)});
    text_->append(locale_string(LocaleString::goodbye_text));
}


StatePtr GoodbyeState::update(Platform& pfrm, Game&, Microseconds delta)
{
    wait_timer_ += delta;
    if (wait_timer_ > seconds(6)) {
        pfrm.soft_exit();
        pfrm.sleep(10);
    }

    return null_state();
}
