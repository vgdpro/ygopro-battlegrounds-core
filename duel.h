/*
 * duel.h
 *
 *  Created on: 2010-4-8
 *      Author: Argon
 */

#ifndef DUEL_H_
#define DUEL_H_

#include "common.h"
#include "sort.h"
#include "mtrandom.h"
#include <set>
#include <unordered_set>
#include <vector>
#include <map>
#include <unordered_map>

class card;
class group;
class effect;
class field;
class interpreter;

using card_set = std::set<card*, card_sort>;

class duel {
public:
	char strbuffer[256]{};
	int32_t rng_version{ 2 };
	uint32_t next_effect_id{1};
	interpreter* lua;
	field* game_field;
	mtrandom random;

	std::vector<byte> message_buffer;
	std::unordered_set<card*> cards;
	std::unordered_set<card*> assumes;
	std::unordered_set<group*> groups;
	std::unordered_set<group*> sgroups;
	std::unordered_set<effect*> effects;
	std::map<int, effect*> effects_map;
	std::unordered_set<effect*> uncopy;

	std::unordered_map<uint32_t, int> card_table_refs;
	
	duel();
	~duel();
	void clear();
	
	uint32_t buffer_size() const {
		return (uint32_t)message_buffer.size() & PROCESSOR_BUFFER_LEN;
	}
	card* new_card(uint32_t code);
	std::vector<card*> new_card_random( uint32_t type,uint32_t count ,bool is_include);
	group* new_group();
	group* new_group(card* pcard);
	group* new_group(const card_set& cset);
	effect* new_effect();
	void delete_card(card* pcard);
	void delete_group(group* pgroup);
	void delete_effect(effect* peffect);
	void release_script_group();
	void restore_assumes();
	int32_t read_buffer(byte* buf);
	void write_buffer(const void* data, int size);
	void write_buffer32(uint32_t value);
	void write_buffer16(uint16_t value);
	void write_buffer8(uint8_t value);
	void clear_buffer();
	void set_responsei(uint32_t resp);
	void set_responseb(byte* resp);
	int32_t get_next_integer(int32_t l, int32_t h);
private:
	group* register_group(group* pgroup);
};

#endif /* DUEL_H_ */
