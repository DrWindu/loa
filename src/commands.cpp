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


#include <functional>

#include <lair/core/log.h>
#include <lair/core/text.h>

#include "map_node.h"
#include "character_class.h"
#include "character.h"
#include "skill.h"
#include "text_moba.h"

#include "commands.h"


using namespace lair;


template<typename It>
String join(It begin, It end, const String& joinString = ", ") {
	std::ostringstream out;
	for(It it = begin; it < end; ++it) {
		if(it != begin)
			out << joinString;
		out << *it;
	}
	return out.str();
}

template<typename Range>
String join(const Range& range, const String& joinString = ", ") {
	return join(range.begin(), range.end(), joinString);
}


String toLower(const String& string) {
	char buffer[8];

	String lower;
	lower.reserve(string.size());

	Utf8CodepointIterator cpit(string);
	while(cpit.hasNext()) {
		*utf8FromCodepoint(buffer, std::tolower(cpit.next())) = '\0';
		lower.append(buffer);
	}

	return lower;
}




HelpCommand::HelpCommand(TextMoba* textMoba)
    : TMCommand(textMoba)
{
	_names.emplace_back("help");
	_names.emplace_back("h");
	_names.emplace_back("?");

	_desc = "  Prints this help message.";
}

bool HelpCommand::exec(const StringVector& /*args*/) {
	for(auto cmd: tm()->commands()) {
		print(join(cmd->names()));
		print(cmd->desc());
	}

	return true;
}



InfoCommand::InfoCommand(TextMoba* textMoba)
    : TMCommand(textMoba)
{
	_names.emplace_back("info");
	_names.emplace_back("i");

	_desc = "  Information about game mechanics.";
}

bool InfoCommand::exec(const StringVector& args) {
	if(args.size() != 2) {
		print("Available topic (use \"", args[0], "\" <topic>\":");
		for(const auto& pair: tm()->infos()) {
			print("  ", pair.first);
		}
	}
	else {
		const String* info = tm()->infos(args[1]);
		if(info) {
			print(*info);
		}
		else {
			print("Unknown topic \"", args[1], "\".");
		}
	}

	return true;
}



LookCommand::LookCommand(TextMoba* textMoba)
    : TMCommand(textMoba)
{
	_names.emplace_back("look");
	_names.emplace_back("l");

	_desc = "  Look around you. Describe the place and what / who is\n"
	        "  here. With a parameter, look at a specific object here.\n"
	        "  Example: look tower (describe a tower)";
}

bool LookCommand::exec(const StringVector& args) {
	if(!player()->isAlive()) {
		print("You are dead... Please use the command \"wait\" until you respawn.");
		return true;
	}

	if(args.size() == 1) {
		MapNodeSP node = player()->node();
		print("You are at ", node->name(), ".");

		print("Here, there is");
		unsigned i = 0;
		CharacterGroups groups = node->characterGroups();
		for(CharacterSP c: node->characters()) {
			print("  ", i, ": ",
			      "[", c->placeName(), "] ", c->name(false), " (lvl ",
			      c->level() + 1, ", ", c->hp(), " / ", c->maxHP(), ")",
			      " dist: ", groups.distanceBetween(player(), c)
			);
			i += 1;
		}
	}

	return true;
}



DirectionsCommand::DirectionsCommand(TextMoba* textMoba)
    : TMCommand(textMoba)
{
	_names.emplace_back("directions");
	_names.emplace_back("dir");
	_names.emplace_back("d");

	_desc = "  List the destinations you can reach from here.";
}

bool DirectionsCommand::exec(const StringVector& /*args*/) {
	if(!player()->isAlive()) {
		print("You are dead... Please use the command \"wait\" until you respawn.");
		return true;
	}

	print("From here, you can go toward:");
	for(const auto& pair: player()->node()->paths()) {
		print("  ", join(pair.second), ": toward ", pair.first->name());
	}
	return true;
}



WaitCommand::WaitCommand(TextMoba* textMoba)
    : TMCommand(textMoba)
{
	_names.emplace_back("wait");
	_names.emplace_back("w");

	_desc = "  Do nothing until next turn.";
}

bool WaitCommand::exec(const StringVector& /*args*/) {
	_textMoba->nextTurn();
	return true;
}



GoCommand::GoCommand(TextMoba* textMoba)
    : TMCommand(textMoba)
{
	_names.emplace_back("go");
	_names.emplace_back("g");

	_desc = "  Walk in a given direction. Type \"directions\" to see where\n"
	        "  you can go. Example: go red (go toward the red base)";
}

bool GoCommand::exec(const StringVector& args) {
	if(!player()->isAlive()) {
		print("You are dead... Please use the command \"wait\" until you respawn.");
		return true;
	}

	if(args.size() != 2) {
		print("I don't understand where you want to go. Type");
		print("  ", args[0], " <direction>");
	}
	else {
		String dir = toLower(args[1]);
		MapNodeSP node = player()->node();
		MapNodeSP dest = node->destination(dir);
		if(dest) {
			tm()->moveCharacter(player(), dest);
			tm()->execCommand("look");
			tm()->nextTurn();
		}
		else {
			print("Unknown direction \"", dir, "\"");
		}
	}

	return true;
}



MoveCommand::MoveCommand(TextMoba* textMoba)
    : TMCommand(textMoba)
{
	_names.emplace_back("move");
	_names.emplace_back("m");

	_desc = "  Take \"front\" or \"back\" in parameter. Move your\n"
	        "  character to the front/back row.";
}

