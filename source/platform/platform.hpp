#pragma once

#include "number/numeric.hpp"
#include "save.hpp"
#include "graphics/sprite.hpp"
#include "graphics/view.hpp"
#include "sound.hpp"
#include "tileMap.hpp"
#include <array>
#include <optional>


// Anything platform specific should be defined here.


////////////////////////////////////////////////////////////////////////////////
// DeltaClock
////////////////////////////////////////////////////////////////////////////////


class DeltaClock {
public:
    static DeltaClock& instance()
    {
        static DeltaClock inst;
        return inst;
    }

    ~DeltaClock();

    Microseconds reset();

private:
    DeltaClock();

    void* impl_;
};


enum class Key {
    action_1,
    action_2,
    start,
    left,
    right,
    up,
    down,
    alt_1,
    alt_2,
    count
};


////////////////////////////////////////////////////////////////////////////////
// Platform
////////////////////////////////////////////////////////////////////////////////


class Platform {
public:

    class Screen;
    class Keyboard;
    class Logger;
    class Speaker;


    inline Screen& screen()
    {
        return screen_;
    }

    inline Keyboard& keyboard()
    {
        return keyboard_;
    }

    inline Logger& logger()
    {
        return logger_;
    }

    inline Speaker& speaker()
    {
        return speaker_;
    }

    void push_map(const TileMap& map);


    // Sleep halts the game for an amount of time equal to some number
    // of game updates. Given that the game should be running at
    // 60fps, one update equals 1/60 of a second.
    void sleep(u32 frames);


    bool is_running() const;


    bool write_save(const SaveData& data);
    std::optional<SaveData> read_save();


    ////////////////////////////////////////////////////////////////////////////
    // Screen
    ////////////////////////////////////////////////////////////////////////////


    class Screen {
    public:
        static constexpr u32 sprite_limit = 128;

        void draw(const Sprite& spr);

        void clear();

        void display();

        Vec2<u32> size() const;

        void set_view(const View& view)
        {
            view_ = view;
        }

        const View& get_view() const
        {
            return view_;
        }

        void fade(float amount, ColorConstant = ColorConstant::rich_black);

    private:
        Screen();

        friend class Platform;

        View view_;
        void* userdata_;
    };


    ////////////////////////////////////////////////////////////////////////////
    // Keyboard
    ////////////////////////////////////////////////////////////////////////////


    class Keyboard {
    public:
        void poll();

        template <Key k> bool pressed() const
        {
            return states_[int(k)];
        }

        template <Key k> bool down_transition() const
        {
            return states_[int(k)] and not prev_[int(k)];
        }

        template <Key k> bool up_transition() const
        {
            return not states_[int(k)] and prev_[int(k)];
        }

    private:
        Keyboard()
        {
            for (int i = 0; i < int(Key::count); ++i) {
                states_[i] = false;
                prev_[i] = false;
            }
        }

        friend class Platform;

        std::array<bool, int(Key::count)> prev_;
        std::array<bool, int(Key::count)> states_;
    };


    ////////////////////////////////////////////////////////////////////////////
    // Logger
    ////////////////////////////////////////////////////////////////////////////


    class Logger {
    public:
        enum class Severity { info, warning, error };

        void log(Severity severity, const char* msg);

    private:
        friend class Platform;
    };


    ////////////////////////////////////////////////////////////////////////////
    // Speaker
    ////////////////////////////////////////////////////////////////////////////

    class Speaker {
    public:
        using Channel = int;

        void play(Note n, Octave o, Channel c);

    private:
        friend class Platform;

        Speaker();

    };

private:
    Platform();

    friend int main();

    Screen screen_;
    Keyboard keyboard_;
    Speaker speaker_;
    Logger logger_;
};


#ifdef __BLINDJUMP_ENABLE_LOGS
#ifdef __GBA__
// #pragma message "Warning: logging can wear down Flash memory, be careful using this on physical hardware!"
#endif
inline void info(Platform& pf, const char* msg)
{
    pf.logger().log(Platform::Logger::Severity::info, msg);
}
inline void warning(Platform& pf, const char* msg)
{
    pf.logger().log(Platform::Logger::Severity::warning, msg);
}
inline void error(Platform& pf, const char* msg)
{
    pf.logger().log(Platform::Logger::Severity::error, msg);
}
#else
inline void info(Platform&, const char*)
{
}
inline void warning(Platform&, const char*)
{
}
inline void error(Platform&, const char*)
{
}
#endif // __BLINDJUMP_ENABLE_LOGS