#ifndef VT_NAMED_OBJECT_H_
#define VT_NAMED_OBJECT_H_

#include <string>

namespace vt {

class NamedObject
{
public:
    const std::string get_name() const
    {
        return m_name;
    }
    void set_name(std::string name)
    {
        m_name = name;
    }

protected:
    std::string m_name;

    NamedObject(std::string name);
};

struct FindByName : std::unary_function<NamedObject*, std::string>
{
    std::string m_key;

    FindByName(std::string key)
    {
        m_key = key;
    }

    bool operator()(const NamedObject* x) const
    {
        return x->get_name() == m_key;
    }
};

}

#endif
