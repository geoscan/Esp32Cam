//
// Subscriber.hpp
//
// Created on: Apr 20, 2022
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//
// Eliminates subscriber / sender boilerplate
//

#ifndef UTILITY_UTILITY_SUBSCRIBER_HPP_
#define UTILITY_UTILITY_SUBSCRIBER_HPP_

#include <type_traits>

namespace Utility {
namespace Sub {

///
/// \tparam Tsub  Subscriber class
/// \tparam Targs Parameters a subscriber accepts
/// \pre    Tsub  Must implement `onSubscription(Targs...)`
///
template <class Tsub>
class Sender1to1 {
public:
	void addSubscriber(Tsub &aSub) {
		subscriber = &aSub;
	}

	void removeSubscriber()
	{
		subscriber = 0;
	}

protected:
	template <class ...Targs>
	void notify(Targs &&...aArgs)
	{
		if (subscriber) {
			subscriber->onSubscription(std::forward<Targs>(aArgs)...);
		}
	}

	/// \brief Sfinae fallback
	///
	void notify(...)
	{
	}

private:
	Tsub *subscriber;
};

}  // namespace Sub
}  // namespace Utility

#endif // UTILITY_UTILITY_SUBSCRIBER_HPP_
