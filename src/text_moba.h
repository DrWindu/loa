/*
 *  Copyright (C) 2018 the authors (see AUTHORS)
 *
 *  This file is part of Draklia's ld41.
 *
 *  lair is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  lair is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with lair.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


#ifndef LD41_TEXT_MOBA_H_
#define LD41_TEXT_MOBA_H_


#include <utility>
#include <unordered_map>
#include <set>

#include <lair/core/lair.h>
#include <lair/core/path.h>
#include <lair/core/parse.h>

#include "console.h"


class MainState;
class Console;


enum Team {
	BLUE,
	RED,
	NEUTRAL,
};

enum Place {
	BACK,
	FRONT,
};


class MapNode;
class CharacterClass;
class Character;
class TextMoba;
class TMCommand;

typedef std::shared_ptr<MapNode>        MapNodeSP;
typedef std::weak_ptr<MapNode>          MapNodeWP;
typedef std::shared_ptr<CharacterClass> CharacterClassSP;
typedef std::shared_ptr<Character>      CharacterSP;
typedef std::shared_ptr<TMCommand>      TMCommandSP;


typedef std::vector<int>          IntVector;
typedef std::vector<lair::String> StringVector;

typedef std::set<CharacterSP>     CharacterSet;



class TextMoba {
public:
	typedef std::vector<TMCommandSP> TMCommandList;

public:
	TextMoba(MainState* mainState, Console* console);

	void initialize(const lair::Path& logicPath);

	MainState* mainState();
	Console* console();

	MapNodeSP mapNode(const lair::String& id);
	CharacterClassSP characterClass(const lair::String& id);
	CharacterSP player();

	void moveCharacter(CharacterSP character, MapNodeSP dest);
	void nextTurn();

	const TMCommandList& commands() const;
	TMCommand* command(const lair::String& name) const;

	void _addCommand(TMCommandSP command);

	template<typename Cmd>
	void _addCommand() {
		_addCommand(std::make_shared<Cmd>(this));
	}

	bool _execCommand(const lair::String& command);

private:
	typedef std::unordered_map<lair::String, MapNodeSP>        NodeMap;
	typedef std::unordered_map<lair::String, TMCommand*>       TMCommandMap;
	typedef std::unordered_map<lair::String, CharacterClassSP> ClassMap;

private:
	void _initialize(std::istream& in, const lair::Path& logicPath);

private:
	MainState*  _mainState;
	Console*    _console;

	TMCommandList _commands;
	TMCommandMap  _commandMap;

	NodeMap     _nodes;
	ClassMap    _classes;
	CharacterSP _player;
};


#endif