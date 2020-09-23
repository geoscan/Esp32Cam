#ifndef COMPONENTS_UTILITY_RUN_HPP
#define COMPONENTS_UTILITY_RUN_HPP

namespace Utility {

//
// Convenient wrapper which may be seamlessly passed to pthread_create(4)
//

template <typename Runnable>
void *run(void *instance)
{
	Runnable *runnable = reinterpret_cast<Runnable *>(instance);
	runnable->run();
	return nullptr;
}

}  // namespace Utility

#endif  // COMPONENTS_UTILITY_RUN_HPP
