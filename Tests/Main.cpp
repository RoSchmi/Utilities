#include <gtest/gtest.h>

class f {
	f() = delete;
	f(const f& o) = delete;
	f& operator=(const f& o) = delete;
public:
	int x;
	f(int z) : x(z) {};
	f(f&& o) { this->x = o.x; o.x = -1; };
	f& operator=(f&& o) { this->x = o.x; o.x = -1; return *this; };
};
#include <vector>
#include <iostream>
using namespace std;

vector<f> baz() {
	vector<f> foo;
	foo.push_back(f(0));
	foo.push_back(f(1));
	return foo;
}

void x(vector<f>& bar) {
	auto& i = bar[0];
	auto& j = bar[1];
	cout << i.x << endl;
	cout << j.x << endl;
}

int main(int argc, char **argv) {
	vector<f> foo = baz();
	x(foo);
	system("pause");
	return 0;
	//::testing::InitGoogleTest(&argc, argv);
	//return RUN_ALL_TESTS();
}
