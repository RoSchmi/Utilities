#include "Common.h"

using namespace std;

date_time util::epoch;

uint64 util::since_epoch(const date_time& dt) {
	return chrono::duration_cast<chrono::milliseconds>(epoch - dt).count();
}

date_time util::from_epoch(uint64 val) {
	return epoch + chrono::milliseconds(val);
}