bool MoveCommand::exec(const StringVector& args) {
	if(!player()->isAlive()) {
		print("You are dead... Please use the command \"wait\" until you respawn.");
		return true;
	}

	if(args.size() != 2 || (args[1] != "front" && args[1] != "back")) {
		print("I don't understand where you want to go. Type");
		print("  ", args[0], " [front|back]");
	}
	else {
		Place place = Place((args[1] == "front")? FRONT: BACK);
		if(place == player()->place()) {
			print("You already are at the ", args[1], " row.");
			return true;
		}
		else {
			tm()->placeCharacter(player(), place);
			tm()->nextTurn();
		}
	}

	return true;
}



AttackCommand::AttackCommand(TextMoba* textMoba)
    : TMCommand(textMoba)
{
	_names.emplace_back("attack");
	_names.emplace_back("a");

	_desc = "  Attack the enemy number n, where n is the number you can\n"
	        "  see when you run the command look.";
}

bool AttackCommand::exec(const StringVector& args) {
	if(!player()->isAlive()) {
		print("You are dead... Please use the command \"wait\" until you respawn.");
		return true;
	}

	if(args.size() != 2) {
		print("I don't understand who you want to attack. Type");
		print("  ", args[0], " <character-number>");
		print("where <character-number> is the number displayed");
		print("when you type \"look\".");
	}
	else {
		unsigned index = 9999;
		try {
			index = std::stoi(args[1]);
		}
		catch(std::invalid_argument) {
			print("I don't understand who you try to attack.");
			return true;
		}

		CharacterSP target = player()->node()->characterAt(index);

		if(!target || !target->isAlive()) {
			print("Invalid target.");
			return true;
		}

		if(target->team() == player()->team()) {
			print("You cannot attack allies.");
			return true;
		}

		CharacterGroups groups = player()->node()->characterGroups();
		if(groups.distanceBetween(player(), target) > player()->range()) {
			print("Target out of range.");
			return true;
		}

		player()->attack(target);
		tm()->nextTurn();
	}

	return true;
}



UseCommand::UseCommand(TextMoba* textMoba)
    : TMCommand(textMoba)
{
	_names.emplace_back("use");
	_names.emplace_back("u");

	_desc = "  Use a skill. Some skills need a <character-number> or a\n"
	        "  row (front or back) in parameter. Example: \n"
	        "    use bomb front";
}

bool UseCommand::exec(const StringVector& args) {
	if(!player()->isAlive()) {
		print("You are dead... Please use the command \"wait\" until you respawn.");
		return true;
	}

	if(args.size() < 2) {
		print("I don't understand what you try to use. Type");
		print("  ", args[0], " <skill-name> [<character-number>|front|back]");
	}
	else {
		SkillSP skill = player()->skill(args[1]);
		if(!skill) {
			print("You don't have a skill called ", args[1]);
			return true;
		}

		if(!skill->usable()) {
			print("You can't use this skill right now.");
			return true;
		}

		CharacterVector targets;
		if(skill->target() == SINGLE) {
			if(args.size() != 3) {
				print("This skill targets a single foe and so takes a "
				      "<character-number> in parameter.");
				return true;
			}

			unsigned charIndex = 9999;
			try {
				charIndex = std::stoi(args[2]);
			}
			catch(std::invalid_argument) {
				print("I don't understand who you're trying to attack.");
				return true;
			}

			CharacterSP target = player()->node()->characterAt(charIndex);

			if(!target || !target->isAlive()) {
				print("Invalid target.");
				return true;
			}

			if(target->team() != skill->targetTeam()) {
				print("Target is in the wrong team.");
				return true;
			}

			targets = skill->targets(target);
			if(targets.empty()) {
				print("Target is out-of-range.");
				return true;
			}
		}
		else if(skill->target() == ANY_ROW) {
			if(args.size() != 3 || (args[2] != "front" && args[2] != "back")) {
				print("This skill target a row so you need to choose between "
				      " \"front\" or \"back\". The row must be in range.");
				return true;
			}

			Place place = (args[2] == "front")? FRONT: BACK;
			targets = skill->targets(place);
		}
		else {
			targets = skill->targets();
		}

		if(targets.empty()) {
			print("No targets in range.");
			return true;
		}

		player()->_mana -= skill->manaCost();
		skill->useOn(targets);
		tm()->nextTurn();
	}

	return true;
}



RestartCommand::RestartCommand(TextMoba* textMoba)
    : TMCommand(textMoba)
    , _readClass(false)
{
	_names.emplace_back("restart");

	_desc = "  Restart the game.";
}

bool RestartCommand::exec(const StringVector& args) {
	if(!_readClass && args.size() == 1) {
		print("Choose your class: [ warrior, ranger, mage ]");
		_readClass = true;
		return false;
	}

	String className;
	if(!_readClass && args.size() == 2) {
		className = args[1];
	}
	else if(_readClass && args.size() == 1) {
		className = args[0];
	}
	else if(!_readClass) {
		print(args[0], " takes 0 or 1 parameter.");
		return true;
	}
	else {
		print("Please enter one class name: warrior, ranger or mage.");
		return false;
	}

	if(className == "warrior" || className == "ranger" || className == "mage") {
		tm()->restart(className);
		_readClass = false;
	}
	else {
		print("Unknown class name ", className);
		print("Please enter one class name: warrior, ranger or mage.");
		return !_readClass;
	}

	return true;
}
