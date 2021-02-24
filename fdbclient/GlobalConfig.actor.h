/*
 * GlobalConfig.actor.h
 *
 * This source file is part of the FoundationDB open source project
 *
 * Copyright 2013-2021 Apple Inc. and the FoundationDB project authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#if defined(NO_INTELLISENSE) && !defined(FDBCLIENT_GLOBALCONFIG_ACTOR_G_H)
#define FDBCLIENT_GLOBALCONFIG_ACTOR_G_H
#include "fdbclient/GlobalConfig.actor.g.h"
#elif !defined(FDBCLIENT_GLOBALCONFIG_ACTOR_H)
#define FDBCLIENT_GLOBALCONFIG_ACTOR_H

#include <any>
#include <map>
#include <unordered_map>

#include "fdbclient/CommitProxyInterface.h"
#include "fdbclient/ReadYourWrites.h"

#include "flow/actorcompiler.h" // has to be last include

// The global configuration is a series of typed key-value pairs synced to all
// nodes (server and client) in an FDB cluster in an eventually consistent
// manner.

// Keys
extern const KeyRef fdbClientInfoTxnSampleRate;
extern const KeyRef fdbClientInfoTxnSizeLimit;

extern const KeyRef transactionTagSampleRate;
extern const KeyRef transactionTagSampleCost;

class GlobalConfig {
public:
	GlobalConfig(const GlobalConfig&) = delete;
	GlobalConfig& operator=(const GlobalConfig&) = delete;

	static void create(DatabaseContext* cx, Reference<AsyncVar<ClientDBInfo>> dbInfo);
	static GlobalConfig& globalConfig();

	const std::any get(KeyRef name);
	const std::map<KeyRef, std::any> get(KeyRangeRef range);

	template <typename T>
	const T get(KeyRef name) {
		try {
			auto any = get(name);
			return std::any_cast<T>(any);
		} catch (Error& e) {
			throw;
		}
	}

	template <typename T>
	const T get(KeyRef name, T defaultVal) {
		auto any = get(name);
		if (any.has_value()) {
			return std::any_cast<T>(any);
		}

		return defaultVal;
	}

	// To write into the global configuration, submit a transaction to
	// \xff\xff/global_config/<your-key> with <your-value> encoded using the
	// FDB tuple typecodes.

	Future<Void> onInitialized();

private:
	GlobalConfig();

	void insert(KeyRef key, ValueRef value);
	void erase(KeyRef key);
	void erase(KeyRangeRef range);

	ACTOR static Future<Void> refresh(GlobalConfig* self);
	ACTOR static Future<Void> updater(GlobalConfig* self, Reference<AsyncVar<ClientDBInfo>> dbInfo);

	Database cx;
	Future<Void> _updater;
	Promise<Void> initialized;
	Arena arena;
	std::unordered_map<StringRef, std::any> data;
	Version lastUpdate;
};

#endif
