#pragma once
// ConfigLoader.h - Simple INI-style config file reader.
// Parses [section] / key = value pairs. Lines starting with '#' or ';'
// are comments. Missing keys return the supplied default.
#include <string>
#include <unordered_map>
class ConfigLoader {
public:
    bool Load(const std::string& path);
    bool Save(const std::string& path) const;
    int         GetInt   (const std::string& s,const std::string& k,int         d=0)    const;
    float       GetFloat (const std::string& s,const std::string& k,float       d=0.f)  const;
    bool        GetBool  (const std::string& s,const std::string& k,bool        d=false)const;
    std::string GetString(const std::string& s,const std::string& k,std::string d="")   const;
    void SetInt   (const std::string& s,const std::string& k,int v);
    void SetFloat (const std::string& s,const std::string& k,float v);
    void SetBool  (const std::string& s,const std::string& k,bool v);
    void SetString(const std::string& s,const std::string& k,const std::string& v);
private:
    std::unordered_map<std::string,std::unordered_map<std::string,std::string>> m_data;
    std::string GetRaw(const std::string& s,const std::string& k,const std::string& d)const;
    void        SetRaw(const std::string& s,const std::string& k,const std::string& v);
};
