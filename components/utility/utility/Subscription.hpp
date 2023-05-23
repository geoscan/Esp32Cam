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
#include <utility>

namespace Ut {
namespace Sub {

///
/// \tparam Tsub  Subscriber class
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

private:
	Tsub *subscriber;
};

}  // namespace Sub
}  // namespace Ut

#endif // UTILITY_UTILITY_SUBSCRIBER_HPP_
