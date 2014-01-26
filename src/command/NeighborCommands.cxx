/*
 * Copyright (C) 2003-2014 The Music Player Daemon Project
 * http://www.musicpd.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"
#include "NeighborCommands.hxx"
#include "client/Client.hxx"
#include "Instance.hxx"
#include "Main.hxx"
#include "protocol/Result.hxx"
#include "neighbor/Glue.hxx"
#include "neighbor/Info.hxx"

#include <set>
#include <string>

#include <assert.h>

bool
neighbor_commands_available()
{
	return instance->neighbors != nullptr;
}

CommandResult
handle_listneighbors(Client &client,
		     gcc_unused int argc, gcc_unused char *argv[])
{
	if (instance->neighbors == nullptr) {
		command_error(client, ACK_ERROR_UNKNOWN,
			      "No neighbor plugin configured");
		return CommandResult::ERROR;
	}

	const auto neighbors = instance->neighbors->GetList();
	for (const auto &i : neighbors)
		client_printf(client,
			      "neighbor: %s\n"
			      "name: %s\n",
			      i.uri.c_str(),
			      i.display_name.c_str());
	return CommandResult::OK;
}
