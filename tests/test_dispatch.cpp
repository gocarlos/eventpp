// eventpp library
// Copyright (C) 2018 Wang Qi (wqking)
// Github: https://github.com/wqking/eventpp
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//   http://www.apache.org/licenses/LICENSE-2.0
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "test.h"
#include "eventpp/eventdispatcher.h"

#include <thread>
#include <algorithm>
#include <numeric>
#include <random>

TEST_CASE("dispatch, std::string, void (const std::string &)")
{
	eventpp::EventDispatcher<std::string, void (const std::string &)> dispatcher;

	int a = 1;
	int b = 5;

	dispatcher.appendListener("event1", [&a](const std::string &) {
		a = 2;
	});
	dispatcher.appendListener("event1", eraseArgs1([&b]() {
		b = 8;
	}));

	REQUIRE(a != 2);
	REQUIRE(b != 8);

	dispatcher.dispatch("event1");
	REQUIRE(a == 2);
	REQUIRE(b == 8);
}

TEST_CASE("dispatch, int, void ()")
{
	eventpp::EventDispatcher<int, void ()> dispatcher;

	int a = 1;
	int b = 5;

	dispatcher.appendListener(3, [&a]() {
		a = 2;
	});
	dispatcher.appendListener(3, [&b]() {
		b = 8;
	});

	REQUIRE(a != 2);
	REQUIRE(b != 8);

	dispatcher.dispatch(3);
	REQUIRE(a == 2);
	REQUIRE(b == 8);
}

TEST_CASE("add/remove, int, void ()")
{
	eventpp::EventDispatcher<int, void ()> dispatcher;
	constexpr int event = 3;

	int a = 1;
	int b = 5;

	decltype(dispatcher)::Handle ha;
	decltype(dispatcher)::Handle hb;

	ha = dispatcher.appendListener(event, [event, &a, &dispatcher, &ha, &hb]() {
		a = 2;
		dispatcher.removeListener(event, hb);
		dispatcher.removeListener(event, ha);
	});
	hb = dispatcher.appendListener(event, [&b]() {
		b = 8;
	});

	REQUIRE(ha);
	REQUIRE(hb);

	REQUIRE(a != 2);
	REQUIRE(b != 8);

	dispatcher.dispatch(event);

	REQUIRE(! ha);
	REQUIRE(! hb);

	REQUIRE(a == 2);
	REQUIRE(b != 8);

	a = 1;
	REQUIRE(a != 2);
	REQUIRE(b != 8);

	dispatcher.dispatch(event);
	REQUIRE(a != 2);
	REQUIRE(b != 8);
}

TEST_CASE("dispatch, add another listener inside a listener, int, void ()")
{
	eventpp::EventDispatcher<int, void ()> dispatcher;
	constexpr int event = 3;

	int a = 1;
	int b = 5;

	dispatcher.appendListener(event, [&event, &a, &dispatcher, &b]() {
		a = 2;
		dispatcher.appendListener(event, [&event, &b]() {
			b = 8;
		});
	});

	REQUIRE(a != 2);
	REQUIRE(b != 8);

	dispatcher.dispatch(event);
	REQUIRE(a == 2);
	REQUIRE(b != 8);
}

TEST_CASE("dispatch inside dispatch, int, void ()")
{
	eventpp::EventDispatcher<int, void ()> dispatcher;
	constexpr int event1 = 3;
	constexpr int event2 = 5;

	int a = 1;
	int b = 5;

	decltype(dispatcher)::Handle ha;
	decltype(dispatcher)::Handle hb;

	ha = dispatcher.appendListener(event1, [&a, &dispatcher, event2]() {
		a = 2;
		dispatcher.dispatch(event2);
	});
	hb = dispatcher.appendListener(event2, [&b, &dispatcher, event1, event2, &ha, &hb]() {
		b = 8;
		dispatcher.removeListener(event1, ha);
		dispatcher.removeListener(event2, hb);
	});

	REQUIRE(ha);
	REQUIRE(hb);

	REQUIRE(a != 2);
	REQUIRE(b != 8);

	dispatcher.dispatch(event1);

	REQUIRE(! ha);
	REQUIRE(! hb);

	REQUIRE(a == 2);
	REQUIRE(b == 8);
}

