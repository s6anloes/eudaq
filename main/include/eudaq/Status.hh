#ifndef EUDAQ_INCLUDED_Status
#define EUDAQ_INCLUDED_Status

#include "eudaq/Serializable.hh"
#include <string>
#include <map>
#include <ostream>
#include "eudaq/Platform.hh"

namespace eudaq {

  class Serializer;
  class Deserializer;

  class DLLEXPORT Status : public Serializable {
  public:
    enum Level {
      LVL_DEBUG,
      LVL_OK,
      LVL_THROW,
      LVL_EXTRA,
      LVL_INFO,
      LVL_WARN,
      LVL_ERROR,
      LVL_USER,
      LVL_BUSY,
      LVL_NONE // The last value, any additions should go before this
    };

    enum State {
      ST_UNCONF,
      ST_CONF,
      ST_RUNNING,
      ST_ERROR
    };

    Status(int level = LVL_OK, const std::string &msg = "",int state = ST_UNCONF)
        : m_level(level), m_msg(msg), m_state(state) {}
    Status(Deserializer &);

    virtual void Serialize(Serializer &) const;

    Status &SetTag(const std::string &name, const std::string &val);
    std::string GetTag(const std::string &name,
                       const std::string &def = "") const;

    static std::string Level2String(int level);
    static std::string State2String(int state);

    static int String2Level(const std::string &);
    static int String2State(const std::string &);

    virtual ~Status() {}
    virtual void print(std::ostream &) const;

    int GetLevel() const { return m_level; }
    int GetState() const { return m_state; }


  protected:
    typedef std::map<std::string, std::string> map_t;

    int m_level;
    int m_state;

    std::string m_msg;
    map_t m_tags; ///< Metadata tags in (name=value) pairs of strings
  };

  inline std::ostream &operator<<(std::ostream &os, const Status &s) {
    s.print(os);
    return os;
  }
}

#endif // EUDAQ_INCLUDED_Status
