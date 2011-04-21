#include <stdio.h>
#include <CoreFoundation/CoreFoundation.h>
#include <sqlite3.h>

// untrackerd
// Ryan Petrich
// Continuously clean up locationd's history data

static inline bool clear_data()
{
	sqlite3 *database = NULL;
	// Open Database
	int result = sqlite3_open("/var/root/Library/Caches/locationd/consolidated.db", &database);
	if (result == SQLITE_OK) {
		sqlite3_stmt *statement = NULL;
		CFAbsoluteTime time = CFAbsoluteTimeGetCurrent() - (60.0 * 30.0);
		// Clear Cell data
		result = sqlite3_prepare_v2(database, "DELETE FROM CellLocation WHERE Timestamp < ?;", -1, &statement, NULL);
		if (result == SQLITE_OK) {
			result = sqlite3_bind_double(statement, 1, time);
			if (result == SQLITE_OK) {
				do {
					result = sqlite3_step(statement);
				} while (result == SQLITE_ROW);
			}
			sqlite3_finalize(statement);
		}
		// Clear WiFi Data
		if (result == SQLITE_DONE) {
			result = sqlite3_prepare_v2(database, "DELETE FROM WifiLocation WHERE Timestamp < ?;", -1, &statement, NULL);
			if (result == SQLITE_OK) {
				result = sqlite3_bind_double(statement, 1, time);
				if (result == SQLITE_OK) {
					do {
						result = sqlite3_step(statement);
					} while (result == SQLITE_ROW);
				}
				sqlite3_finalize(statement);
			}
		}
		// Close Database
		sqlite3_close(database);
	}
	return (result == SQLITE_OK) || (result == SQLITE_DONE);
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
