#include "../src/item_draw_hook.cpp"
#include <cassert>
#include <cstdio>
struct ItemData {int rare=4,stack=1,type=100;};
struct WorldData {bool active=true;ItemData* inner;Vec2 pos{30,40};int w=16,h=16;};
int draws=0;
bool draw_rarity_beam(float,float,int,RarityColor,float,bool){++draws;return true;}
bool initialize_beam_renderer(){return true;}void reset_beam_renderer(){}
void diagnostics_write(const char*,...){}
bool active(void* w,const void*){return static_cast<WorldData*>(w)->active;}
void* field(void* id,void* obj){switch((uintptr_t)id){
case 1:return &static_cast<WorldData*>(obj)->inner;
case 2:return &static_cast<ItemData*>(obj)->rare;
case 3:return &static_cast<WorldData*>(obj)->pos;
case 4:return &static_cast<WorldData*>(obj)->w;
case 5:return &static_cast<WorldData*>(obj)->h;
case 6:return &static_cast<ItemData*>(obj)->stack;
case 7:return &static_cast<ItemData*>(obj)->type;
}return nullptr;}
int main(){
 g_inner=(void*)1;g_rare=(void*)2;g_position=(void*)3;g_width=(void*)4;g_height=(void*)5;g_stack=(void*)6;g_type=(void*)7;g_active=active;patchlib_field_get_pointer=field;
 ItemData item;WorldData w;w.inner=&item;void* object=&w;int slot=3;void* args[]={&object,&slot};
 // Match KernelLoader's dispatcher: prefix true skips the original.
 int originals=0;
 if(!draw_item_prefix(nullptr,args,nullptr,nullptr))++originals;
 draw_item_postfix(nullptr,args,nullptr,nullptr);
 assert(originals==1&&draws==2);
 draws=1;
 w.active=false;draw_item_postfix(nullptr,args,nullptr,nullptr);assert(draws==1);
 w.active=true;item.stack=0;draw_item_postfix(nullptr,args,nullptr,nullptr);assert(draws==1);
 item.stack=1;item.type=0;draw_item_postfix(nullptr,args,nullptr,nullptr);assert(draws==1);
 item.type=100;draw_item_postfix(nullptr,args,nullptr,nullptr);assert(draws==2);
 puts("PASS: pickup/inactive/zero-stack/empty-type stop drawing; slot reuse resumes only for a valid live item");
}
