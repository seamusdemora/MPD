// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "TagAny.hxx"
#include "TagStream.hxx"
#include "TagFile.hxx"
#include "tag/Generic.hxx"
#include "song/LightSong.hxx"
#include "db/Interface.hxx"
#include "storage/StorageInterface.hxx"
#include "client/Client.hxx"
#include "protocol/Ack.hxx"
#include "fs/AllocatedPath.hxx"
#include "input/InputStream.hxx"
#include "util/ScopeExit.hxx"
#include "util/StringCompare.hxx"
#include "util/UriExtract.hxx"
#include "LocateUri.hxx"

#include <utility> // for std::unreachable()

static void
TagScanStream(const char *uri, TagHandler &handler)
{
	Mutex mutex;

	auto is = InputStream::OpenReady(uri, mutex);
	if (!tag_stream_scan(*is, handler))
		throw ProtocolError(ACK_ERROR_NO_EXIST, "Failed to load file");

	ScanGenericTags(*is, handler);
}

static void
TagScanFile(const Path path_fs, TagHandler &handler)
{
	if (!ScanFileTagsNoGeneric(path_fs, handler))
		throw ProtocolError(ACK_ERROR_NO_EXIST, "Failed to load file");

	ScanGenericTags(path_fs, handler);
}

#ifdef ENABLE_DATABASE

/**
 * Collapse "../" prefixes in a URI relative to the specified base
 * URI.
 */
static std::string
ResolveUri(std::string_view base, const char *relative)
{
	while (true) {
		const char *rest = StringAfterPrefix(relative, "../");
		if (rest == nullptr)
			break;

		if (base == ".")
			throw ProtocolError(ACK_ERROR_NO_EXIST, "Bad real URI");

		base = PathTraitsUTF8::GetParent(base);
		relative = rest;
	}

	return PathTraitsUTF8::Build(base, relative);
}

/**
 * Look up the specified song in the database and return its
 * (resolved) "real" URI.
 */
static std::string
GetRealSongUri(Client &client, std::string_view uri)
{
	const auto &db = client.GetDatabaseOrThrow();

	const auto *song = db.GetSong(uri);
	if (song == nullptr)
		throw ProtocolError(ACK_ERROR_NO_EXIST, "No such song");

	AtScopeExit(&db, song) { db.ReturnSong(song); };

	if (song->real_uri == nullptr)
		return {};

	return ResolveUri(PathTraitsUTF8::GetParent(uri), song->real_uri);
}

#endif

static void
TagScanDatabase(Client &client, const char *uri, TagHandler &handler)
{
#ifdef ENABLE_DATABASE
	const auto real_uri = GetRealSongUri(client, uri);

	if (!real_uri.empty()) {
		uri = real_uri.c_str();

		// TODO: support absolute paths?
		if (uri_has_scheme(uri))
			return TagScanStream(uri, handler);
	}

	const Storage *storage = client.GetStorage();
	if (storage == nullptr) {
#else
		(void)client;
		(void)uri;
		(void)handler;
#endif
		throw ProtocolError(ACK_ERROR_NO_EXIST, "No database");
#ifdef ENABLE_DATABASE
	}

	if (const auto path_fs = storage->MapFS(uri); !path_fs.IsNull())
		return TagScanFile(path_fs, handler);

	if (const auto absolute_uri = storage->MapUTF8(uri);
	    uri_has_scheme(absolute_uri))
		return TagScanStream(absolute_uri.c_str(), handler);

	throw ProtocolError(ACK_ERROR_NO_EXIST, "No such file");
#endif
}

void
TagScanAny(Client &client, const char *uri, TagHandler &handler)
{
	const auto located_uri = LocateUri(UriPluginKind::INPUT, uri, &client
#ifdef ENABLE_DATABASE
					   , nullptr
#endif
					   );
	switch (located_uri.type) {
	case LocatedUri::Type::ABSOLUTE:
		return TagScanStream(located_uri.canonical_uri, handler);

	case LocatedUri::Type::RELATIVE:
		return TagScanDatabase(client, located_uri.canonical_uri,
				       handler);

	case LocatedUri::Type::PATH:
		return TagScanFile(located_uri.path, handler);
	}

	std::unreachable();
}
