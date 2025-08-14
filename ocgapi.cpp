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
static uint32_t default_card_reader_random(card_data* data, uint32_t type, bool is_include) {
	return 0;
}
static uint32_t default_message_handler(intptr_t pduel, uint32_t message_type) {
	return 0;
}
static script_reader sreader = default_script_reader;
static card_reader creader = default_card_reader;
static card_reader_random rcreader = default_card_reader_random;
static message_handler mhandler = default_message_handler;
static byte buffer[0x100000];
static std::set<duel*> duel_set;

OCGCORE_API void set_script_reader(script_reader f) {
	sreader = f;
}
OCGCORE_API void set_card_reader(card_reader f) {
	creader = f;
}
OCGCORE_API void set_card_reader_random(card_reader_random f) {
	rcreader = f;
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
uint32_t read_card_random(card_data* data ,uint32_t type,bool is_include) {
	return rcreader(data ,type, is_include);
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
	pduel->random.seed(seed_sequence, SEED_COUNT);
	pduel->rng_version = 2;
	return (intptr_t)pduel;
}
OCGCORE_API void copy_duel_data(intptr_t source_pduel, intptr_t spduel,uint32_t playerid,uint32_t location){
	// duel* source = (duel*)source_pduel;
	// duel* target = (duel*)spduel;
	// if(location & LOCATION_SZONE){
	// 	for(auto& pcard : source->game_field->player[playerid].list_szone){
	// 		if(pcard){
	// 			source->game_field->remove_card(pcard);
	// 		}
	// 	}
	// 	for(int i=0; i < target->game_field->player[0].list_szone.size(); ++i) {
	// 		if(target->game_field->player[0].list_szone[i]) {
	// 			card* pcard = target->game_field->player[0].list_szone[i];
	// 			new_card(source_pduel, pcard->get_code(),pcard->owner==0?playerid:(1-playerid),playerid,LOCATION_SZONE,pcard->current.sequence,pcard->current.position);
	// 			card* new_card = source->game_field->player[playerid].list_szone[i];
	// 		}
	// 	}
	// }
	// if(location & LOCATION_MZONE){
	// 	for(auto& pcard : source->game_field->player[playerid].list_mzone){
	// 		if(pcard){
	// 			source->game_field->remove_card(pcard);
	// 		}
	// 	}
	// 	for(int i=0; i < target->game_field->player[0].list_mzone.size(); ++i) {
	// 		if(target->game_field->player[0].list_mzone[i]) {
	// 			card* pcard = target->game_field->player[0].list_mzone[i];
	// 			new_card(source_pduel, pcard->get_code(),pcard->owner==0?playerid:(1-playerid),playerid,LOCATION_MZONE,pcard->current.sequence,pcard->current.position);
	// 			card* new_card = source->game_field->player[playerid].list_mzone[i];
	// 			for(auto& pc :pcard->xyz_materials){
	// 				if(pc){
	// 					card* mat = source->new_card(pc->get_code());
	// 					new_card->xyz_add(mat);
	// 				}
	// 			}
	// 		}
	// 	}
	// }
	// if(location & LOCATION_HAND){
	// 	for(auto& pcard : source->game_field->player[playerid].list_hand){
	// 		if(pcard){
	// 			source->game_field->remove_card(pcard);
	// 		}
	// 	}
	// 	for(int i=0; i < target->game_field->player[0].list_hand.size(); ++i) {
	// 		if(target->game_field->player[0].list_hand[i]) {
	// 			card* pcard = target->game_field->player[0].list_hand[i];
	// 			new_card(source_pduel, pcard->get_code(),pcard->owner==0?playerid:(1-playerid),playerid,LOCATION_HAND,pcard->current.sequence,pcard->current.position);
	// 			card* new_card = source->game_field->player[playerid].list_hand[i];
	// 		}
	// 	}
	// }
	// if(location & LOCATION_GRAVE){
	// 	for(auto& pcard : source->game_field->player[playerid].list_grave){
	// 		if(pcard){
	// 			source->game_field->remove_card(pcard);
	// 		}
	// 	}
	// 	for(int i=0; i < target->game_field->player[0].list_grave.size(); ++i) {
	// 		if(target->game_field->player[0].list_grave[i]) {
	// 			card* pcard = target->game_field->player[0].list_grave[i];
	// 			new_card(source_pduel, pcard->get_code(),pcard->owner==0?playerid:(1-playerid),playerid,LOCATION_GRAVE,pcard->current.sequence,pcard->current.position);
	// 			card* new_card = source->game_field->player[playerid].list_grave[i];
	// 		}
	// 	}
	// }
	// if(location & LOCATION_REMOVED){
	// 	for(auto& pcard : source->game_field->player[playerid].list_remove){
	// 		if(pcard){
	// 			source->game_field->remove_card(pcard);
	// 		}
	// 	}
	// 	for(int i=0; i < target->game_field->player[0].list_remove.size(); ++i) {
	// 		if(target->game_field->player[0].list_remove[i]) {
	// 			card* pcard = target->game_field->player[0].list_remove[i];
	// 			new_card(source_pduel, pcard->get_code(),pcard->owner==0?playerid:(1-playerid),playerid,LOCATION_REMOVED,pcard->current.sequence,pcard->current.position);
	// 			card* new_card = source->game_field->player[playerid].list_remove[i];
	// 		}
	// 	}
	// }
	// if(location & LOCATION_EXTRA){
	// 	for(auto& pcard : source->game_field->player[playerid].list_extra){
	// 		if(pcard){
	// 			source->game_field->remove_card(pcard);
	// 		}
	// 	}
	// 	for(int i=0; i < target->game_field->player[0].list_extra.size(); ++i) {
	// 		if(target->game_field->player[0].list_extra[i]) {
	// 			card* pcard = target->game_field->player[0].list_extra[i];
	// 			new_card(source_pduel, pcard->get_code(),pcard->owner==0?playerid:(1-playerid),playerid,LOCATION_EXTRA,pcard->current.sequence,pcard->current.position);
	// 			card* new_card = source->game_field->player[playerid].list_extra[i];
	// 		}
	// 	}
	// }
	// if(location & LOCATION_DECK){
	// 	for(auto& pcard : source->game_field->player[playerid].list_main){
	// 		if(pcard){
	// 			source->game_field->remove_card(pcard);
	// 		}
	// 	}
	// 	for(int i=0; i < target->game_field->player[0].list_main.size(); ++i) {
	// 		if(target->game_field->player[0].list_main[i]) {
	// 			card* pcard = target->game_field->player[0].list_main[i];
	// 			new_card(source_pduel, pcard->get_code(),pcard->owner==0?playerid:(1-playerid),playerid,LOCATION_DECK,pcard->current.sequence,pcard->current.position);
	// 			card* new_card = source->game_field->player[playerid].list_main[i];
	// 		}
	// 	}
	// }
	

	// for(int i=0; i < target->game_field->player[0].list_szone.size(); ++i) {
	// 	if(target->game_field->player[0].list_szone[i]) {
	// 		card* pcard = target->game_field->player[0].list_szone[i];
	// 		card* new_card = source->game_field->player[playerid].list_szone[i];
	// 		card_data_copy(new_card, pcard, playerid);
	// 	}
	// }

	// for(int i=0; i < target->game_field->player[0].list_mzone.size(); ++i) {
	// 	if(target->game_field->player[0].list_mzone[i]) {
	// 		card* pcard = target->game_field->player[0].list_mzone[i];
	// 		card* new_card = source->game_field->player[playerid].list_mzone[i];
	// 		card_data_copy(new_card, pcard, playerid);
	// 		for(int i=0; i < new_card->xyz_materials.size(); ++i) {
	// 			card_data_copy(new_card->xyz_materials[i], pcard->xyz_materials[i], playerid);
	// 		}
	// 	}
	// }

	// for(int i=0; i < target->game_field->player[0].list_hand.size(); ++i) {
	// 	if(target->game_field->player[0].list_hand[i]) {
	// 		card* pcard = target->game_field->player[0].list_hand[i];
	// 		card* new_card = source->game_field->player[playerid].list_hand[i];
	// 		card_data_copy(new_card, pcard, playerid);
	// 	}
	// }

	// for(int i=0; i < target->game_field->player[0].list_grave.size(); ++i) {
	// 	if(target->game_field->player[0].list_grave[i]) {
	// 		card* pcard = target->game_field->player[0].list_grave[i];
	// 		card* new_card = source->game_field->player[playerid].list_grave[i];
	// 		card_data_copy(new_card, pcard, playerid);
	// 	}
	// }

	// for(int i=0; i < target->game_field->player[0].list_remove.size(); ++i) {
	// 	if(target->game_field->player[0].list_remove[i]) {
	// 		card* pcard = target->game_field->player[0].list_remove[i];
	// 		card* new_card = source->game_field->player[playerid].list_remove[i];
	// 		card_data_copy(new_card, pcard, playerid);
	// 	}
	// }

	// for(int i=0; i < target->game_field->player[0].list_extra.size(); ++i) {
	// 	if(target->game_field->player[0].list_extra[i]) {
	// 		card* pcard = target->game_field->player[0].list_extra[i];
	// 		card* new_card = source->game_field->player[playerid].list_extra[i];
	// 		card_data_copy(new_card, pcard, playerid);
	// 	}
	// }

	// for(int i=0; i < target->game_field->player[0].list_main.size(); ++i) {
	// 	if(target->game_field->player[0].list_main[i]) {
	// 		card* pcard = target->game_field->player[0].list_main[i];
	// 		card* new_card = source->game_field->player[playerid].list_main[i];
	// 		card_data_copy(new_card, pcard, playerid);
	// 	}
	// }
	
}
void card_data_copy(card* new_card, card* pcard, uint32_t playerid){
	// new_card->ref_handle = pcard->ref_handle;
	// // new_card->previous = pcard->previous;
	// // new_card->previous.code = pcard->previous.code;
	// // new_card->previous.code2 = pcard->previous.code2;
	// // new_card->previous.setcode = pcard->previous.setcode;
	// // new_card->previous.type = pcard->previous.type;
	// // new_card->previous.level = pcard->previous.level;
	// // new_card->previous.rank = pcard->previous.rank;
	// // new_card->previous.link = pcard->previous.link;
	// // new_card->previous.lscale = pcard->previous.lscale;
	// // new_card->previous.rscale = pcard->previous.rscale;
	// // new_card->previous.attribute = pcard->previous.attribute;
	// // new_card->previous.race = pcard->previous.race;
	// // new_card->previous.attack = pcard->previous.attack;
	// // new_card->previous.defense = pcard->previous.defense;
	// // new_card->previous.base_attack = pcard->previous.base_attack;
	// // new_card->previous.base_defense = pcard->previous.base_defense;
	// // new_card->previous.controler = pcard->previous.controler;
	// // new_card->previous.location = pcard->previous.location;
	// // new_card->previous.sequence = pcard->previous.sequence;
	// // new_card->previous.position = pcard->previous.position;
	// // new_card->previous.reason = pcard->previous.reason;
	// // new_card->previous.reason_player = pcard->previous.reason_player;
	// // if(pcard->previous.reason_card && pcard->previous.reason_card->current.controler == 0) {
	// // 	new_card->previous.reason_card = find_card(new_card->pduel, pcard->previous.reason_card, playerid);
	// // }
	// // else{
	// // 	new_card->previous.reason_card = nullptr;
	// // }
	// // if(pcard->previous.reason_effect && pcard->previous.reason_effect->owner->current.controler == 0) {
	// // 	effect* new_effect=new_card->pduel->new_effect();
	// // 	effect_data_copy(new_effect, pcard->previous.reason_effect, playerid);
	// // }
	// // else{
	// // 	new_card->previous.reason_effect = nullptr;
	// // }
	// // new_card->temp = pcard->temp;
	// // if(pcard->temp.reason_card) {
	// // 	if(pcard->temp.reason_card->current.controler == 0){
	// // 		new_card->temp.reason_card = find_card(new_card->pduel, pcard->temp.reason_card, playerid);
	// // 	}
	// // }
	// // if(pcard->temp.reason_effect) {
	// // 	if(pcard->temp.reason_effect->owner->current.controler == 0){
	// // 		effect* new_effect=new_card->pduel->new_effect();
	// // 		effect_data_copy(new_effect, pcard->temp.reason_effect, playerid);
	// // 	}
	// // 	else{
	// // 		new_card->temp.reason_effect = nullptr;
	// // 	}
	// // }
	// // new_card->current = pcard->current;
	// // if(pcard->current.reason_card) {
	// // 	if(pcard->current.reason_card->current.controler == 0){
	// // 		new_card->current.reason_card = find_card(new_card->pduel, pcard->current.reason_card, playerid);
	// // 	}
	// // }
	// // if(pcard->current.reason_effect) {
	// // 	if(pcard->current.reason_effect->owner->current.controler == 0){
	// // 		effect* new_effect=new_card->pduel->new_effect();
	// // 		effect_data_copy(new_effect, pcard->current.reason_effect, playerid);
	// // 	}
	// // 	else{
	// // 		new_card->current.reason_effect = nullptr;
	// // 	}
	// // }
	// // new_card->spsummon = pcard->spsummon;
	// // if(pcard->spsummon.reason_card) {
	// // 	if(pcard->spsummon.reason_card->spsummon.controler == 0){
	// // 		new_card->spsummon.reason_card = find_card(new_card->pduel, pcard->spsummon.reason_card, playerid);
	// // 	}
	// // }
	// // if(pcard->spsummon.reason_effect) {
	// // 	if(pcard->spsummon.reason_effect->owner->spsummon.controler == 0){
	// // 		effect* new_effect=new_card->pduel->new_effect();
	// // 		effect_data_copy(new_effect, pcard->spsummon.reason_effect, playerid);
	// // 	}
	// // 	else{
	// // 		new_card->spsummon.reason_effect = nullptr;
	// // 	}
	// // }
	// // // card_state previous;
	// // card_state temp;
	// // card_state current;
	// // card_state spsummon;
	// new_card->q_cache = pcard->q_cache;
	// new_card->summon_player = pcard->summon_player;
	// new_card->summon_info = pcard->summon_info;
	// new_card->status = pcard->status;
	// new_card->sendto_param = pcard->sendto_param;
	// new_card->release_param = pcard->release_param;
	// new_card->sum_param = pcard->sum_param;
	// new_card->position_param = pcard->position_param;
	// new_card->spsummon_param= pcard->spsummon_param;
	// new_card->to_field_param = pcard->to_field_param;
	// new_card->attack_announce_count = pcard->attack_announce_count;
	// new_card->direct_attackable = pcard->direct_attackable;
	// new_card->announce_count = pcard->announce_count;
	// new_card->attacked_count = pcard->attacked_count;
	// new_card->attack_all_target = pcard->attack_all_target;
	// new_card->attack_controler = pcard->attack_controler;
	// new_card->cardid= pcard->cardid;
	// new_card->fieldid = pcard->fieldid;
	// new_card->fieldid_r = pcard->fieldid_r;
	// new_card->activate_count_id = pcard->activate_count_id;
	// new_card->turnid = pcard->turnid;
	// new_card->turn_counter = pcard->turn_counter;
	// new_card->unique_pos[0] = pcard->unique_pos[0];
	// new_card->unique_pos[1] = pcard->unique_pos[1];
	// new_card->unique_fieldid = pcard->unique_fieldid;
	// new_card->unique_code = pcard->unique_code;
	// new_card->unique_location = pcard->unique_location;
	// new_card->unique_function = pcard->unique_function;
	// // unique_effect is not copied, it should be set by the effect
	// // if(pcard->unique_effect) {
	// // 	if(pcard->unique_effect->owner->current.controler == 0){
	// // 		effect* new_effect=new_card->pduel->new_effect();
	// // 		effect_data_copy(new_effect, pcard->unique_effect, playerid);
	// // 	}
	// // 	else{
	// // 		new_card->unique_effect = nullptr;
	// // 	}
	// // }
	// new_card->spsummon_code = pcard->spsummon_code;
	// new_card->spsummon_counter[0] = pcard->spsummon_counter[0];
	// new_card->spsummon_counter[1] = pcard->spsummon_counter[1];
	// new_card->assume_type = pcard->assume_type;
	// new_card->assume_value = pcard->assume_value;
	// // new_card->equiping_target = find_card(new_card->pduel, pcard->equiping_target, playerid);
	// // new_card->pre_equip_target = find_card(new_card->pduel, pcard->pre_equip_target, playerid);
	// // new_card->overlay_target = find_card(new_card->pduel, pcard->overlay_target, playerid);
	// // new_card->relations.clear();
	// // for(const auto& rel : pcard->relations) {
	// // 	card* old_related = rel.first;
	// // 	uint32_t rel_val = rel.second;
	// // 	card* new_related = find_card(new_card->pduel, old_related, playerid);
	// // 	if(new_related)
	// // 		new_card->relations[new_related] = rel_val;
	// // }
	// // card* equiping_target{};
	// // card* pre_equip_target{};
	// // card* overlay_target{};
	// // relation_map relations;
	// new_card->counters = pcard->counters;
	// new_card->indestructable_effects = pcard->indestructable_effects;
	// // attacker_map announced_cards;
	// // attacker_map attacked_cards;
	// // attacker_map battled_cards;
	// // card_set equiping_cards;
	// // card_set material_cards;
	// // card_set effect_target_owner;
	// // card_set effect_target_cards;
	// // card_vector xyz_materials;
	// new_card->xyz_materials_previous_count_onfield = pcard->xyz_materials_previous_count_onfield;
	// // effect_container single_effect;
	// // effect_container field_effect;
	// // effect_container equip_effect;
	// // effect_container target_effect;
	// // effect_container xmaterial_effect;
	// // effect_indexer indexer;
	// // effect_relation relate_effect;
	// // effect_set_v immune_effect;
	// // effect_collection initial_effect;
	// // effect_collection owning_effect;
}
void effect_data_copy(effect* new_effect, effect* peffect,uint32_t playerid){
	new_effect->ref_handle = peffect->ref_handle;
	new_effect->owner = find_card(new_effect->pduel, peffect->owner, playerid);
	new_effect->handler = find_card(new_effect->pduel, peffect->handler, playerid);
	new_effect->effect_owner = peffect->effect_owner;
	new_effect->description = peffect->description;
	new_effect->code = peffect->code;
	new_effect->id = peffect->id;
	new_effect->type = peffect->type;
	new_effect->copy_id = peffect->copy_id;
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
	new_effect->active_handler = find_card(new_effect->pduel, peffect->active_handler, playerid);
	new_effect->last_handler = find_card(new_effect->pduel, peffect->last_handler, playerid);
	new_effect->label = peffect->label;
	new_effect->label_object = peffect->label_object;
	new_effect->condition = peffect->condition;
	new_effect->cost = peffect->cost;
	new_effect->target = peffect->target;
	new_effect->value = peffect->value;
	new_effect->operation = peffect->operation;
	new_effect->cost_checked = peffect->cost_checked;
	new_effect->required_handorset_effects = peffect->required_handorset_effects;
	new_effect->object_type = peffect->object_type;
	find_card(new_effect->pduel, peffect->owner, playerid)->add_effect(new_effect);
}
card* find_card(duel*pduel, card* pcard, uint32_t playerid){
	if(pcard->current.location == LOCATION_MZONE){
		return pduel->game_field->player[playerid].list_mzone[pcard->current.sequence];
	}
	if(pcard->current.location == LOCATION_SZONE){
		return pduel->game_field->player[playerid].list_szone[pcard->current.sequence];
	}
	if(pcard->current.location == LOCATION_HAND){
		return pduel->game_field->player[playerid].list_hand[pcard->current.sequence];
	}
	if(pcard->current.location == LOCATION_GRAVE){
		return pduel->game_field->player[playerid].list_grave[pcard->current.sequence];
	}
	if(pcard->current.location == LOCATION_REMOVED){
		return pduel->game_field->player[playerid].list_remove[pcard->current.sequence];
	}
	if(pcard->current.location == LOCATION_EXTRA){
		return pduel->game_field->player[playerid].list_extra[pcard->current.sequence];
	}
	if(pcard->current.location == LOCATION_DECK){
		return pduel->game_field->player[playerid].list_main[pcard->current.sequence];
	}
	if(pcard->current.location == LOCATION_OVERLAY){
		if(pcard->overlay_target){
			return pduel->game_field->player[playerid].list_mzone[pcard->overlay_target->current.sequence]->xyz_materials[pcard->current.sequence];
		}
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
