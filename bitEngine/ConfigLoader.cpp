// ConfigLoader.cpp - INI config file parser.
#include "ConfigLoader.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
static std::string Trim(const std::string& s){
    auto a=s.find_first_not_of(" \t\r\n");
    if(a==std::string::npos)return{};
    auto b=s.find_last_not_of(" \t\r\n");
    return s.substr(a,b-a+1);
}
static std::string Lo(std::string s){
    std::transform(s.begin(),s.end(),s.begin(),::tolower);return s;
}
bool ConfigLoader::Load(const std::string& path){
    std::ifstream f(path);
    if(!f.is_open()){std::cout<<"[Config] "<<path<<" not found, using defaults.\n";return false;}
    std::string sec,line;
    while(std::getline(f,line)){
        line=Trim(line);
        if(line.empty()||line[0]=='#'||line[0]==';')continue;
        if(line.front()=='['&&line.back()==']'){sec=Lo(line.substr(1,line.size()-2));continue;}
        auto eq=line.find('=');if(eq==std::string::npos)continue;
        m_data[sec][Lo(Trim(line.substr(0,eq)))]=Trim(line.substr(eq+1));
    }
    std::cout<<"[Config] Loaded "<<path<<"\n";return true;
}
bool ConfigLoader::Save(const std::string& path) const{
    std::ofstream f(path);if(!f.is_open())return false;
    for(auto&[s,p]:m_data){f<<"["<<s<<"]\n";for(auto&[k,v]:p)f<<k<<" = "<<v<<"\n";f<<"\n";}
    return true;
}
std::string ConfigLoader::GetRaw(const std::string& s,const std::string& k,const std::string& d)const{
    auto si=m_data.find(Lo(s));if(si==m_data.end())return d;
    auto ki=si->second.find(Lo(k));if(ki==si->second.end())return d;
    return ki->second;
}
void ConfigLoader::SetRaw(const std::string& s,const std::string& k,const std::string& v){m_data[Lo(s)][Lo(k)]=v;}
int         ConfigLoader::GetInt   (const std::string& s,const std::string& k,int d)const
            {auto r=GetRaw(s,k,"");if(r.empty())return d;try{return std::stoi(r);}catch(...){return d;}}
float       ConfigLoader::GetFloat (const std::string& s,const std::string& k,float d)const
            {auto r=GetRaw(s,k,"");if(r.empty())return d;try{return std::stof(r);}catch(...){return d;}}
bool        ConfigLoader::GetBool  (const std::string& s,const std::string& k,bool d)const
            {auto r=Lo(GetRaw(s,k,""));if(r.empty())return d;return r=="true"||r=="1"||r=="yes";}
std::string ConfigLoader::GetString(const std::string& s,const std::string& k,std::string d)const{return GetRaw(s,k,d);}
void ConfigLoader::SetInt   (const std::string& s,const std::string& k,int v)             {SetRaw(s,k,std::to_string(v));}
void ConfigLoader::SetFloat (const std::string& s,const std::string& k,float v)           {SetRaw(s,k,std::to_string(v));}
void ConfigLoader::SetBool  (const std::string& s,const std::string& k,bool v)            {SetRaw(s,k,v?"true":"false");}
void ConfigLoader::SetString(const std::string& s,const std::string& k,const std::string& v){SetRaw(s,k,v);}
