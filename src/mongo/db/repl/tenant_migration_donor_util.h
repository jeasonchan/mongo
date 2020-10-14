/**
 *    Copyright (C) 2020-present MongoDB, Inc.
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the Server Side Public License, version 1,
 *    as published by MongoDB, Inc.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    Server Side Public License for more details.
 *
 *    You should have received a copy of the Server Side Public License
 *    along with this program. If not, see
 *    <http://www.mongodb.com/licensing/server-side-public-license>.
 *
 *    As a special exception, the copyright holders give permission to link the
 *    code of portions of this program with the OpenSSL library under certain
 *    conditions as described in each individual source file and distribute
 *    linked combinations including the program with the OpenSSL library. You
 *    must comply with the Server Side Public License in all respects for
 *    all of the code used other than as permitted herein. If you modify file(s)
 *    with this exception, you may extend this exception to your version of the
 *    file(s), but you are not obligated to do so. If you do not wish to do so,
 *    delete this exception statement from your version. If you delete this
 *    exception statement from all source files in the program, then also delete
 *    it in the license file.
 */

#pragma once

#include "mongo/db/commands.h"
#include "mongo/db/commands/tenant_migration_donor_cmds_gen.h"
#include "mongo/db/operation_context.h"
#include "mongo/db/repl/tenant_migration_access_blocker_by_prefix.h"
#include "mongo/db/repl/tenant_migration_conflict_info.h"
#include "mongo/db/repl/tenant_migration_state_machine_gen.h"
#include "mongo/db/request_execution_context.h"
#include "mongo/executor/task_executor.h"
#include "mongo/rpc/get_status_from_command_result.h"
#include "mongo/rpc/reply_builder_interface.h"
#include "mongo/util/functional.h"
#include "mongo/util/future.h"

namespace mongo {

namespace tenant_migration_donor {

/**
 * Creates a task executor to be used for a tenant migration.
 */
std::unique_ptr<executor::TaskExecutor> makeTenantMigrationExecutor(ServiceContext* serviceContext);

/**
 * Updates the donor's in-memory migration state to reflect the given persisted state.
 */
void onWriteToDonorStateDoc(OperationContext* opCtx, const BSONObj& donorStateDocBson);

/**
 * If the operation has read concern "snapshot" or includes afterClusterTime, and the database is
 * in the read blocking state at the given atClusterTime or afterClusterTime or the selected read
 * timestamp, blocks until the migration is committed or aborted.
 */
void checkIfCanReadOrBlock(OperationContext* opCtx, StringData dbName);

/**
 * If the operation has read concern "linearizable", throws TenantMigrationCommitted error if the
 * database has been migrated to a different replica set.
 */
void checkIfLinearizableReadWasAllowedOrThrow(OperationContext* opCtx, StringData dbName);

/**
 * Throws TenantMigrationConflict if the database is being migrated and the migration is in the
 * blocking state. Throws TenantMigrationCommitted if it is in committed.
 */
void onWriteToDatabase(OperationContext* opCtx, StringData dbName);

/**
 * Returns a future that asynchronously schedules and runs the argument function 'callable'. If it
 * throws a TenantMigrationConflict error (as indicated in 'replyBuilder'), clears 'replyBuilder'
 * and blocks until the migration commits or aborts, then returns TenantMigrationCommitted or
 * TenantMigrationAborted.
 */
Future<void> migrationConflictHandler(std::shared_ptr<RequestExecutionContext> rec,
                                      unique_function<Future<void>()> callable);

}  // namespace tenant_migration_donor

}  // namespace mongo
