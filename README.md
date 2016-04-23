# mmgen
Generators (kinda) with iterators in C++

This is an attempt to create python-esque generators in C++ using only iterators.
A general `generator` class is defined, which esentially can either give the `next` item, or a pair of iterators, 
one to the current head of the stream, and another to the "end" (if there is one).
The iterators are `InputIterator`s, they can be prefix++'d, the can be `!=` (but not `==`, since we can only tell if an iterator is at the end or not).

Basically, a `generator` can be constructed from any copyable callable, and its the callables responsibility to keep state, and
emmit a `yield_result`. Returning a default constructed `yield_result` signals the end of the generation.

A comfortable way to work with `mmgen` is using `mutable` lambda functions, there are a copule marcos in place for that purpouse.

##Usage Example
####Creating a generator:
```
//this is a function returning a generator, the generator itself is implemented using a mutable lambda
mmgen::generator<int> infrange(int start, int step = 1)
{
	return _MGENERATOR(current = start, step)
	{
		while (true) {
			auto temp = current;
			current += step;
			return mmgen::yield_result<int>(temp);
		}
		return mmgen::yield_result<int>();
	};
}
```
The function `infrange` creates a generator which represents an infinite range. The generator itself is a lambda, capturing the
start and the step. `current` keeps its state between calls. In general, all values that should keep state between calls go in the capture group.
This is a big limitation, but hey.

#### What can we do with `infrange`?
We can iterate it, like so:
```
for (auto i : infrange(10, 2))) {
		std::cout << i << std::endl;
}
```
But this code will run forever.

Lets define another function, which takes n elements from the generator.
```
template<typename T>
mmgen::generator<T> take(mmgen::generator<T> generator, size_t count)
{
	return _MGENERATOR(generator = _TAKE_MGENERATOR(generator), count)
	{
		for (auto&& item : *generator) {
			if (count > 0) {
				--count;
				return mmgen::yield_result<T>(std::forward<decltype(item)>(item));
			} else {
				break;
			}
		}
		return mmgen::yield_result<T>();
	};
}
```
Here, `take` consumes the parameter generator, and yields items until count is 0 (and at this point, we took all the items we wanted).

Now we can take the first 20 items in the generator, for example.
```
for (auto i : take(infrange(10, 2), 20))) {
		std::cout << i << std::endl;
}
```

In the example shown above, you can see how generators can be composed, to create a new generator.
