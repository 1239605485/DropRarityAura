#pragma once
#include "tefkernel/patchlib/field.h"
#include "tefkernel/patchlib/method.h"
#include <vector>
namespace desktop {
// Desktop handles own managed GC roots, unlike Android's raw IL2CPP pointers.
struct Object {
 patch_handle_t value{};
 explicit Object(patch_handle_t v=nullptr):value(v){}
 ~Object(){if(value&&patchlib_free)patchlib_free(value);}
 Object(const Object&)=delete;Object& operator=(const Object&)=delete;
};
struct Metadata {
 std::vector<patch_handle_t> handles;
 patch_handle_t keep(patch_handle_t h){if(h)handles.push_back(h);return h;}
 patch_handle_t type(const char* ns,const char* name){return keep(patchlib_type_get_type(ns,name));}
 patch_handle_t field(patch_handle_t t,const char* name){return t?keep(patchlib_type_get_field(t,name)):nullptr;}
 patch_handle_t method(patch_handle_t t,const char* name,int count){return t?keep(patchlib_type_get_method_by_param_count(t,name,count)):nullptr;}
 void clear(){for(auto h:handles)if(patchlib_free)patchlib_free(h);handles.clear();}
};
template<class T>T read(patch_handle_t f,patch_handle_t object=nullptr){T value{};if(f)patchlib_field_get_value(f,object,&value);return value;}
template<class T>bool call(patch_handle_t m,patch_handle_t object,T& value,void** args=nullptr){return m&&patchlib_method_invoke_args(m,object,&value,args);}
}