TEST_CASE("dispatch, int, void (const std::string &, int)")
{
	eventpp::EventDispatcher<int, void (const std::string &, int)> dispatcher;
	constexpr int event = 3;

	std::vector<std::string> sList(2);
	std::vector<int> iList(sList.size());

	dispatcher.appendListener(event, [event, &dispatcher, &sList, &iList](const std::string & s, int i) {
		sList[0] = s;
		iList[0] = i;
	});
	dispatcher.appendListener(event, [event, &dispatcher, &sList, &iList](std::string s, const int i) {
		sList[1] = s + "2";
		iList[1] = i + 5;
	});

	REQUIRE(sList[0] != "first");
	REQUIRE(sList[1] != "first2");
	REQUIRE(iList[0] != 3);
	REQUIRE(iList[1] != 8);

	dispatcher.dispatch(event, "first", 3);

	REQUIRE(sList[0] == "first");
	REQUIRE(sList[1] == "first2");
	REQUIRE(iList[0] == 3);
	REQUIRE(iList[1] == 8);
}

TEST_CASE("dispatch, Event struct, void (const std::string &, int)")
{
	struct MyEvent {
		int type;
		std::string message;
		int param;
	};
	struct EventTypeGetter : public eventpp::EventGetterBase
	{
		using Event = int;

		static int getEvent(const MyEvent & e, const std::string &, int) {
			return e.type;
		}
	};


	eventpp::EventDispatcher<EventTypeGetter, void (const MyEvent &, const std::string &, int)> dispatcher;
	constexpr int event = 3;

	std::vector<std::string> sList(2);
	std::vector<int> iList(sList.size());

	dispatcher.appendListener(event, [event, &dispatcher, &sList, &iList](const MyEvent & e, const std::string & s, int i) {
		sList[0] = e.message + " " + s;
		iList[0] = e.param + i;
	});
	dispatcher.appendListener(event, [event, &dispatcher, &sList, &iList](const MyEvent & e, std::string s, const int i) {
		sList[1] = s + " " + e.message;
		iList[1] = e.param * i;
	});

	REQUIRE(sList[0] != "Hello World");
	REQUIRE(sList[1] != "World Hello");
	REQUIRE(iList[0] != 8);
	REQUIRE(iList[1] != 15);

	dispatcher.dispatch(MyEvent{ event, "Hello", 5 }, "World", 3);

	REQUIRE(sList[0] == "Hello World");
	REQUIRE(sList[1] == "World Hello");
	REQUIRE(iList[0] == 8);
	REQUIRE(iList[1] == 15);
}

TEST_CASE("dispatch many, int, void (int)")
{
	eventpp::EventDispatcher<int, void (int)> dispatcher;

	constexpr int eventCount = 1024 * 64;
	std::vector<int> eventList(eventCount);

	std::iota(eventList.begin(), eventList.end(), 0);
	std::shuffle(eventList.begin(), eventList.end(), std::mt19937(std::random_device()()));

	std::vector<int> dataList(eventCount);

	for(int i = 1; i < eventCount; i += 2) {
		dispatcher.appendListener(eventList[i], [&dispatcher, i, &dataList](const int e) {
			dataList[i] = e;
		});
	}

	for(int i = 0; i < eventCount; i += 2) {
		dispatcher.appendListener(eventList[i], [&dispatcher, i, &dataList](const int e) {
			dataList[i] = e;
		});
	}

	for(int i = 0; i < eventCount; ++i) {
		dispatcher.dispatch(eventList[i]);
	}

	std::sort(eventList.begin(), eventList.end());
	std::sort(dataList.begin(), dataList.end());

	REQUIRE(eventList == dataList);
}

