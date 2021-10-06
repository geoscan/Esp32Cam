#ifndef RR_SUBSCRIPTION_SUBSCRIPTION_HPP
#define RR_SUBSCRIPTION_SUBSCRIPTION_HPP

#include <list>

// ------------ Thread Safety Macro ------------ //

#if 1
# define SUBSCRIPTION_HPP_ENABLE_THREAD_SAFETY
#endif

#if defined(SUBSCRIPTION_HPP_ENABLE_THREAD_SAFETY)
# include <mutex>
# include <thread>
#endif  /* defined(SUBSCRIPTION_HPP_ENABLE_THREAD_SAFETY) */

namespace Rr {
namespace Subscription {


// ------------ Utility ------------ //

namespace Util {

// https://en.cppreference.com/w/cpp/named_req/BasicLockable
struct MockBasicLockable final {
	inline void lock()
	{
	}
	inline void unlock()
	{
	}
};

// Mocks STL's lock_guard:
// https://en.cppreference.com/w/cpp/thread/lock_guard
template <typename Mutex>
struct LockGuard final {
public:
	LockGuard(Mutex &aMutex) : mutex(aMutex)
	{
		mutex.lock();
	}
	~LockGuard()
	{
		mutex.unlock();
	}
private:
	Mutex &mutex;
};


// ------------ SyncTraits ------------ //
//
// Synchronization facilities used by Key<...> Interface requirements are
// defined by RAII facilities:
// - LockObserverEnable
// - LockObserverStorage
//

// Mock class serving as pseudo-type for subscribers
struct Observer {
};

}  // namespace Util

#if defined(SUBSCRIPTION_HPP_ENABLE_THREAD_SAFETY)
struct SyncTraitsFull {
	using MutObserversStorage = std::mutex;                           // Type of mutex for static observers storage
	using MutObserver         = std::mutex;                           // Type of mutex for particular observers
	using LockObserverStorage = std::lock_guard<MutObserversStorage>; // RAII sync. for static observers storage
	using LockObserverEnable  = std::lock_guard<MutObserver>;         // RAII sync. for particular observers: lifecycle management
	using LockObserverNotify  = std::lock_guard<MutObserver>;         // RAII sync. for particular observers: on 'notify' call
};
#endif /* #if defined(SUBSCRIPTION_HPP_ENABLE_THREAD_SAFETY) */
struct SyncTraitsNoSync {
	using MutObserversStorage = Util::MockBasicLockable;
	using MutObserver         = Util::MockBasicLockable;
	using LockObserverStorage = Util::LockGuard<MutObserversStorage>;
	using LockObserverEnable  = Util::LockGuard<MutObserver>;
	using LockObserverNotify  = Util::LockGuard<MutObserver>;
};

// ------------ Key ------------ //

#if defined(SUBSCRIPTION_HPP_ENABLE_THREAD_SAFETY)
using DefaultSyncTraits = SyncTraitsFull;
#else
using DefaultSyncTraits = SyncTraitsNoSync;
#endif

struct DefaultTopic {
};

// ------------ KeyBase (multiargument) ------------ //


template <typename TopicTrait, typename SyncTraits, typename ...Type>
class KeyBase {
protected:
	using MemberCallable = void (Util::Observer:: *)(Type...);
	using StaticCallable = void (*)(Type...);

	struct ObserverInfo {
		Util::Observer *observer;
		union {
			MemberCallable memberCallable;
			StaticCallable staticCallable;
		};
		typename SyncTraits::MutObserver mutex;
		bool enabled = true;
		ObserverInfo(Util::Observer *aObserver, MemberCallable aCallable, bool aEnabled = true) : observer(aObserver), memberCallable(aCallable), enabled(aEnabled)
		{
		}
		ObserverInfo(StaticCallable aCallable, bool aEnabled = true) : observer(nullptr), staticCallable(aCallable), enabled(aEnabled)
		{
		}
	};

	static struct Observers {
		std::list<ObserverInfo> list;
		typename SyncTraits::MutObserversStorage mutex;
	} observers;

	static std::pair<typename std::list<ObserverInfo>::iterator, typename std::list<ObserverInfo>::iterator> getIterators() {
		typename SyncTraits::LockObserverStorage lock(observers.mutex);
		return {observers.list.begin(), observers.list.end()};
	}

	static ObserverInfo &pushObserver(MemberCallable aObsCall, Util::Observer *aObs, bool aEnabled = true)
	{
		typename SyncTraits::LockObserverStorage lock(observers.mutex);
		observers.list.emplace_back(aObs, aObsCall, aEnabled);
		return observers.list.back();
	}

	static ObserverInfo &pushObserver(StaticCallable aStaticCallable, bool aEnabled = true)
	{
		typename SyncTraits::LockObserverStorage lock(observers.mutex);
		observers.list.emplace_back(aStaticCallable, aEnabled);
		return observers.list.back();
	}

	ObserverInfo *observerInfo = nullptr;

public:
	using Topic = TopicTrait;

	void enableSubscribe(bool aEnabled)
	{
		if (observerInfo) {
			typename SyncTraits::LockObserverEnable lock(observerInfo->mutex);  // Thread-safe, 'coz the entry will not be destructed no matter what
			observerInfo->enabled = aEnabled;
		}
	}

	template <typename InstanceType>
	KeyBase(void (InstanceType:: *callable)(Type...), InstanceType *instance, bool enabled = true)
	{
		observerInfo = &pushObserver((MemberCallable) callable, (Util::Observer *) instance, enabled);
	}

	KeyBase(void (*staticCallable)(Type...), bool enabled = true)
	{
		observerInfo = &pushObserver((StaticCallable) staticCallable, enabled);
	}

	static void notify(Type... args)
	{
		auto iterators = getIterators();  // The returned range won't be changed, therefore there's no reason to syncrhonize access

		for (auto it = iterators.first; it != iterators.second; ++it) {
			if (!it->enabled) {  // Preliminary check to avoid unnecessary lock expenses
				continue;
			}
			typename SyncTraits::LockObserverNotify lock(it->mutex);
			if (!it->enabled) {
				continue;
			}
			auto obs = it->observer;
			if (obs) {  // Member
				auto obsCallable = it->memberCallable;
				(obs->*obsCallable)(args...);
			} else {  // Static
				it->staticCallable(args...);
			}
		}
	}

	KeyBase()
	{
	}

	~KeyBase()
	{
		enableSubscribe(false);
	}
};

template <typename TopicTrait, typename SyncTraits, typename ...Type>
typename KeyBase<TopicTrait, SyncTraits, Type...>::Observers KeyBase<TopicTrait, SyncTraits, Type...>::observers {};

// ------------ Key (single argument) ------------ //

template <typename SingleType, typename TopicTrait = DefaultTopic, typename SyncTraits = DefaultSyncTraits>
class Key : public KeyBase<TopicTrait, SyncTraits, SingleType> {
public:
	using Type = SingleType;
	using KeyBase<TopicTrait, SyncTraits, SingleType>::KeyBase;
};

template <typename TopicTrait, typename SyncTraits>
class Key<void, TopicTrait, SyncTraits> : public KeyBase<TopicTrait, SyncTraits> {
public:
	using Type = void;
	using KeyBase<TopicTrait, SyncTraits>::KeyBase;
};

}  // namespace Subscription
}  // namespace Rr

#endif // RR_SUBSCRIPTION_SUBSCRIPTION_HPP
