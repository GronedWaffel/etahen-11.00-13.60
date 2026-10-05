// SPDX-License-Identifier: GPL-3.0-or-later
#include "HookedFuncs.hpp"
#include "private-1240-p5.h"

MonoObject* OverlayRoot() {
    if (!AppSystem_img || !pui_img || !Root_Domain) return nullptr;
    auto layer=mono_class_from_name(AppSystem_img,"Sce.Vsh.ShellUI.AppSystem","LayerManager");
    auto scene=mono_class_from_name(pui_img,"Sce.PlayStation.PUI.UI2","Scene");
    if (!layer || !scene) return nullptr;
    auto find=mono_class_get_method_from_name(layer,"FindContainerSceneByPath",1);
    auto prop=mono_class_get_property_from_name(scene,"RootWidget");
    auto get=prop?mono_property_get_get_method(prop):nullptr;
    if (!find || !get) return nullptr;
    void* args[]={mono_string_new(Root_Domain,"Game")};
    MonoObject* exception=nullptr;
    auto current=mono_runtime_invoke(find,nullptr,args,&exception);
    if (exception || !current) return nullptr;
    auto root=mono_runtime_invoke(get,current,nullptr,&exception);
    if(exception){P5Event("overlay RootWidget exception");return nullptr;}
    return root;
}
MonoObject* OverlayFind(MonoObject* root,const char* name) {
    if (!root || !name || !pui_img) return nullptr;
    auto klass=mono_class_from_name(pui_img,"Sce.PlayStation.PUI.UI2","Widget");
    auto method=klass?mono_class_get_method_from_name(klass,"FindWidgetByName",1):nullptr;
    if (!method) return nullptr;
    void* args[]={mono_string_new(Root_Domain,name)};
    MonoObject* exception=nullptr;
    auto widget=mono_runtime_invoke(method,root,args,&exception);
    if(exception){P5Event("overlay FindWidget exception");return nullptr;}
    return widget;
}
bool OverlayText(MonoObject* label,const char* text) {
    if (!label || !text || !pui_img) return false;
    auto klass=mono_class_from_name(pui_img,"Sce.PlayStation.PUI.UI2","Label");
    auto prop=klass?mono_class_get_property_from_name(klass,"Text"):nullptr;
    auto method=prop?mono_property_get_set_method(prop):nullptr;
    if(!method)return false;
    void* args[]={mono_string_new(Root_Domain,text)};
    MonoObject* exception=nullptr;
    mono_runtime_invoke(method,label,args,&exception);
    if(exception)P5Event("overlay Text setter exception");
    return !exception;
}