TEST_CASE("event filter")
{
	using ED = eventpp::EventDispatcher<int, void (int, int)>;
	ED dispatcher;

	constexpr int itemCount = 5;
	std::vector<int> dataList(itemCount);

	for(int i = 0; i < itemCount; ++i) {
		dispatcher.appendListener(i, [&dataList, i](int e, int index) {
			dataList[e] = index;
		});
	}


	constexpr int filterCount = 2;
	std::vector<int> filterData(filterCount);

	SECTION("Filter invoked count") {
		dispatcher.appendFilter([&filterData](int /*e*/, int /*index*/) -> bool {
			++filterData[0];
			return true;
		});
		dispatcher.appendFilter([&filterData](int /*e*/, int /*index*/) -> bool {
			++filterData[1];
			return true;
		});

		for(int i = 0; i < itemCount; ++i) {
			dispatcher.dispatch(i, 58);
		}

		REQUIRE(filterData == std::vector<int>{ itemCount, itemCount });
		REQUIRE(dataList == std::vector<int>{ 58, 58, 58, 58, 58 });
	}

	SECTION("First filter blocks all other filters and listeners") {
		dispatcher.appendFilter([&filterData](int e, int /*index*/) -> bool {
			++filterData[0];
			if(e >= 2) {
				return false;
			}
			return true;
		});
		dispatcher.appendFilter([&filterData](int /*e*/, int /*index*/) -> bool {
			++filterData[1];
			return true;
		});

		for(int i = 0; i < itemCount; ++i) {
			dispatcher.dispatch(i, 58);
		}

		REQUIRE(filterData == std::vector<int>{ itemCount, 2 });
		REQUIRE(dataList == std::vector<int>{ 58, 58, 0, 0, 0 });
	}

	SECTION("Second filter doesn't block first filter but all listeners") {
		dispatcher.appendFilter([&filterData](int /*e*/, int /*index*/) -> bool {
			++filterData[0];
			return true;
		});
		dispatcher.appendFilter([&filterData](int e, int /*index*/) -> bool {
			++filterData[1];
			if(e >= 2) {
				return false;
			}
			return true;
		});

		for(int i = 0; i < itemCount; ++i) {
			dispatcher.dispatch(i, 58);
		}

		REQUIRE(filterData == std::vector<int>{ itemCount, itemCount });
		REQUIRE(dataList == std::vector<int>{ 58, 58, 0, 0, 0 });
	}

	SECTION("Filter manipulates the parameters") {
		dispatcher.appendFilter([&filterData](int e, int & index) -> bool {
			++filterData[0];
			if(e >= 2) {
				++index;
			}
			return true;
		});
		dispatcher.appendFilter([&filterData](int /*e*/, int /*index*/) -> bool {
			++filterData[1];
			return true;
		});

		for(int i = 0; i < itemCount; ++i) {
			dispatcher.dispatch(i, 58);
		}

		REQUIRE(filterData == std::vector<int>{ itemCount, itemCount });
		REQUIRE(dataList == std::vector<int>{ 58, 58, 59, 59, 59 });
	}
}

TEST_CASE("dispatch multi threading, int, void (int)")
{
	using ED = eventpp::EventDispatcher<int, void (int)>;
	ED dispatcher;

	constexpr int threadCount = 256;
	constexpr int eventCountPerThread = 1024 * 4;
	constexpr int itemCount = threadCount * eventCountPerThread;

	std::vector<int> eventList(itemCount);
	std::iota(eventList.begin(), eventList.end(), 0);
	std::shuffle(eventList.begin(), eventList.end(), std::mt19937(std::random_device()()));

	std::vector<int> dataList(itemCount);
	std::vector<ED::Handle> handleList(itemCount);

	std::vector<std::thread> threadList;

	for(int i = 0; i < threadCount; ++i) {
		threadList.emplace_back([i, eventCountPerThread, &dispatcher, &eventList, &handleList, &dataList]() {
			for(int k = i * eventCountPerThread; k < (i + 1) * eventCountPerThread; ++k) {
				handleList[k] = dispatcher.appendListener(eventList[k], [&dispatcher, k, &dataList, &eventList, &handleList](const int e) {
					dataList[k] += e;
					dispatcher.removeListener(eventList[k], handleList[k]);
				});
			}
		});
	}
	for(int i = 0; i < threadCount; ++i) {
		threadList[i].join();
	}

	threadList.clear();
	for(int i = 0; i < threadCount; ++i) {
		threadList.emplace_back([i, eventCountPerThread, &dispatcher, &eventList]() {
			for(int k = i * eventCountPerThread; k < (i + 1) * eventCountPerThread; ++k) {
				dispatcher.dispatch(eventList[k]);
			}
		});
	}
	for(int i = 0; i < threadCount; ++i) {
		threadList[i].join();
	}

	std::sort(eventList.begin(), eventList.end());
	std::sort(dataList.begin(), dataList.end());

	REQUIRE(eventList == dataList);
}

