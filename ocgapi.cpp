/*
 * interface.cpp
 *
 *  Created on: 2010-5-2
 *      Author: Argon
 */
#include <cstdio>
#include <cstring>
#include <set>
#include "ocgapi.h"
#include "duel.h"
#include "card.h"
#include "group.h"
#include "effect.h"
#include "field.h"
#include "interpreter.h"
#include "buffer.h"

static uint32_t default_card_reader(uint32_t code, card_data* data) {
	return 0;
}
static uint32_t default_message_handler(intptr_t pduel, uint32_t message_type) {
	return 0;
}
static uint32_t default_card_reader_random(std::vector<int>& data ,uint32_t count, uint32_t type, bool is_include) {
	return 0;
}
static script_reader sreader = default_script_reader;
static card_reader creader = default_card_reader;
static card_reader_random rcreader = default_card_reader_random;
static message_handler mhandler = default_message_handler;
static byte buffer[0x100000];
static std::set<duel*> duel_set;
static interpreter* public_lua;
static uint32_t public_seed_sequence[SEED_COUNT]{};
static std::map<card*,card*> xyz_list[2];

OCGCORE_API void set_script_reader(script_reader f) {
	sreader = f;
}
OCGCORE_API void set_card_reader_random(card_reader_random f) {
	rcreader = f;
}
OCGCORE_API void set_card_reader(card_reader f) {
	creader = f;
}
OCGCORE_API void set_message_handler(message_handler f) {
	mhandler = f;
}
byte* read_script(const char* script_name, int* len) {
	return sreader(script_name, len);
}
uint32_t read_card(uint32_t code, card_data* data) {
	if (code == TEMP_CARD_ID) {
		data->clear();
		return 0;
	}
	return creader(code, data);
}
uint32_t read_card_random(std::vector<int>& data,uint32_t count, uint32_t type,bool is_include) {
	return rcreader(data ,count, type, is_include);
}
uint32_t handle_message(void* pduel, uint32_t message_type) {
	return mhandler((intptr_t)pduel, message_type);
}
OCGCORE_API byte* default_script_reader(const char* script_name, int* slen) {
	FILE *fp;
	fp = std::fopen(script_name, "rb");
	if (!fp)
		return nullptr;
	size_t len = std::fread(buffer, 1, sizeof buffer, fp);
	std::fclose(fp);
	if (len >= sizeof buffer)
		return nullptr;
	*slen = (int)len;
	return buffer;
}
OCGCORE_API intptr_t create_duel(uint_fast32_t seed) {
	duel* pduel = new duel();
	duel_set.insert(pduel);
	pduel->random.seed(seed);
	pduel->rng_version = 1;
	return (intptr_t)pduel;
}
OCGCORE_API intptr_t create_duel_v2(uint32_t seed_sequence[]) {
	duel* pduel = new duel();
	duel_set.insert(pduel);
	public_lua = pduel->lua;
	pduel->random.seed(seed_sequence, SEED_COUNT);
	std::memcpy(public_seed_sequence, seed_sequence, sizeof(uint32_t) * SEED_COUNT);
	pduel->rng_version = 2;
	return (intptr_t)pduel;
}
OCGCORE_API intptr_t create_duel_v3() {
	duel* pduel = new duel();
	pduel->lua = public_lua;
	duel_set.insert(pduel);
	pduel->random.seed(public_seed_sequence, SEED_COUNT);
	pduel->rng_version = 2;
	return (intptr_t)pduel;
}
OCGCORE_API void change_lua_duel(intptr_t pduel) {
    duel* target = (duel*)pduel;
	duel* originduel = target->lua->pduel;
	if(originduel == target)
		return;
	// 1. 先将所有卡片表保存到 card_table_refs 中
    lua_pushglobaltable(originduel->lua->lua_state);
    lua_pushnil(originduel->lua->lua_state);
    while (lua_next(originduel->lua->lua_state, -2)) {
        if (lua_type(originduel->lua->lua_state, -2) == LUA_TSTRING) {
            const char* key = lua_tostring(originduel->lua->lua_state, -2);
            if (key[0] == 'c' && isdigit(key[1])) {
                // 尝试从变量名中提取卡片编号
                uint32_t code = 0;
                if (sscanf(key + 1, "%u", &code) == 1 && code > 0) {
                    // 检查是否已存在此卡片的引用
                    auto it = originduel->card_table_refs.find(code);
                    if (it == originduel->card_table_refs.end()) {
                        // 保存表到 registry 中
                        lua_pushvalue(originduel->lua->lua_state, -1); // 复制表
                        int ref = luaL_ref(originduel->lua->lua_state, LUA_REGISTRYINDEX);
                        // 保存引用到 card_table_refs
                        originduel->card_table_refs[code] = ref;
                    }
                }
            }
        }
        lua_pop(originduel->lua->lua_state, 1);
    }

	lua_getglobal(originduel->lua->lua_state, "Auxiliary"); // +1 Auxiliary
	if (lua_istable(originduel->lua->lua_state, -1)) { // 确认是表
		lua_pushvalue(originduel->lua->lua_state, -1); // 复制表 +1
		int ref = luaL_ref(originduel->lua->lua_state, LUA_REGISTRYINDEX); // -1
		// 保存引用到 card_table_refs，使用 0 作为特殊码
		originduel->card_table_refs[0] = ref;
	}
	lua_pop(originduel->lua->lua_state, 1); // 弹出 Auxiliary

	// 清除 Auxiliary 表
	lua_pushglobaltable(originduel->lua->lua_state); // +1 _G
	lua_pushstring(originduel->lua->lua_state, "Auxiliary"); // +1 "Auxiliary"
	lua_pushnil(originduel->lua->lua_state); // +1 nil
	lua_settable(originduel->lua->lua_state, -3); // -2，设置 _G["Auxiliary"] = nil
	lua_pop(originduel->lua->lua_state, 1); // 弹出 _G

    // 能帮我在这里把所有卡片表放到card_table_refs里吗？这样就不用修改load_card_script了
    lua_pushglobaltable(originduel->lua->lua_state);
    lua_pushnil(originduel->lua->lua_state);
    while (lua_next(originduel->lua->lua_state, -2)) {
        if (lua_type(originduel->lua->lua_state, -2) == LUA_TSTRING) {
            const char* key = lua_tostring(originduel->lua->lua_state, -2);
            if (key[0] == 'c' && isdigit(key[1])) {
                lua_pushvalue(originduel->lua->lua_state, -2); 
                lua_pushnil(originduel->lua->lua_state);
                lua_settable(originduel->lua->lua_state, -5); 
            }
        }
        lua_pop(originduel->lua->lua_state, 1); 
    }
    lua_pop(originduel->lua->lua_state, 1); 

	target->lua->pduel = target;
    std::memcpy(lua_getextraspace(target->lua->lua_state), &target, LUA_EXTRASPACE);

    for (const auto& it : target->card_table_refs) {
		uint32_t code = it.first;
		int ref = it.second;
		
		if (code != 0) {
			// 处理卡片表
			char class_name[20];
			sprintf(class_name, "c%u", code);
			lua_rawgeti(target->lua->lua_state, LUA_REGISTRYINDEX, ref);
			lua_setglobal(target->lua->lua_state, class_name);
		}
	}

	if(target->card_table_refs.find(0) != target->card_table_refs.end()){
		// 特殊处理 Auxiliary 表
		lua_rawgeti(target->lua->lua_state, LUA_REGISTRYINDEX, target->card_table_refs[0]);
		lua_setglobal(target->lua->lua_state, "Auxiliary");
	}else{
		target->lua->load_script("./script/utility.lua");
		target->lua->load_script("./script/procedure.lua");
	}
	

    return;
}
void sync_used_xyz(card* pcard ,card* mat,int32_t playerid) {
	if(xyz_list[playerid].find(pcard) == xyz_list[playerid].end())
		return;
	for(auto& it :xyz_list[playerid][pcard]->xyz_materials){
		if(it && it->data.code == mat->data.code){
			xyz_list[playerid][pcard]->xyz_remove(it);
			xyz_list[playerid][pcard]->pduel->game_field->add_card(0, pcard, LOCATION_GRAVE, 0);
		}
	}
	return;
}
OCGCORE_API void reload_field_info(intptr_t pduel){
	duel* source = (duel*)pduel;
	source->game_field->reload_field_info();
}
OCGCORE_API void copy_duel_data(intptr_t source_pduel, intptr_t spduel1,intptr_t spduel2,uint32_t location){
	duel* source = (duel*)source_pduel;
	duel* target1 = (duel*)spduel1;
	duel* target2 = (duel*)spduel2;

	xyz_list[0].clear();
	xyz_list[1].clear();
	source->card_table_refs.clear();
	change_lua_duel(source_pduel);
	uint32_t options = source->game_field->core.duel_options;
	delete source->game_field;
	source->game_field = new field(source);
	source->game_field->temp_card = source->new_card(TEMP_CARD_ID);
	source->message_buffer.clear();
	source->game_field->player[0].start_count = 0;
	source->game_field->player[1].start_count = 0;
	source->game_field->player[0].draw_count = 0;
	source->game_field->player[1].draw_count = 0;
	// FILE *fp = fopen("error.log", "at");
	// fprintf(fp, "MSG10 %d\n", 123123);
	// fclose(fp);
	for(auto& pcard : source->cards)
		delete pcard;
	for(auto& pgroup : source->groups)
		delete pgroup;
	for(auto& peffect : source->effects)
		delete peffect;
	source->cards.clear();
	source->groups.clear();
	source->effects.clear();
	source->assumes.clear();
	source->sgroups.clear();
	source->uncopy.clear();
	source->effects_map.clear();
	// FILE *fp1 = fopen("error.log", "at");
	// fprintf(fp1, "MSG11 %d\n", 123123);
	// fclose(fp1);
	start_duel(source_pduel, options);
	// for(auto& it : source->effects){
	// 	source->delete_effect(it);
	// }

	source->game_field->infos.turn_id = target1->game_field->infos.turn_id;
	source->game_field->infos.turn_id_by_player[0] = target1->game_field->infos.turn_id_by_player[0];
	source->game_field->infos.turn_id_by_player[1] = target2->game_field->infos.turn_id_by_player[1];
	source->game_field->player[0].lp = target1->game_field->player[0].lp;
	source->game_field->player[1].lp = target2->game_field->player[0].lp;

	copy_field_data(source_pduel, spduel1, location, 0,0);
	copy_field_data(source_pduel, spduel2, location, 1,0);
	// FILE *fp = fopen("error.log", "at");
	// fprintf(fp, "MSG0 %d\n", 123123);
	// fclose(fp);
}
void copy_field_data(intptr_t source_pduel, intptr_t spduel, uint32_t location, uint32_t playerid,uint32_t target_playerid){
	duel* source = (duel*)source_pduel;
	duel* target = (duel*)spduel;
	// FILE *fp = fopen("error.log", "at");
	// fprintf(fp, "MSG0 %d\n", 123123);
	// fclose(fp);

	if(location & LOCATION_SZONE){
		for(int i=0; i < target->game_field->player[target_playerid].list_szone.size(); ++i) {
			if(target->game_field->player[target_playerid].list_szone[i]) {
				card* pcard = target->game_field->player[target_playerid].list_szone[i];
				new_card(source_pduel, pcard->data.code,playerid,playerid, LOCATION_SZONE, pcard->current.sequence, pcard->current.position);
			}
		}
	}
	if(location & LOCATION_MZONE){
		for(int i=0; i < target->game_field->player[target_playerid].list_mzone.size(); ++i) {
			if(target->game_field->player[target_playerid].list_mzone[i]) {
				card* pcard = target->game_field->player[target_playerid].list_mzone[i];
				new_card(source_pduel, pcard->data.code,playerid,playerid,LOCATION_MZONE,pcard->current.sequence,pcard->current.position);
				card* new_card = source->game_field->player[playerid].list_mzone[i];
				for(auto& pc :pcard->xyz_materials){
					if(pc){
						card* mat = source->new_card(pc->data.code);
						pc->owner = playerid;
						new_card->xyz_add(mat);
					}
				}
				if(pcard->data.type & TYPE_XYZ){
					card* new_card = source->game_field->player[playerid].list_mzone[i];
					xyz_list[playerid][new_card] = pcard;
				}
			}
		}
	}
	if(location & LOCATION_HAND){
		for(int i=0; i < target->game_field->player[target_playerid].list_hand.size(); ++i) {
			if(target->game_field->player[target_playerid].list_hand[i]) {
				card* pcard = target->game_field->player[target_playerid].list_hand[i];
				new_card(source_pduel, pcard->data.code,playerid,playerid,LOCATION_HAND,pcard->current.sequence,pcard->current.position);
			}
		}
	}
	if(location & LOCATION_GRAVE){
		for(int i=0; i < target->game_field->player[target_playerid].list_grave.size(); ++i) {
			if(target->game_field->player[target_playerid].list_grave[i]) {
				card* pcard = target->game_field->player[target_playerid].list_grave[i];
				new_card(source_pduel, pcard->data.code,playerid,playerid,LOCATION_GRAVE,pcard->current.sequence,pcard->current.position);
			}
		}
	}
	if(location & LOCATION_REMOVED){
		for(int i=0; i < target->game_field->player[target_playerid].list_remove.size(); ++i) {
			if(target->game_field->player[target_playerid].list_remove[i]) {
				card* pcard = target->game_field->player[target_playerid].list_remove[i];
				new_card(source_pduel, pcard->data.code,playerid,playerid,LOCATION_REMOVED,pcard->current.sequence,pcard->current.position);
			}
		}
	}
	if(location & LOCATION_EXTRA){
		for(int i=0; i < target->game_field->player[target_playerid].list_extra.size(); ++i) {
			if(target->game_field->player[target_playerid].list_extra[i]) {
				card* pcard = target->game_field->player[target_playerid].list_extra[i];
				new_card(source_pduel, pcard->data.code,playerid,playerid,LOCATION_EXTRA,pcard->current.sequence,pcard->current.position);
			}
		}
	}
	if(location & LOCATION_DECK){
		for(int i=0; i < target->game_field->player[target_playerid].list_main.size(); ++i) {
			if(target->game_field->player[target_playerid].list_main[i]) {
				card* pcard = target->game_field->player[target_playerid].list_main[i];
				new_card(source_pduel, pcard->data.code,playerid,playerid,LOCATION_DECK,pcard->current.sequence,pcard->current.position);
			}
		}
	}

	// FILE *fp1 = fopen("error.log", "at");
	// fprintf(fp1, "MSG1 %d\n", 123123);
	// fclose(fp1);

	std::map<int, effect*> effects_map;
	for(int i=0; i < source->game_field->player[playerid].list_szone.size(); ++i) {
		if(source->game_field->player[playerid].list_szone[i]) {
			card* pcard = target->game_field->player[target_playerid].list_szone[i];
			card* new_card = source->game_field->player[playerid].list_szone[i];
			for(auto& it : pcard->indexer){
				if(!it.first->is_init_effect){
					effect* new_effect = new_card->pduel->new_effect();
					effect_data_copy(new_effect, it.first, playerid, target_playerid);
					if(!new_effect->owner){
						source->delete_effect(new_effect);
						continue;
					}
					new_card->add_effect(new_effect);
					
					effects_map[it.first->clone_id] = new_effect;
				}else{
					effects_map[it.first->clone_id] = new_card->init_effect_list[it.first->init_clone_id];
				}
			}
		}
	}
	for(int i=0; i < source->game_field->player[playerid].list_mzone.size(); ++i) {
		if(source->game_field->player[playerid].list_mzone[i]) {
			card* pcard = target->game_field->player[target_playerid].list_mzone[i];
			card* new_card = source->game_field->player[playerid].list_mzone[i];

			for(auto& it : pcard->indexer){
				if(!it.first->is_init_effect){
					effect* new_effect = new_card->pduel->new_effect();
					effect_data_copy(new_effect, it.first, playerid, target_playerid);
					if(!new_effect->owner){
						source->delete_effect(new_effect);
						continue;
					}
					new_card->add_effect(new_effect);
					
					effects_map[it.first->clone_id] = new_effect;
				}else{
					effects_map[it.first->clone_id] = new_card->init_effect_list[it.first->init_clone_id];
				}
			}
			for(int i=0; i < new_card->xyz_materials.size(); ++i) {

				for(auto& it : pcard->xyz_materials[i]->indexer){
					if(!it.first->is_init_effect){
						effect* new_effect = new_card->pduel->new_effect();
						effect_data_copy(new_effect, it.first, playerid, target_playerid);
						if(!new_effect->owner){
							source->delete_effect(new_effect);
							continue;
						}
						new_card->add_effect(new_effect);
						
						effects_map[it.first->clone_id] = new_effect;
					}else{
						effects_map[it.first->clone_id] = new_card->init_effect_list[it.first->init_clone_id];
					}
				}
			}
		}
	}
	for(int i=0; i < source->game_field->player[playerid].list_hand.size(); ++i) {
		if(source->game_field->player[playerid].list_hand[i]) {
			card* pcard = target->game_field->player[target_playerid].list_hand[i];
			card* new_card = source->game_field->player[playerid].list_hand[i];
			// FILE *fp1 = fopen("error.log", "at");
			// fprintf(fp1, "MSGhand %d\n", new_card->data.code);
			// fclose(fp1);
			for(auto& it : pcard->indexer){
				if(!it.first->is_init_effect){
					effect* new_effect = new_card->pduel->new_effect();
					effect_data_copy(new_effect, it.first, playerid, target_playerid);
					if(!new_effect->owner){
						source->delete_effect(new_effect);
						continue;
					}
					new_card->add_effect(new_effect);
					
					effects_map[it.first->clone_id] = new_effect;
				}else{
					effects_map[it.first->clone_id] = new_card->init_effect_list[it.first->init_clone_id];
				}
			}
		}
	}
	for(int i=0; i < source->game_field->player[playerid].list_grave.size(); ++i) {
		if(source->game_field->player[playerid].list_grave[i]) {
			card* pcard = target->game_field->player[target_playerid].list_grave[i];
			card* new_card = source->game_field->player[playerid].list_grave[i];
			for(auto& it : pcard->indexer){
				if(!it.first->is_init_effect){
					effect* new_effect = new_card->pduel->new_effect();
					effect_data_copy(new_effect, it.first, playerid, target_playerid);
					if(!new_effect->owner){
						source->delete_effect(new_effect);
						continue;
					}
					new_card->add_effect(new_effect);
					
					effects_map[it.first->clone_id] = new_effect;
				}else{
					effects_map[it.first->clone_id] = new_card->init_effect_list[it.first->init_clone_id];
				}
			}
		}
	}
	for(int i=0; i < source->game_field->player[playerid].list_remove.size(); ++i) {
		if(source->game_field->player[playerid].list_remove[i]) {
			card* pcard = target->game_field->player[target_playerid].list_remove[i];
			card* new_card = source->game_field->player[playerid].list_remove[i];
			for(auto& it : pcard->indexer){
				if(!it.first->is_init_effect){
					effect* new_effect = new_card->pduel->new_effect();
					effect_data_copy(new_effect, it.first, playerid, target_playerid);
					if(!new_effect->owner){
						source->delete_effect(new_effect);
						continue;
					}
					new_card->add_effect(new_effect);
					
					effects_map[it.first->clone_id] = new_effect;
				}else{
					effects_map[it.first->clone_id] = new_card->init_effect_list[it.first->init_clone_id];
				}
			}
		}
	}
	for(int i=0; i < source->game_field->player[playerid].list_extra.size(); ++i) {
		if(source->game_field->player[playerid].list_extra[i]) {
			card* pcard = target->game_field->player[target_playerid].list_extra[i];
			card* new_card = source->game_field->player[playerid].list_extra[i];
			for(auto& it : pcard->indexer){
				if(!it.first->is_init_effect){
					effect* new_effect = new_card->pduel->new_effect();
					effect_data_copy(new_effect, it.first, playerid, target_playerid);
					if(!new_effect->owner){
						source->delete_effect(new_effect);
						continue;
					}
					new_card->add_effect(new_effect);
					
					effects_map[it.first->clone_id] = new_effect;
				}else{
					effects_map[it.first->clone_id] = new_card->init_effect_list[it.first->init_clone_id];
				}
			}
		}
	}
	for(int i=0; i < source->game_field->player[playerid].list_main.size(); ++i) {
		if(source->game_field->player[playerid].list_main[i]) {
			card* pcard = target->game_field->player[target_playerid].list_main[i];
			card* new_card = source->game_field->player[playerid].list_main[i];
			for(auto& it : pcard->indexer){
				if(!it.first->is_init_effect){
					effect* new_effect = new_card->pduel->new_effect();
					effect_data_copy(new_effect, it.first, playerid, target_playerid);
					if(!new_effect->owner){
						source->delete_effect(new_effect);
						continue;
					}
					new_card->add_effect(new_effect);
					
					effects_map[it.first->clone_id] = new_effect;
				}else{
					effects_map[it.first->clone_id] = new_card->init_effect_list[it.first->init_clone_id];
				}
			}
		}
	}
	for(auto& it : target->game_field->effects.indexer){
		if(it.first &&  it.first->owner){
			if(effects_map[it.first->clone_id]){
				source->game_field->add_effect(effects_map[it.first->clone_id], it.first->effect_owner);
			}else{
				if(!it.first->is_init_effect){
					effect* new_effect = source->new_effect();
					effect_data_copy(new_effect, it.first, playerid, target_playerid);
					if(!new_effect->owner){
						source->delete_effect(new_effect);
						continue;
					}
					source->game_field->add_effect(new_effect, it.first->effect_owner);					
					effects_map[it.first->clone_id] = new_effect;
				}else{
					effects_map[it.first->clone_id] = it.first->owner->init_effect_list[it.first->init_clone_id];
				}
			}
		}
	}

	// source->lua->params.clear();
	// FILE *fp2 = fopen("error.log", "at");
	// fprintf(fp2, "MSG2 %d\n", 123123);
	// fclose(fp2);

	for(int i=0; i < source->game_field->player[playerid].list_szone.size(); ++i) {
		if(source->game_field->player[playerid].list_szone[i]) {
			card* pcard = target->game_field->player[target_playerid].list_szone[i];
			card* new_card = source->game_field->player[playerid].list_szone[i];
			// FILE *fp2 = fopen("error.log", "at");
			// fprintf(fp2, "MSG3 %d\n", new_card->data.code);
			// fclose(fp2);
			card_data_copy(new_card, pcard, playerid, target_playerid, effects_map);
		}
	}
	for(int i=0; i < source->game_field->player[playerid].list_mzone.size(); ++i) {
		if(source->game_field->player[playerid].list_mzone[i]) {
			card* pcard = target->game_field->player[target_playerid].list_mzone[i];
			card* new_card = source->game_field->player[playerid].list_mzone[i];
			// FILE *fp2 = fopen("error.log", "at");
			// fprintf(fp2, "MSG4 %d\n", new_card->data.code);
			// fclose(fp2);
			card_data_copy(new_card, pcard, playerid, target_playerid, effects_map);
			for(int i=0; i < new_card->xyz_materials.size(); ++i) {
				card_data_copy(new_card->xyz_materials[i], pcard->xyz_materials[i], playerid, target_playerid, effects_map);
			}
		}
	}
	for(int i=0; i < source->game_field->player[playerid].list_hand.size(); ++i) {
		if(source->game_field->player[playerid].list_hand[i]) {
			card* pcard = target->game_field->player[target_playerid].list_hand[i];
			card* new_card = source->game_field->player[playerid].list_hand[i];
			// FILE *fp2 = fopen("error.log", "at");
			// fprintf(fp2, "MSG5 %d\n", new_card->data.code);
			// fclose(fp2);
			card_data_copy(new_card, pcard, playerid, target_playerid, effects_map);
		}
	}
	for(int i=0; i < source->game_field->player[playerid].list_grave.size(); ++i) {
		if(source->game_field->player[playerid].list_grave[i]) {
			card* pcard = target->game_field->player[target_playerid].list_grave[i];
			card* new_card = source->game_field->player[playerid].list_grave[i];
			// FILE *fp2 = fopen("error.log", "at");
			// fprintf(fp2, "MSG6 %d\n", new_card->data.code);
			// fclose(fp2);
			card_data_copy(new_card, pcard, playerid, target_playerid, effects_map);
		}
	}
	for(int i=0; i < source->game_field->player[playerid].list_remove.size(); ++i) {
		if(source->game_field->player[playerid].list_remove[i]) {
			card* pcard = target->game_field->player[target_playerid].list_remove[i];
			card* new_card = source->game_field->player[playerid].list_remove[i];
			// FILE *fp2 = fopen("error.log", "at");
			// fprintf(fp2, "MSG7 %d\n", new_card->data.code);
			// fclose(fp2);
			card_data_copy(new_card, pcard, playerid, target_playerid, effects_map);
		}
	}
	for(int i=0; i < source->game_field->player[playerid].list_extra.size(); ++i) {
		if(source->game_field->player[playerid].list_extra[i]) {
			card* pcard = target->game_field->player[target_playerid].list_extra[i];
			card* new_card = source->game_field->player[playerid].list_extra[i];
			// FILE *fp2 = fopen("error.log", "at");
			// fprintf(fp2, "MSG8 %d\n", new_card->data.code);
			// fclose(fp2);
			card_data_copy(new_card, pcard, playerid, target_playerid, effects_map);
		}
	}
	for(int i=0; i < source->game_field->player[playerid].list_main.size(); ++i) {
		if(source->game_field->player[playerid].list_main[i]) {
			card* pcard = target->game_field->player[target_playerid].list_main[i];
			card* new_card = source->game_field->player[playerid].list_main[i];
			// FILE *fp2 = fopen("error.log", "at");
			// fprintf(fp2, "MSG9 %d\n", new_card->data.code);
			// fclose(fp2);
			card_data_copy(new_card, pcard, playerid, target_playerid, effects_map);
		}
	}
	for(auto& it : effects_map){
		// FILE *fp = fopen("error.log", "at");
		// fprintf(fp, "MSG1 %d\n", target->effects_map[it.first]->owner->data.code);
		// fprintf(fp, "MSG2 %d\n", target->effects_map[it.first]->object_type);
		// fclose(fp);
		if(!it.second)
			continue;
		if(target->effects_map[it.first]->object_type == PARAM_TYPE_CARD){

			if(target->effects_map[it.first]->label_object == 0)
				continue;
			luaL_checkstack(target->lua->current_state, 1, nullptr);
			lua_rawgeti(target->lua->current_state, LUA_REGISTRYINDEX, target->effects_map[it.first]->label_object);
			if(!lua_isuserdata(target->lua->current_state, -1)){
				lua_pop(target->lua->current_state, 1);
				continue;
			}
			void* p = *(void**)lua_touserdata(target->lua->current_state, -1);
			lua_pop(target->lua->current_state, 1);
			card* pcard = reinterpret_cast<card*>(p);
			if(pcard && pcard->ref_handle != target->effects_map[it.first]->label_object){
				continue;
			}

			if(pcard->current.controler == 0)
				it.second->label_object = (find_card(source, pcard, playerid))->ref_handle;
		}
		if(target->effects_map[it.first]->object_type == PARAM_TYPE_EFFECT){

			if(target->effects_map[it.first]->label_object == 0)
				continue;
			luaL_checkstack(target->lua->current_state, 1, nullptr);
			lua_rawgeti(target->lua->current_state, LUA_REGISTRYINDEX, target->effects_map[it.first]->label_object);
			if(!lua_isuserdata(target->lua->current_state, -1)){
				lua_pop(target->lua->current_state, 1);
				continue;
			}
			void* p = *(void**)lua_touserdata(target->lua->current_state, -1);
			lua_pop(target->lua->current_state, 1);
			effect* new_effect = reinterpret_cast<effect*>(p);
			if(new_effect && new_effect->ref_handle != target->effects_map[it.first]->label_object){
				continue;
			}

			if(new_effect->owner && new_effect->owner->current.controler == 0){
				auto itmap = effects_map.find(new_effect->clone_id);
				if(itmap != effects_map.end() && itmap->second) {
					// 防御性检查目标指针
					if(it.second) {
						it.second->label_object = itmap->second->ref_handle;
					} 
				}
			}
		}
		if(target->effects_map[it.first]->object_type == PARAM_TYPE_GROUP){
			if(it.second->code & EFFECT_SPSUMMON_PROC)
				continue;
			group* return_value = source->new_group();

			if(target->effects_map[it.first]->label_object == 0)
				continue;
			luaL_checkstack(target->lua->current_state, 1, nullptr);
			lua_rawgeti(target->lua->current_state, LUA_REGISTRYINDEX, target->effects_map[it.first]->label_object);
			if(!lua_isuserdata(target->lua->current_state, -1)){
				it.second->label_object = return_value->ref_handle;
				lua_pop(target->lua->current_state, 1);
				continue;
			}
			void* p = *(void**)lua_touserdata(target->lua->current_state, -1);
			lua_pop(target->lua->current_state, 1);
			group* pgroup = reinterpret_cast<group*>(p);
			if(pgroup->ref_handle != target->effects_map[it.first]->label_object){
				continue;
			}

			// 防御性检查：确保 pgroup 非空且看起来是有效的 group
			if(!pgroup || pgroup->ref_handle == 0 || pgroup->pduel == nullptr || pgroup->pduel != target) {
				// userdata 无效或来自其它 duel，返回空 group
				it.second->label_object = return_value->ref_handle;
				continue;
			}
			if(pgroup->container.empty()){
				it.second->label_object = return_value->ref_handle;
				continue;
			}
			std::vector<card*> temp(pgroup->container.begin(), pgroup->container.end());
			for(auto pc : temp) {
				bool in_target = false;
                for(auto& chk : target->cards) {
                    if(chk == pc) { in_target = true; break; }
                }
                if(!in_target) {
                    continue;
                }
				if(pc && pc->current.controler == 0){
					card* new_card = find_card(source, pc, playerid);
					if(new_card)
						return_value->container.insert(new_card);
				}
			}
			it.second->label_object = return_value->ref_handle;
		}
	}
		
}
void card_data_copy(card* new_card, card* pcard, uint32_t playerid ,uint32_t target_playerid ,std::map<int, effect*> effects_map) {
	new_card->q_cache = pcard->q_cache;
	new_card->summon_info = pcard->summon_info;
	new_card->status = pcard->status;
	new_card->sendto_param = pcard->sendto_param;
	new_card->release_param = pcard->release_param;
	new_card->sum_param = pcard->sum_param;
	new_card->position_param = pcard->position_param;
	new_card->spsummon_param= pcard->spsummon_param;
	new_card->to_field_param = pcard->to_field_param;
	new_card->attack_announce_count = pcard->attack_announce_count;
	new_card->direct_attackable = pcard->direct_attackable;
	new_card->announce_count = pcard->announce_count;
	new_card->attacked_count = pcard->attacked_count;
	new_card->attack_all_target = pcard->attack_all_target;
	new_card->attack_controler = pcard->attack_controler;
	
	
	new_card->turnid = pcard->turnid;
	new_card->turn_counter = pcard->turn_counter;
	new_card->unique_pos[0] = pcard->unique_pos[0];
	new_card->unique_pos[1] = pcard->unique_pos[1];

	new_card->unique_code = pcard->unique_code;
	new_card->unique_location = pcard->unique_location;
	new_card->unique_function = pcard->unique_function;
	// unique_effect is not copied, it should be set by the effect
	if(pcard->unique_effect && pcard->unique_effect->owner&& pcard->unique_effect->owner->current.controler == 0 
		&& find_card(new_card->pduel, pcard->unique_effect->owner, playerid) && effects_map.find(pcard->unique_effect->clone_id) != effects_map.end()) {
		new_card->unique_effect = effects_map[pcard->unique_effect->clone_id];
	} else {
		new_card->unique_effect = nullptr;
	}
	new_card->spsummon_code = pcard->spsummon_code;
	new_card->spsummon_counter[0] = pcard->spsummon_counter[0];
	new_card->spsummon_counter[1] = pcard->spsummon_counter[1];
	new_card->assume_type = pcard->assume_type;
	new_card->assume_value = pcard->assume_value;
	if(pcard->equiping_target){
		new_card->equiping_target = find_card(new_card->pduel, pcard->equiping_target, playerid);
	}
	if(pcard->pre_equip_target){
		new_card->pre_equip_target = find_card(new_card->pduel, pcard->pre_equip_target, playerid);
	}
	// if(pcard->overlay_target){
	// 	new_card->overlay_target = find_card(new_card->pduel, pcard->overlay_target, playerid);
	// }
	new_card->relations.clear();
	for(const auto& rel : pcard->relations) {
		card* old_related = rel.first;
		uint32_t rel_val = rel.second;
		card* new_related = find_card(new_card->pduel, old_related, playerid);
		if(new_related)
			new_card->relations[new_related] = rel_val;
	}
	new_card->counters = pcard->counters;
	new_card->indestructable_effects = pcard->indestructable_effects;
	new_card->announced_cards.clear();
	for(const auto& it : pcard->announced_cards) {
		uint32_t fieldid = it.first;
		card* old_ptr = it.second.first;
		uint32_t count = it.second.second;
		card* new_ptr = nullptr;
		if(old_ptr && old_ptr->current.controler == target_playerid)
			new_ptr = find_card(new_card->pduel, old_ptr, playerid);
		new_card->announced_cards[fieldid] = std::make_pair(new_ptr, count);
	}
	new_card->attacked_cards.clear();
	for(const auto& it : pcard->attacked_cards) {
		uint32_t fieldid = it.first;
		card* old_ptr = it.second.first;
		uint32_t count = it.second.second;
		card* new_ptr = nullptr;
		if(old_ptr && old_ptr->current.controler == target_playerid)
			new_ptr = find_card(new_card->pduel, old_ptr, playerid);
		new_card->attacked_cards[fieldid] = std::make_pair(new_ptr, count);
	}
	new_card->battled_cards.clear();
	for(const auto& it : pcard->battled_cards) {
		uint32_t fieldid = it.first;
		card* old_ptr = it.second.first;
		uint32_t count = it.second.second;
		card* new_ptr = nullptr;
		if(old_ptr && old_ptr->current.controler == target_playerid)
			new_ptr = find_card(new_card->pduel, old_ptr, playerid);
		new_card->battled_cards[fieldid] = std::make_pair(new_ptr, count);
	}
	// 拷贝 equiping_cards
	new_card->equiping_cards.clear();
	for(card* old_ptr : pcard->equiping_cards) {
		if(old_ptr->current.controler != target_playerid)
			continue;
		card* new_ptr = find_card(new_card->pduel, old_ptr, playerid);
		if(new_ptr)
			new_card->equiping_cards.insert(new_ptr);
	}

	// 拷贝 material_cards
	new_card->material_cards.clear();
	for(card* old_ptr : pcard->material_cards) {
		if(old_ptr->current.controler != target_playerid)
			continue;
		card* new_ptr = find_card(new_card->pduel, old_ptr, playerid);
		if(new_ptr)
			new_card->material_cards.insert(new_ptr);
	}

	// 拷贝 effect_target_owner
	new_card->effect_target_owner.clear();
	for(card* old_ptr : pcard->effect_target_owner) {
		if(old_ptr->current.controler != target_playerid)
			continue;
		card* new_ptr = find_card(new_card->pduel, old_ptr, playerid);
		if(new_ptr)
			new_card->effect_target_owner.insert(new_ptr);
	}

	// 拷贝 effect_target_cards
	new_card->effect_target_cards.clear();
	for(card* old_ptr : pcard->effect_target_cards) {
		if(old_ptr->current.controler != target_playerid)
			continue;
		card* new_ptr = find_card(new_card->pduel, old_ptr, playerid);
		if(new_ptr)
			new_card->effect_target_cards.insert(new_ptr);
	}

	// card_vector xyz_materials;
	new_card->xyz_materials_previous_count_onfield = pcard->xyz_materials_previous_count_onfield;

	//效果
	// int temp2 = new_card->effect_list.size();
	// for(int i=temp2;i<temp;++i) {
	// 	effect* new_effect = new_card->pduel->new_effect();
	// 	effect_data_copy(new_effect, pcard->effect_list[i], playerid, target_playerid);
	// 	new_card->add_effect(new_effect);
	// }

	new_card->filter_immune_effect();
	new_card->relate_effect.clear();
	for(const auto& rel : pcard->relate_effect) {
		if(rel.first && rel.first->owner&& rel.first->owner->current.controler == 0 
			&& find_card(new_card->pduel, rel.first->owner, playerid) && effects_map.find(rel.first->clone_id) != effects_map.end()) {
			new_card->relate_effect.emplace(effects_map[rel.first->clone_id], rel.second);
		}
	}

	new_card->previous = pcard->previous;
	new_card->previous.controler = playerid;
	if(pcard->previous.reason_card && pcard->previous.reason_card->current.controler == target_playerid) {
		new_card->previous.reason_card = find_card(new_card->pduel, pcard->previous.reason_card, playerid);
	} else{
		new_card->previous.reason_card = nullptr;
	}
	if(pcard->previous.reason_effect && pcard->previous.reason_effect->owner&& pcard->previous.reason_effect->owner->current.controler == 0 
		&& find_card(new_card->pduel, pcard->previous.reason_effect->owner, playerid) && effects_map.find(pcard->previous.reason_effect->clone_id) != effects_map.end()) {
		new_card->previous.reason_effect = effects_map[pcard->previous.reason_effect->clone_id];
	} else {
		new_card->previous.reason_effect = nullptr;
	}

	new_card->temp = pcard->temp;
	new_card->temp.controler = playerid;
	if(pcard->temp.reason_card && pcard->temp.reason_card->current.controler == target_playerid) {
		new_card->temp.reason_card = find_card(new_card->pduel, pcard->temp.reason_card, playerid);
	} else{
		new_card->temp.reason_card  = nullptr;
	}
	if(pcard->temp.reason_effect && pcard->temp.reason_effect->owner&& pcard->temp.reason_effect->owner->current.controler == 0 
		&& find_card(new_card->pduel, pcard->temp.reason_effect->owner, playerid) && effects_map.find(pcard->temp.reason_effect->clone_id) != effects_map.end()) {
		new_card->temp.reason_effect = effects_map[pcard->temp.reason_effect->clone_id];
	} else {
		new_card->temp.reason_effect = nullptr;
	}

	new_card->current = pcard->current;
	new_card->current.controler = playerid;
	if(pcard->current.reason_card && pcard->current.reason_card->current.controler == target_playerid) {
		new_card->current.reason_card = find_card(new_card->pduel, pcard->current.reason_card, playerid);
	} else{
		new_card->current.reason_card = nullptr;
	}
	if(pcard->current.reason_effect && pcard->current.reason_effect->owner&& pcard->current.reason_effect->owner->current.controler == 0 
		&& find_card(new_card->pduel, pcard->current.reason_effect->owner, playerid) && effects_map.find(pcard->current.reason_effect->clone_id) != effects_map.end()) {
		new_card->current.reason_effect = effects_map[pcard->current.reason_effect->clone_id];
	} else {
		new_card->current.reason_effect = nullptr;
	}
	
	new_card->spsummon = pcard->spsummon;
	new_card->spsummon.controler = playerid;
	if(pcard->spsummon.reason_card && pcard->spsummon.reason_card->spsummon.controler == target_playerid) {
		new_card->spsummon.reason_card = find_card(new_card->pduel, pcard->spsummon.reason_card, playerid);
	} else{
		new_card->spsummon.reason_card = nullptr;
	}
	if(pcard->spsummon.reason_effect && pcard->spsummon.reason_effect->owner&& pcard->spsummon.reason_effect->owner->current.controler == 0 
		&& find_card(new_card->pduel, pcard->spsummon.reason_effect->owner, playerid) && effects_map.find(pcard->spsummon.reason_effect->clone_id) != effects_map.end()) {
		new_card->spsummon.reason_effect = effects_map[pcard->spsummon.reason_effect->clone_id];
	} else {
		new_card->spsummon.reason_effect = nullptr;
	}
}
void effect_data_copy(effect* new_effect, effect* peffect,uint32_t playerid,uint32_t target_playerid){
	if(peffect->owner ){
		if(peffect->owner->current.location !=0){
			new_effect->owner = find_card(new_effect->pduel, peffect->owner, playerid);
		}else{
			new_effect->owner = new_effect->pduel->game_field->temp_card;
		}
	}
	if(peffect->handler){
		if(peffect->handler->current.location !=0){
			new_effect->handler = find_card(new_effect->pduel, peffect->handler, playerid);
		}else{
			new_effect->handler = new_effect->pduel->game_field->temp_card;
		}
	}
	new_effect->effect_owner = playerid;
	new_effect->description = peffect->description;
	new_effect->code = peffect->code;
	new_effect->clone_id = peffect->clone_id;
	// new_effect->id = peffect->id;
	new_effect->type = peffect->type;
	// new_effect->copy_id = peffect->copy_id;
	new_effect->range = peffect->range;
	new_effect->s_range = peffect->s_range;
	new_effect->o_range = peffect->o_range;
	new_effect->count_limit = peffect->count_limit;
	new_effect->count_limit_max = peffect->count_limit_max;
	new_effect->status = peffect->status;
	new_effect->reset_count = peffect->reset_count;
	new_effect->reset_flag = peffect->reset_flag;
	new_effect->count_code = peffect->count_code;
	new_effect->category = peffect->category;
	new_effect->flag[0] = peffect->flag[0];
	new_effect->flag[1] = peffect->flag[1];
	new_effect->hint_timing[0] = peffect->hint_timing[0];
	new_effect->hint_timing[1] = peffect->hint_timing[1];
	new_effect->card_type = peffect->card_type;
	new_effect->active_type = peffect->active_type;
	new_effect->active_location = peffect->active_location;
	new_effect->active_sequence = peffect->active_sequence;
	if(peffect->active_handler && peffect->active_handler->current.controler ==target_playerid ){
		new_effect->active_handler = find_card(new_effect->pduel, peffect->active_handler, playerid);
	}
	if(peffect->last_handler && peffect->last_handler->current.controler ==target_playerid ){
		new_effect->last_handler = find_card(new_effect->pduel, peffect->last_handler, playerid);
	}
	new_effect->label = peffect->label;
    new_effect->label_object = 0;

	if(peffect->condition)
		new_effect->condition = public_lua->clone_function_ref(peffect->condition);
	if(peffect->cost)
		new_effect->cost = public_lua->clone_function_ref(peffect->cost);
	if(peffect->target)
		new_effect->target = public_lua->clone_function_ref(peffect->target);
	if(peffect->operation)
		new_effect->operation = public_lua->clone_function_ref(peffect->operation);
	if(peffect->value){
		if(!peffect->is_flag(EFFECT_FLAG_FUNC_VALUE)) {
			new_effect->value = peffect->value;
		} else {
			new_effect->value = public_lua->clone_function_ref(peffect->value);
		}
	}

	// lua_State* srcL = nullptr;
    // lua_State* dstL = nullptr;
    // if(peffect->pduel && peffect->pduel->lua) srcL = peffect->pduel->lua->lua_state;
    // if(new_effect->pduel && new_effect->pduel->lua) dstL = new_effect->pduel->lua->lua_state;


    // if(!peffect->is_flag(EFFECT_FLAG_FUNC_VALUE)) {
    //     new_effect->value = peffect->value;
    // } else {
    //     if(srcL && dstL && srcL == dstL) {
    //         new_effect->value = peffect->value; 
    //     } else if(srcL && dstL) {
    //         int newref = rebind_lua_function_between_states(srcL, peffect->value, dstL);
    //         if(newref) new_effect->value = newref;
    //         else {
    //             new_effect->value = 0; 
    //         }
    //     } else {
    //         new_effect->value = 0;
    //     }
    // }

    // if(srcL && dstL && srcL == dstL) {
    //     new_effect->condition = peffect->condition;
    //     new_effect->cost = peffect->cost;
    //     new_effect->target = peffect->target;
    //     new_effect->operation = peffect->operation;
    // } else if(srcL && dstL) {
    //     new_effect->condition = rebind_lua_function_between_states(srcL, peffect->condition, dstL);
    //     new_effect->cost      = rebind_lua_function_between_states(srcL, peffect->cost, dstL);
    //     new_effect->target    = rebind_lua_function_between_states(srcL, peffect->target, dstL);
    //     new_effect->operation = rebind_lua_function_between_states(srcL, peffect->operation, dstL);
    // } else {
    //     new_effect->condition = 0;
    //     new_effect->cost = 0;
    //     new_effect->target = 0;
    //     new_effect->operation = 0;
    // }

    new_effect->cost_checked = peffect->cost_checked;
	// new_effect->required_handorset_effects = peffect->required_handorset_effects;
	new_effect->object_type = peffect->object_type;
}
card* find_card(duel* pduel, card* pcard, uint32_t playerid) {
    if(!pduel || !pcard) return nullptr;
    if(playerid > 1) return nullptr;
    size_t seq = pcard->current.sequence;
    auto &player = pduel->game_field->player[playerid];

    switch(pcard->current.location) {
    case LOCATION_MZONE:
        if(seq < player.list_mzone.size()) return player.list_mzone[seq];
        break;
    case LOCATION_SZONE:
        if(seq < player.list_szone.size()) return player.list_szone[seq];
        break;
    case LOCATION_HAND:
        if(seq < player.list_hand.size()) return player.list_hand[seq];
        break;
    case LOCATION_GRAVE:
        if(seq < player.list_grave.size()) return player.list_grave[seq];
        break;
    case LOCATION_REMOVED:
        if(seq < player.list_remove.size()) return player.list_remove[seq];
        break;
    case LOCATION_EXTRA:
        if(seq < player.list_extra.size()) return player.list_extra[seq];
        break;
    case LOCATION_DECK:
        if(seq < player.list_main.size()) return player.list_main[seq];
        break;
    case LOCATION_OVERLAY:
        if(pcard->overlay_target) {
            size_t ovseq = pcard->overlay_target->current.sequence;
            if(ovseq < player.list_mzone.size()) {
                card* host = player.list_mzone[ovseq];
                if(host && seq < host->xyz_materials.size())
                    return host->xyz_materials[seq];
            }
        }
        break;
    default:
        break;
    }
    return nullptr;
}
OCGCORE_API void start_duel(intptr_t pduel, uint32_t options) {
	duel* pd = (duel*)pduel;
	options |= DUEL_ATTACK_FIRST_TURN;
	uint16_t duel_rule = options >> 16;
	uint16_t duel_options = options & 0xffff;
	pd->game_field->core.duel_options |= duel_options;
	if (duel_rule >= 1 && duel_rule <= CURRENT_RULE)
		pd->game_field->core.duel_rule = duel_rule;
	else if(options & DUEL_OBSOLETE_RULING)		//provide backward compatibility with replay
		pd->game_field->core.duel_rule = 1;
	if (pd->game_field->core.duel_rule < 1 || pd->game_field->core.duel_rule > CURRENT_RULE)
		pd->game_field->core.duel_rule = CURRENT_RULE;
	if (pd->game_field->core.duel_rule == MASTER_RULE3) {
		pd->game_field->player[0].szone_size = 8;
		pd->game_field->player[1].szone_size = 8;
	}
	pd->game_field->core.shuffle_hand_check[0] = FALSE;
	pd->game_field->core.shuffle_hand_check[1] = FALSE;
	pd->game_field->core.shuffle_deck_check[0] = FALSE;
	pd->game_field->core.shuffle_deck_check[1] = FALSE;
	if(pd->game_field->player[0].start_count > 0)
		pd->game_field->draw(0, REASON_RULE, PLAYER_NONE, 0, pd->game_field->player[0].start_count);
	if(pd->game_field->player[1].start_count > 0)
		pd->game_field->draw(0, REASON_RULE, PLAYER_NONE, 1, pd->game_field->player[1].start_count);
	if(options & DUEL_TAG_MODE) {
		for(int i = 0; i < pd->game_field->player[0].start_count && pd->game_field->player[0].tag_list_main.size(); ++i) {
			card* pcard = pd->game_field->player[0].tag_list_main.back();
			pd->game_field->player[0].tag_list_main.pop_back();
			pd->game_field->player[0].tag_list_hand.push_back(pcard);
			pcard->current.controler = 0;
			pcard->current.location = LOCATION_HAND;
			pcard->current.sequence = (uint8_t)pd->game_field->player[0].tag_list_hand.size() - 1;
			pcard->current.position = POS_FACEDOWN;
		}
		for(int i = 0; i < pd->game_field->player[1].start_count && pd->game_field->player[1].tag_list_main.size(); ++i) {
			card* pcard = pd->game_field->player[1].tag_list_main.back();
			pd->game_field->player[1].tag_list_main.pop_back();
			pd->game_field->player[1].tag_list_hand.push_back(pcard);
			pcard->current.controler = 1;
			pcard->current.location = LOCATION_HAND;
			pcard->current.sequence = (uint8_t)pd->game_field->player[1].tag_list_hand.size() - 1;
			pcard->current.position = POS_FACEDOWN;
		}
	}
	pd->game_field->add_process(PROCESSOR_TURN, 0, 0, 0, 0, 0);
}
OCGCORE_API void end_duel(intptr_t pduel) {
	duel* pd = (duel*)pduel;
	if(duel_set.count(pd)) {
		duel_set.erase(pd);
		delete pd;
	}
}
OCGCORE_API void set_player_lp(intptr_t pduel, intptr_t player, int32_t playerid){
	duel* pd = (duel*)pduel;
	duel* pd2 = (duel*)player;
	pd2->game_field->player[0].lp = pd->game_field->player[playerid].lp;
	pd2->game_field->player[1].lp = pd->game_field->player[1-playerid].lp;
}
OCGCORE_API void set_player_info(intptr_t pduel, int32_t playerid, int32_t lp, int32_t startcount, int32_t drawcount) {
	if (!check_playerid(playerid))
		return;
	duel* pd = (duel*)pduel;
	if(lp > 0)
		pd->game_field->player[playerid].lp = lp;
	if(startcount >= 0)
		pd->game_field->player[playerid].start_count = startcount;
	if(drawcount >= 0)
		pd->game_field->player[playerid].draw_count = drawcount;
}
OCGCORE_API void get_log_message(intptr_t pduel, char* buf) {
	duel* pd = (duel*)pduel;
	std::strncpy(buf, pd->strbuffer, sizeof pd->strbuffer - 1);
	buf[sizeof pd->strbuffer - 1] = 0;
}
OCGCORE_API int32_t get_message(intptr_t pduel, byte* buf) {
	int32_t len = ((duel*)pduel)->read_buffer(buf);
	((duel*)pduel)->clear_buffer();
	return len;
}
OCGCORE_API uint32_t process(intptr_t pduel) {
	duel* pd = (duel*)pduel;
	uint32_t result = 0; 
	do {
		result = pd->game_field->process();
	} while ((result & PROCESSOR_BUFFER_LEN) == 0 && (result & PROCESSOR_FLAG) == 0);
	return result;
}
OCGCORE_API void new_card(intptr_t pduel, uint32_t code, uint8_t owner, uint8_t playerid, uint8_t location, uint8_t sequence, uint8_t position) {
	if (!check_playerid(owner) || !check_playerid(playerid))
		return;
	duel* ptduel = (duel*)pduel;
	if(ptduel->game_field->is_location_useable(playerid, location, sequence)) {
		card* pcard = ptduel->new_card(code);
		pcard->owner = owner;
		ptduel->game_field->add_card(playerid, pcard, location, sequence);
		pcard->current.position = position;
		if(!(location & LOCATION_ONFIELD) || (position & POS_FACEUP)) {
			pcard->enable_field_effect(true);
			ptduel->game_field->adjust_instant();
		} if(location & LOCATION_ONFIELD) {
			if(location == LOCATION_MZONE)
				pcard->set_status(STATUS_PROC_COMPLETE, TRUE);
		}
	}
}
OCGCORE_API void new_tag_card(intptr_t pduel, uint32_t code, uint8_t owner, uint8_t location) {
	duel* ptduel = (duel*)pduel;
	if(owner > 1 || !(location & (LOCATION_DECK | LOCATION_EXTRA)))
		return;
	card* pcard = ptduel->new_card(code);
	switch(location) {
	case LOCATION_DECK:
		ptduel->game_field->player[owner].tag_list_main.push_back(pcard);
		pcard->owner = owner;
		pcard->current.controler = owner;
		pcard->current.location = LOCATION_DECK;
		pcard->current.sequence = (uint8_t)ptduel->game_field->player[owner].tag_list_main.size() - 1;
		pcard->current.position = POS_FACEDOWN_DEFENSE;
		break;
	case LOCATION_EXTRA:
		ptduel->game_field->player[owner].tag_list_extra.push_back(pcard);
		pcard->owner = owner;
		pcard->current.controler = owner;
		pcard->current.location = LOCATION_EXTRA;
		pcard->current.sequence = (uint8_t)ptduel->game_field->player[owner].tag_list_extra.size() - 1;
		pcard->current.position = POS_FACEDOWN_DEFENSE;
		break;
	}
}
/**
* @brief Get card information.
* @param buf int32_t array
* @return buffer length in bytes
*/
OCGCORE_API int32_t query_card(intptr_t pduel, uint8_t playerid, uint8_t location, uint8_t sequence, int32_t query_flag, byte* buf, int32_t use_cache) {
	if (!check_playerid(playerid))
		return LEN_FAIL;
	duel* ptduel = (duel*)pduel;
	card* pcard = nullptr;
	location &= 0x7f;
	if (location == LOCATION_MZONE || location == LOCATION_SZONE)
		pcard = ptduel->game_field->get_field_card(playerid, location, sequence);
	else {
		card_vector* lst = nullptr;
		if (location == LOCATION_HAND)
			lst = &ptduel->game_field->player[playerid].list_hand;
		else if (location == LOCATION_GRAVE)
			lst = &ptduel->game_field->player[playerid].list_grave;
		else if (location == LOCATION_REMOVED)
			lst = &ptduel->game_field->player[playerid].list_remove;
		else if (location == LOCATION_EXTRA)
			lst = &ptduel->game_field->player[playerid].list_extra;
		else if (location == LOCATION_DECK)
			lst = &ptduel->game_field->player[playerid].list_main;
		else
			return LEN_FAIL;
		if (sequence >= (int32_t)lst->size())
			return LEN_FAIL;
		pcard = (*lst)[sequence];
	}
	if (pcard) {
		return pcard->get_infos(buf, query_flag, use_cache);
	}
	else {
		buffer_write<int32_t>(buf, LEN_EMPTY);
		return LEN_EMPTY;
	}
}
OCGCORE_API int32_t query_field_count(intptr_t pduel, uint8_t playerid, uint8_t location) {
	duel* ptduel = (duel*)pduel;
	if (!check_playerid(playerid))
		return 0;
	auto& player = ptduel->game_field->player[playerid];
	if(location == LOCATION_HAND)
		return (int32_t)player.list_hand.size();
	if(location == LOCATION_GRAVE)
		return (int32_t)player.list_grave.size();
	if(location == LOCATION_REMOVED)
		return (int32_t)player.list_remove.size();
	if(location == LOCATION_EXTRA)
		return (int32_t)player.list_extra.size();
	if(location == LOCATION_DECK)
		return (int32_t)player.list_main.size();
	if(location == LOCATION_MZONE) {
		int32_t count = 0;
		for(auto& pcard : player.list_mzone)
			if(pcard)
				++count;
		return count;
	}
	if(location == LOCATION_SZONE) {
		int32_t count = 0;
		for(auto& pcard : player.list_szone)
			if(pcard)
				++count;
		return count;
	}
	return 0;
}
OCGCORE_API int32_t query_field_card(intptr_t pduel, uint8_t playerid, uint8_t location, uint32_t query_flag, byte* buf, int32_t use_cache) {
	if (!check_playerid(playerid))
		return LEN_FAIL;
	duel* ptduel = (duel*)pduel;
	auto& player = ptduel->game_field->player[playerid];
	byte* p = buf;
	if(location == LOCATION_MZONE) {
		for(auto& pcard : player.list_mzone) {
			if(pcard) {
				int32_t clen = pcard->get_infos(p, query_flag, use_cache);
				p += clen;
			} else {
				buffer_write<int32_t>(p, LEN_EMPTY);
			}
		}
	}
	else if(location == LOCATION_SZONE) {
		for(auto& pcard : player.list_szone) {
			if(pcard) {
				int32_t clen = pcard->get_infos(p, query_flag, use_cache);
				p += clen;
			} else {
				buffer_write<int32_t>(p, LEN_EMPTY);
			}
		}
	}
	else {
		card_vector* lst = nullptr;
		if(location == LOCATION_HAND)
			lst = &player.list_hand;
		else if(location == LOCATION_GRAVE)
			lst = &player.list_grave;
		else if(location == LOCATION_REMOVED)
			lst = &player.list_remove;
		else if(location == LOCATION_EXTRA)
			lst = &player.list_extra;
		else if(location == LOCATION_DECK)
			lst = &player.list_main;
		else
			return LEN_FAIL;
		for(auto& pcard : *lst) {
			int32_t clen = pcard->get_infos(p, query_flag, use_cache);
			p += clen;
		}
	}
	return (int32_t)(p - buf);
}
OCGCORE_API int32_t query_field_info(intptr_t pduel, byte* buf) {
	duel* ptduel = (duel*)pduel;
	byte* p = buf;
	*p++ = MSG_RELOAD_FIELD;
	*p++ = (uint8_t)ptduel->game_field->core.duel_rule;
	for(int playerid = 0; playerid < 2; ++playerid) {
		auto& player = ptduel->game_field->player[playerid];
		buffer_write<int32_t>(p, player.lp);
		for(auto& pcard : player.list_mzone) {
			if(pcard) {
				*p++ = 1;
				*p++ = pcard->current.position;
				*p++ = (uint8_t)pcard->xyz_materials.size();
			} else {
				*p++ = 0;
			}
		}
		for(auto& pcard : player.list_szone) {
			if(pcard) {
				*p++ = 1;
				*p++ = pcard->current.position;
			} else {
				*p++ = 0;
			}
		}
		*p++ = (uint8_t)player.list_main.size();
		*p++ = (uint8_t)player.list_hand.size();
		*p++ = (uint8_t)player.list_grave.size();
		*p++ = (uint8_t)player.list_remove.size();
		*p++ = (uint8_t)player.list_extra.size();
		*p++ = (uint8_t)player.extra_p_count;
	}
	*p++ = (uint8_t)ptduel->game_field->core.current_chain.size();
	for(const auto& ch : ptduel->game_field->core.current_chain) {
		effect* peffect = ch.triggering_effect;
		buffer_write<uint32_t>(p, peffect->get_handler()->data.code);
		buffer_write<uint32_t>(p, peffect->get_handler()->get_info_location());
		*p++ = ch.triggering_controler;
		*p++ = (uint8_t)ch.triggering_location;
		*p++ = ch.triggering_sequence;
		buffer_write<uint32_t>(p, peffect->description);
	}
	return (int32_t)(p - buf);
}
OCGCORE_API void set_responsei(intptr_t pduel, int32_t value) {
	((duel*)pduel)->set_responsei(value);
}
OCGCORE_API void set_responseb(intptr_t pduel, byte* buf) {
	((duel*)pduel)->set_responseb(buf);
}
OCGCORE_API int32_t preload_script(intptr_t pduel, const char* script_name) {
	return ((duel*)pduel)->lua->load_script(script_name);
}
