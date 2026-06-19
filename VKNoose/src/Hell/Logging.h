#pragma once
#include <string>
#include <sstream>

#ifdef ERROR
#undef ERROR
#endif

namespace Logging {
    enum struct Level {
        INIT,
        ERROR,
        WARNING,
        DEBUG,
        FATAL,
        TODO,
        FUNCTION
    };
    
    struct MessageStream {
        explicit MessageStream(Level level);
        ~MessageStream();

        MessageStream(const MessageStream&) = delete;
        MessageStream& operator=(const MessageStream&) = delete;
        MessageStream(MessageStream&&) noexcept;
        MessageStream& operator=(MessageStream&&) noexcept;

        //template<class T>
        //MessageStream& operator<<(const T& v) { if (m_enabled) m_ss << v; return *this; }
        //
        //using Manip = std::ostream& (*)(std::ostream&);
        //MessageStream& operator<<(Manip m) { if (m_enabled) m_ss << m; return *this; }

        template<class T, std::enable_if_t<!std::is_enum_v<T>, int> = 0>
        MessageStream& operator<<(const T& v) { if (m_enabled) m_ss << v; return *this; }

        template<class E, std::enable_if_t<std::is_enum_v<E>, int> = 0>
        MessageStream& operator<<(E e) {
            if (m_enabled) m_ss << static_cast<std::underlying_type_t<E>>(e);
            return *this;
        }

    private:
        Level m_level;
        bool m_enabled = false;
        bool m_moved = false;
        std::ostringstream m_ss;
    };
    
    void EnableLevel(Level level);
    MessageStream Message(Level level);

    inline MessageStream Init()             { return MessageStream(Level::INIT); }
    inline MessageStream Debug()            { return MessageStream(Level::DEBUG); }
    inline MessageStream Warning()          { return MessageStream(Level::WARNING); }
    inline MessageStream Error()            { return MessageStream(Level::ERROR); }
    inline MessageStream Fatal()            { return MessageStream(Level::FATAL); }
    inline MessageStream ToDo()             { return MessageStream(Level::TODO); }
    inline MessageStream Function()         { return MessageStream(Level::FUNCTION); }
}