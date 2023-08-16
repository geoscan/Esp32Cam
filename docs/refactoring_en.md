# Refactoring guidelines / milestones

## System layer

Severe mistakes were made from the architectural point of view. The code is
detached from neither ESP-IDF framework, nor the hardware. Moreover, it
comprises multiple parts intertwined at various levels of abstraction.

The ideal situation: the application code must invoke system level code, which,
in its turn, must depend on the actual implementation.

Here is the list of doable things that might be taken care of, so the
aforementioned goal will become closer through series of incremental steps;

- [ ] `utility`: Extract out system-related stuff into a different component
	- System-level, and middleware components (such as UartDevice) must reside in a separate component.

## Overengineering

- [ ] Make `module` API and field paradigm separate
	- `module` API implements the paradigm of virtual twins of physical or logical "devices" configurable through **fields**
	- It greatly increases the amount of code that has to be written, and obfuscates the code, while providing no substantial practical benefits;
	- Better be replaced with a more straightforward approach;
		- Invoke modules directly (`Camera`, `Wi-Fi`, ...);
			- not necessarily in a single component;
		- Implement fields later, if needed
- RR library abuse
	- It found its uses in this code beyond event distribution;
	- [ ] Refactor legacy topics
	- [ ] Add a layer of abstraction over RR
	- [ ] XXX? Move subscription topics from RR closer to the actual modules that produce the events
