// Minimal stub of nlohmann::json sufficient for local compilation in this project.
// This is NOT a full implementation. It's a tiny, permissive serializer/container
// to allow compiling when system nlohmann/json.hpp is not available.
// For production builds use the official nlohmann/json header (package nlohmann-json3-dev).

#ifndef INCLUDED_MINIMAL_NLOHMANN_JSON_HPP
#define INCLUDED_MINIMAL_NLOHMANN_JSON_HPP

#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <initializer_list>
#include <type_traits>

namespace nlohmann {

class json {
public:
    enum Type { Null, Object, Array, String, Number, Boolean };

    json(): type(Null) {}
    static json object() { json j; j.type = Object; j.obj = {} ; return j; }
    static json array() { json j; j.type = Array; j.arr = {}; return j; }

    // assignment helpers
    json& operator=(const std::string &s) { type = String; str = s; return *this; }
    json& operator=(const char *s) { return operator=(std::string(s)); }
    json& operator=(long long v) { type = Number; num = (double)v; return *this; }
    json& operator=(int v) { return operator=( (long long)v ); }
    json& operator=(double v) { type = Number; num = v; return *this; }
    json& operator=(bool b) { type = Boolean; boolean = b; return *this; }

    // object access
    json& operator[](const std::string &key) {
        if (type != Object) { type = Object; obj = {}; }
        return obj[key];
    }
    // const access (returns by value)
    json operator[](const std::string &key) const {
        if (type != Object) return json();
        auto it = obj.find(key);
        if (it == obj.end()) return json();
        return it->second;
    }

    // contains helper
    bool contains(const std::string &key) const {
        if (type != Object) return false;
        return obj.find(key) != obj.end();
    }

    // array push
    void push_back(const json &v) { if (type != Array) { type = Array; arr.clear(); } arr.push_back(v); }

    // simple dump
    std::string dump() const {
        std::ostringstream o;
        dump_to(o);
        return o.str();
    }

    // parse (very small stub) — returns empty object for any input to allow compilation/runtime stability
    static json parse(const std::string &s) {
        (void)s;
        return json::object();
    }

    // helpers used by code
    bool is_array() const { return type == Array; }
    bool is_null() const { return type == Null; }
    size_t size() const { return (type==Array)?arr.size():0; }

    template<typename T>
    T get() const {
        if constexpr (std::is_same<T, std::string>::value) {
            if (type==String) return str;
            return T();
        } else if constexpr (std::is_same<T, int>::value) {
            if (type==Number) return static_cast<int>(num);
            return 0;
        } else if constexpr (std::is_same<T, long long>::value) {
            if (type==Number) return static_cast<long long>(num);
            return 0LL;
        } else if constexpr (std::is_same<T, double>::value) {
            if (type==Number) return num;
            return 0.0;
        }
        return T();
    }

    std::string value(const std::string &k, const std::string &def) const {
        if (type != Object) return def;
        auto it = obj.find(k);
        if (it == obj.end()) return def;
        if (it->second.type == String) return it->second.str;
        return def;
    }

    // iterator support for arrays
    std::vector<json>::const_iterator begin() const { return arr.begin(); }
    std::vector<json>::const_iterator end() const { return arr.end(); }

private:
    Type type;
    std::map<std::string,json> obj;
    std::vector<json> arr;
    std::string str;
    double num = 0;
    bool boolean = false;

    void dump_to(std::ostringstream &o) const {
        switch(type) {
            case Null: o << "null"; break;
            case String: o << '"' << escape(str) << '"'; break;
            case Number: o << num; break;
            case Boolean: o << (boolean?"true":"false"); break;
            case Array: {
                o << '[';
                bool first=true; for (const auto &v: arr) { if(!first) o<<','; v.dump_to(o); first=false; }
                o << ']'; break; }
            case Object: {
                o << '{'; bool first=true; for (const auto &p: obj) { if(!first) o<<','; o << '"' << escape(p.first) << '"' << ':'; p.second.dump_to(o); first=false; } o<<'}'; break; }
        }
    }

    static std::string escape(const std::string &s) {
        std::ostringstream o; for (char c: s) { switch(c){ case '"': o<<"\\\""; break; case '\\': o<<"\\\\"; break; case '\n': o<<"\\n"; break; default: o<<c; } } return o.str(); }
};

} // namespace nlohmann

#endif // INCLUDED_MINIMAL_NLOHMANN_JSON_HPP
