#ifndef MONITOR_HPP
#define MONITOR_HPP

#include <tabulate/tabulate.hpp>

#include <atomic>
#include <chrono>
#include <iomanip>
#include <thread>

template <class A, class Ord> class LiveMonitor
{
    public:
	LiveMonitor(const std::vector<Protocol::Beaconing::BeaconStore<A, Ord>> &stores, const scheduler::Calendar &cal,
		    std::chrono::milliseconds interval = std::chrono::milliseconds(500))
	    : stores_(stores)
	    , cal_(cal)
	    , interval_(interval)
	    , running_(false)
	{
	}

	void start()
	{
		running_ = true;
		thread_  = std::thread([this]() { loop(); });
	}

	void stop()
	{
		running_ = false;
		if (thread_.joinable())
			thread_.join();
	}

	~LiveMonitor()
	{
		stop();
	}

    private:
	const std::vector<Protocol::Beaconing::BeaconStore<A, Ord>> &stores_;
	const scheduler::Calendar                                   &cal_;
	std::chrono::milliseconds                                    interval_;
	std::atomic<bool>                                            running_;
	std::thread                                                  thread_;

	void loop()
	{
		while (running_)
		{
			print();
			std::this_thread::sleep_for(interval_);
		}
	}

	void print()
	{
		// Snapshot — reading only, no mutation
		tabulate::Table t;
		t.add_row({"Node", "Best attr", "Set size", "Sim time"});
		t[0].format().font_style({tabulate::FontStyle::bold}).font_color(tabulate::Color::cyan);

		for (int u = 0; u < (int)stores_.size(); ++u)
		{
			auto              &store = stores_[u];
			std::ostringstream attr;
			attr << std::fixed << std::setprecision(2) << store.best_candidate.attribute;
			t.add_row({std::to_string(u), attr.str(), std::to_string(store.candidates.size()),
				   std::to_string(cal_.now())});
		}

		// Move cursor to top-left and clear the screen.
		std::cout << "\033[2J\033[H";
		std::cout << "Simulation time: " << cal_.now() << "\n\n";
		std::cout << t << std::flush;
	}
};

#endif
