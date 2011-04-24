#include <stdio.h>
#include <CoreFoundation/CoreFoundation.h>
#include <sqlite3.h>
#include <unistd.h>
#include <sys/stat.h>

// untrackerd
// Ryan Petrich
// Continuously clean up locationd's history data

#define DATABASE_PATH "/var/root/Library/Caches/locationd/consolidated.db"

static time_t last_time;

static inline bool sqlite_is_success(int result)
{
	switch (result) {
		case SQLITE_OK:
		case SQLITE_DONE:
			return true;
		default:
			return false;
	}
}

static inline bool clear_data()
{
	struct stat file_stat;
	// Determine if file was modified
	if (stat(DATABASE_PATH, &file_stat) == 0) {
		time_t old_time = last_time;
		last_time = file_stat.st_mtime;
		if (last_time == old_time) {
			return true;
		}
	}
	sqlite3 *database = NULL;
	// Open Database
	int result = sqlite3_open(DATABASE_PATH, &database);
	if (sqlite_is_success(result)) {
		sqlite3_stmt *statement = NULL;
		CFAbsoluteTime time = CFAbsoluteTimeGetCurrent() - (60.0 * 30.0);
		// Ensure secure delete is set
		result = sqlite3_prepare_v2(database, "PRAGMA secure_delete;", -1, &statement, NULL);
		if (!sqlite_is_success(result))
			goto close;
		int secure_delete = 0;
		while ((result = sqlite3_step(statement)) == SQLITE_ROW) {
			secure_delete = sqlite3_column_int(statement, 0);
		}
		sqlite3_finalize(statement);
		if (secure_delete == 0) {
			sqlite3_exec(database, "PRAGMA secure_delete = 1;", NULL, NULL, NULL);
		}
		if (!sqlite_is_success(result))
			goto close;
		// Clear Cell data
		result = sqlite3_prepare_v2(database, "DELETE FROM CellLocation WHERE Timestamp < ?;", -1, &statement, NULL);
		if (!sqlite_is_success(result))
			goto close;
		result = sqlite3_bind_double(statement, 1, time);
		if (sqlite_is_success(result)) {
			do {
				result = sqlite3_step(statement);
			} while (result == SQLITE_ROW);
		}
		sqlite3_finalize(statement);
		if (!sqlite_is_success(result))
			goto close;
		// Clear Local Cell data
		result = sqlite3_prepare_v2(database, "DELETE FROM CellLocationLocal WHERE Timestamp < ?;", -1, &statement, NULL);
		if (!sqlite_is_success(result))
			goto close;
		result = sqlite3_bind_double(statement, 1, time);
		if (sqlite_is_success(result)) {
			do {
				result = sqlite3_step(statement);
			} while (result == SQLITE_ROW);
		}
		sqlite3_finalize(statement);
		if (!sqlite_is_success(result))
			goto close;
		// Clear WiFi Data
		result = sqlite3_prepare_v2(database, "DELETE FROM WifiLocation WHERE Timestamp < ?;", -1, &statement, NULL);
		if (!sqlite_is_success(result))
			goto close;
		result = sqlite3_bind_double(statement, 1, time);
		if (sqlite_is_success(result)) {
			do {
				result = sqlite3_step(statement);
			} while (result == SQLITE_ROW);
		}
		sqlite3_finalize(statement);
		if (!sqlite_is_success(result))
			goto close;
		// Apply vacuuming if necessary
		result = sqlite3_prepare_v2(database, "PRAGMA auto_vacuum;", -1, &statement, NULL);
		if (!sqlite_is_success(result))
			goto close;
		int auto_vacuum = 0;
		while ((result = sqlite3_step(statement)) == SQLITE_ROW) {
			auto_vacuum = sqlite3_column_int(statement, 0);
		}
		sqlite3_finalize(statement);
		if (auto_vacuum == 0) {
			sqlite3_exec(database, "PRAGMA auto_vacuum = 1; VACUUM;", NULL, NULL, NULL);
		}
close:
		// Close Database
		sqlite3_close(database);
	}
	return sqlite_is_success(result);
}

static void timer_callback(CFRunLoopTimerRef timer, void *info)
{
	clear_data();
}

int main(int argc, const char *argv[])
{
	// Loop ad infinitum
	CFRunLoopTimerRef timer = CFRunLoopTimerCreate(kCFAllocatorDefault, CFAbsoluteTimeGetCurrent(), (5.0 * 60.0), 0, 0, &timer_callback, NULL);
	CFRunLoopRef loop = CFRunLoopGetCurrent();
	CFRunLoopAddTimer(loop, timer, kCFRunLoopCommonModes);
	CFRunLoopRun();
	CFRunLoopRemoveTimer(loop, timer, kCFRunLoopCommonModes);
	CFRelease(timer);
	return 0;
}
